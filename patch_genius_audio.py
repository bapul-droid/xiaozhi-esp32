from pathlib import Path

root = Path(r"D:\xiaozhi-esp32")
header_path = root / "main" / "genius_client" / "genius_client.h"
source_path = root / "main" / "genius_client" / "genius_client.cc"

header = header_path.read_text(encoding="utf-8")
source = source_path.read_text(encoding="utf-8")

declarations = """    static void AudioTaskEntry(void* arg);

    void StartLocalAudio(
        const std::string& filename
    );

"""

if "static void AudioTaskEntry(void* arg);" not in header:
    anchor = "    std::string BuildDeviceId() const;"

    if anchor not in header:
        raise RuntimeError(
            "Tidak menemukan BuildDeviceId() di genius_client.h"
        )

    header = header.replace(
        anchor,
        declarations + anchor,
        1,
    )

if "TaskHandle_t audio_task_handle_" not in header:
    anchor = "    TaskHandle_t task_handle_ = nullptr;"

    if anchor not in header:
        raise RuntimeError(
            "Tidak menemukan task_handle_ di genius_client.h"
        )

    header = header.replace(
        anchor,
        anchor + "\n"
        "    TaskHandle_t audio_task_handle_ = nullptr;",
        1,
    )

include_line = '#include "audio_stream_client.h"'

if include_line not in source:
    anchor = '#include "genius_client.h"'

    if anchor not in source:
        raise RuntimeError(
            "Tidak menemukan include genius_client.h"
        )

    source = source.replace(
        anchor,
        anchor + "\n" + include_line,
        1,
    )

task_struct = """
struct LocalAudioTaskArgs {
    GeniusClient* client;
    std::string filename;
};
"""

if "struct LocalAudioTaskArgs" not in source:
    anchor = "namespace {"

    if anchor not in source:
        raise RuntimeError(
            "Tidak menemukan blok namespace di genius_client.cc"
        )

    source = source.replace(
        anchor,
        anchor + "\n" + task_struct,
        1,
    )

implementations = r'''

void GeniusClient::StartLocalAudio(
    const std::string& filename
)
{
    ESP_LOGI(
        TAG,
        "StartLocalAudio(): %s",
        filename.c_str()
    );

    if (audio_task_handle_ != nullptr) {
        ESP_LOGW(
            TAG,
            "Audio task is already running"
        );
        return;
    }

    auto* args = new LocalAudioTaskArgs{
        this,
        filename,
    };

    BaseType_t result = xTaskCreate(
        AudioTaskEntry,
        "genius_audio",
        8192,
        args,
        3,
        &audio_task_handle_
    );

    if (result != pdPASS) {
        audio_task_handle_ = nullptr;
        delete args;

        ESP_LOGE(
            TAG,
            "Failed to create audio task"
        );
    }
}


void GeniusClient::AudioTaskEntry(void* arg)
{
    ESP_LOGI(TAG, "Audio task started");

    auto* args =
        static_cast<LocalAudioTaskArgs*>(arg);

    GeniusClient* client = args->client;
    const std::string filename = args->filename;

    delete args;

    const std::string url =
        std::string(GENIUS_SERVER_URL) +
        "/api/audio/local/" +
        filename;

    AudioStreamClient audio_client;

    const bool success =
        audio_client.Download(url);

    ESP_LOGI(
        TAG,
        "Local audio download finished: %s",
        success ? "success" : "failed"
    );

    client->audio_task_handle_ = nullptr;

    vTaskDelete(nullptr);
}
'''

if "void GeniusClient::StartLocalAudio(" not in source:
    source = source.rstrip() + implementations + "\n"

header_path.write_text(header, encoding="utf-8")
source_path.write_text(source, encoding="utf-8")

print("PATCH BERHASIL")
print("Diperbarui:", header_path)
print("Diperbarui:", source_path)