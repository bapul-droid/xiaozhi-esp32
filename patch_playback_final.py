from pathlib import Path

source_path = Path(
    r"D:\xiaozhi-esp32\main\genius_client\genius_client.cc"
)

source = source_path.read_text(encoding="utf-8")

old_code = '''void GeniusClient::StartLocalAudio(
    const std::string& filename
)
{
    ESP_LOGI(
        TAG,
        "StartLocalAudio(): %s",
        filename.c_str()
    );
}
'''

new_code = '''void GeniusClient::StartLocalAudio(
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
            "Audio playback is already running"
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
            "Failed to create audio playback task"
        );
    }
}


void GeniusClient::AudioTaskEntry(void* arg)
{
    auto* args =
        static_cast<LocalAudioTaskArgs*>(arg);

    GeniusClient* client = args->client;
    std::string filename = args->filename;

    delete args;

    ESP_LOGI(
        TAG,
        "Audio task started: %s",
        filename.c_str()
    );

    const std::string url =
        std::string(GENIUS_SERVER_URL) +
        "/api/audio/local/" +
        filename;

    AudioStreamClient audio_client;

    const bool success =
        audio_client.Download(url);

    ESP_LOGI(
        TAG,
        "Audio playback finished: %s",
        success ? "success" : "failed"
    );

    client->audio_task_handle_ = nullptr;

    vTaskDelete(nullptr);
}
'''

if old_code not in source:
    raise RuntimeError(
        "Fungsi StartLocalAudio lama tidak ditemukan. "
        "Patch dibatalkan agar file tidak rusak."
    )

if "void GeniusClient::AudioTaskEntry(void* arg)" in source:
    raise RuntimeError(
        "AudioTaskEntry sudah ada. Patch dibatalkan "
        "untuk mencegah fungsi ganda."
    )

backup_path = source_path.with_suffix(".cc.before_playback")
backup_path.write_text(source, encoding="utf-8")

source = source.replace(old_code, new_code, 1)
source_path.write_text(source, encoding="utf-8")

print("PATCH PLAYBACK BERHASIL")
print("Backup:", backup_path)
print("Updated:", source_path)