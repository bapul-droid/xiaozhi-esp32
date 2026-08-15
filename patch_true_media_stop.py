from pathlib import Path


ROOT = Path(r"D:\xiaozhi-esp32")

CLIENT_H = ROOT / "main" / "genius_client" / "genius_client.h"
CLIENT_CC = ROOT / "main" / "genius_client" / "genius_client.cc"
STREAM_H = ROOT / "main" / "genius_client" / "audio_stream_client.h"
STREAM_CC = ROOT / "main" / "genius_client" / "audio_stream_client.cc"


def backup(path: Path) -> None:
    backup_path = path.with_suffix(path.suffix + ".before_true_stop")

    if not backup_path.exists():
        backup_path.write_text(
            path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )

    print("Backup:", backup_path)


for file_path in [CLIENT_H, CLIENT_CC, STREAM_H, STREAM_CC]:
    backup(file_path)


# =========================================================
# genius_client.h
# =========================================================

text = CLIENT_H.read_text(encoding="utf-8")

if "#include <atomic>" not in text:
    text = text.replace(
        "#include <string>\n",
        "#include <string>\n#include <atomic>\n",
        1,
    )

old = """    TaskHandle_t audio_task_handle_ = nullptr;
    TaskHandle_t task_handle_ = nullptr;
    bool registered_ = false;
"""

new = """    TaskHandle_t audio_task_handle_ = nullptr;
    TaskHandle_t task_handle_ = nullptr;

    std::atomic<bool> audio_stop_requested_{false};

    bool registered_ = false;
"""

if "audio_stop_requested_" not in text:
    if old not in text:
        raise RuntimeError(
            "Lokasi member GeniusClient tidak ditemukan"
        )

    text = text.replace(old, new, 1)

CLIENT_H.write_text(text, encoding="utf-8")


# =========================================================
# audio_stream_client.h
# =========================================================

STREAM_H.write_text(
r'''#pragma once

#include <atomic>
#include <string>


class AudioStreamClient {
public:
    AudioStreamClient() = default;

    bool Download(
        const std::string& url,
        const std::atomic<bool>& stop_requested
    );
};
''',
    encoding="utf-8",
)


# =========================================================
# audio_stream_client.cc
# =========================================================

text = STREAM_CC.read_text(encoding="utf-8")

text = text.replace(
"""bool AudioStreamClient::Download(
    const std::string& url
) {""",
"""bool AudioStreamClient::Download(
    const std::string& url,
    const std::atomic<bool>& stop_requested
) {""",
1,
)

old_capture = """            &timestamp,
            &opus_packet_count,
            &queue_error
"""

new_capture = """            &timestamp,
            &opus_packet_count,
            &queue_error,
            &stop_requested
"""

if "&stop_requested" not in text:
    if old_capture not in text:
        raise RuntimeError(
            "Capture callback demuxer tidak ditemukan"
        )

    text = text.replace(
        old_capture,
        new_capture,
        1,
    )

old_callback_check = """                data == nullptr ||
                length == 0 ||
                queue_error
"""

new_callback_check = """                data == nullptr ||
                length == 0 ||
                queue_error ||
                stop_requested.load()
"""

if "queue_error ||\n                stop_requested.load()" not in text:
    if old_callback_check not in text:
        raise RuntimeError(
            "Pemeriksaan callback demuxer tidak ditemukan"
        )

    text = text.replace(
        old_callback_check,
        new_callback_check,
        1,
    )

old_loop = """    while (!queue_error) {
        const int bytes_read =
"""

new_loop = """    while (
        !queue_error &&
        !stop_requested.load()
    ) {
        const int bytes_read =
"""

if "!stop_requested.load()" not in text[text.find("while (!queue_error)") - 50:]:
    if old_loop not in text:
        raise RuntimeError(
            "Loop download audio tidak ditemukan"
        )

    text = text.replace(old_loop, new_loop, 1)

old_after_read = """        if (bytes_read == 0) {
            break;
        }

        total_bytes +=
"""

new_after_read = """        if (bytes_read == 0) {
            break;
        }

        if (stop_requested.load()) {
            ESP_LOGI(
                TAG,
                "Stop requested while reading stream"
            );
            break;
        }

        total_bytes +=
"""

if "Stop requested while reading stream" not in text:
    if old_after_read not in text:
        raise RuntimeError(
            "Posisi pemeriksaan hasil Read tidak ditemukan"
        )

    text = text.replace(
        old_after_read,
        new_after_read,
        1,
    )

old_finish = """    ESP_LOGI(
        TAG,
        "Audio stream finished: "
        "%u bytes, %u Opus packets",
"""

new_finish = """    if (stop_requested.load()) {
        ESP_LOGI(
            TAG,
            "Audio stream stopped by request"
        );
    }

    ESP_LOGI(
        TAG,
        "Audio stream finished: "
        "%u bytes, %u Opus packets",
"""

if "Audio stream stopped by request" not in text:
    if old_finish not in text:
        raise RuntimeError(
            "Log akhir stream tidak ditemukan"
        )

    text = text.replace(
        old_finish,
        new_finish,
        1,
    )

old_return = """    return (
        !queue_error &&
        opus_packet_count > 0
    );
"""

new_return = """    return (
        !stop_requested.load() &&
        !queue_error &&
        opus_packet_count > 0
    );
"""

if old_return not in text:
    raise RuntimeError(
        "Return AudioStreamClient tidak ditemukan"
    )

text = text.replace(old_return, new_return, 1)

STREAM_CC.write_text(text, encoding="utf-8")


# =========================================================
# genius_client.cc
# =========================================================

text = CLIENT_CC.read_text(encoding="utf-8")

old_remote_stop = """    } else if (
        strcmp(action_text, "stop_audio") == 0
    ) {
        Application::GetInstance()
            .GetAudioService()
            .ResetDecoder();

        ESP_LOGI(TAG, "Audio stop requested");

"""

new_remote_stop = """    } else if (
        strcmp(action_text, "stop_audio") == 0
    ) {
        StopAudio();

        ESP_LOGI(TAG, "Audio stop requested");

"""

if old_remote_stop in text:
    text = text.replace(
        old_remote_stop,
        new_remote_stop,
        1,
    )

old_stop = """void GeniusClient::StopAudio()
{
    Application::GetInstance()
        .GetAudioService()
        .ResetDecoder();

    ESP_LOGI(TAG, "Audio stopped by MCP");
}
"""

new_stop = """void GeniusClient::StopAudio()
{
    if (audio_task_handle_ == nullptr) {
        ESP_LOGI(TAG, "No media playback to stop");

        Application::GetInstance()
            .GetAudioService()
            .ResetDecoder();

        return;
    }

    audio_stop_requested_.store(true);

    Application::GetInstance()
        .GetAudioService()
        .ResetDecoder();

    ESP_LOGI(TAG, "Audio stop requested");
}
"""

if old_stop not in text:
    raise RuntimeError(
        "Fungsi GeniusClient::StopAudio tidak ditemukan"
    )

text = text.replace(old_stop, new_stop, 1)


# Reset stop flag before starting radio
old_radio = """void GeniusClient::PlayRadio(
    const std::string& station_id
)
{
    if (audio_task_handle_ != nullptr) {
"""

new_radio = """void GeniusClient::PlayRadio(
    const std::string& station_id
)
{
    if (audio_task_handle_ != nullptr) {
"""

# Tambahkan setelah pemeriksaan task aktif
radio_anchor = """        return;
    }

    auto* args = new LocalAudioTaskArgs{
        this,
        std::string("__radio__:") + station_id,
"""

radio_replacement = """        return;
    }

    audio_stop_requested_.store(false);

    auto* args = new LocalAudioTaskArgs{
        this,
        std::string("__radio__:") + station_id,
"""

if radio_anchor not in text:
    raise RuntimeError(
        "Lokasi start radio tidak ditemukan"
    )

text = text.replace(
    radio_anchor,
    radio_replacement,
    1,
)


# Reset stop flag before starting local audio
local_anchor = """        return;
    }

    auto* args = new LocalAudioTaskArgs{
        this,
        filename,
"""

local_replacement = """        return;
    }

    audio_stop_requested_.store(false);

    auto* args = new LocalAudioTaskArgs{
        this,
        filename,
"""

if local_anchor not in text:
    raise RuntimeError(
        "Lokasi start local audio tidak ditemukan"
    )

text = text.replace(
    local_anchor,
    local_replacement,
    1,
)


old_download = """    const bool success =
        audio_client.Download(url);
"""

new_download = """    const bool success =
        audio_client.Download(
            url,
            client->audio_stop_requested_
        );
"""

if old_download not in text:
    raise RuntimeError(
        "Pemanggilan AudioStreamClient::Download tidak ditemukan"
    )

text = text.replace(
    old_download,
    new_download,
    1,
)


old_finish_log = """    ESP_LOGI(
        TAG,
        "Audio playback finished: %s",
        success ? "success" : "failed"
    );

    client->audio_task_handle_ = nullptr;
"""

new_finish_log = """    const bool was_stopped =
        client->audio_stop_requested_.load();

    ESP_LOGI(
        TAG,
        "Audio playback finished: %s",
        was_stopped
            ? "stopped"
            : (success ? "success" : "failed")
    );

    client->audio_task_handle_ = nullptr;
"""

if old_finish_log not in text:
    raise RuntimeError(
        "Log akhir AudioTaskEntry tidak ditemukan"
    )

text = text.replace(
    old_finish_log,
    new_finish_log,
    1,
)

CLIENT_CC.write_text(text, encoding="utf-8")


print()
print("PATCH TRUE MEDIA STOP BERHASIL")
print("Perubahan:")
print("- StopAudio mengirim permintaan stop")
print("- Loop HTTP keluar secara normal")
print("- HTTP ditutup sebelum task berakhir")
print("- Perintah MCP stop memakai fungsi yang sama")