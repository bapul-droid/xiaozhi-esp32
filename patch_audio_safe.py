from pathlib import Path


ROOT = Path(r"D:\xiaozhi-esp32")

GENIUS_CC = ROOT / "main" / "genius_client" / "genius_client.cc"
AUDIO_CC = ROOT / "main" / "genius_client" / "audio_stream_client.cc"


def backup(path: Path) -> None:
    backup_path = path.with_suffix(path.suffix + ".before_audio_safe")
    backup_path.write_text(
        path.read_text(encoding="utf-8"),
        encoding="utf-8",
    )
    print(f"Backup: {backup_path}")


backup(GENIUS_CC)
backup(AUDIO_CC)


# =========================================================
# 1. Ganti penuh audio_stream_client.cc
# =========================================================

audio_source = r'''#include "audio_stream_client.h"

#include "application.h"
#include "boards/common/board.h"
#include "audio/demuxer/ogg_demuxer.h"
#include "protocols/protocol.h"

#include <esp_log.h>

#include <array>
#include <cstdint>
#include <memory>


namespace {

constexpr const char* TAG = "AudioStream";
constexpr size_t READ_BUFFER_SIZE = 2048;

// Kanal HTTP khusus audio.
// Kanal 3 tetap dipakai register, heartbeat, dan command.
constexpr int AUDIO_HTTP_CONNECTION_ID = 1;

}  // namespace


bool AudioStreamClient::Download(
    const std::string& url
) {
    ESP_LOGI(TAG, "Preparing audio HTTP client");

    auto network =
        Board::GetInstance().GetNetwork();

    if (network == nullptr) {
        ESP_LOGE(
            TAG,
            "Network interface unavailable"
        );
        return false;
    }

    auto http =
        network->CreateHttp(
            AUDIO_HTTP_CONNECTION_ID
        );

    if (http == nullptr) {
        ESP_LOGE(
            TAG,
            "Failed to create audio HTTP client"
        );
        return false;
    }

    http->SetTimeout(30000);
    http->SetKeepAlive(false);
    http->SetHeader("Accept", "audio/ogg");
    http->SetHeader(
        "Connection",
        "close"
    );
    http->SetHeader(
        "User-Agent",
        "GeniusAI-ESP32/0.1"
    );

    ESP_LOGI(
        TAG,
        "Opening audio stream: %s",
        url.c_str()
    );

    if (!http->Open("GET", url)) {
        ESP_LOGE(
            TAG,
            "Failed to open audio stream, error=0x%x",
            http->GetLastError()
        );
        http->Close();
        return false;
    }

    const int status_code =
        http->GetStatusCode();

    ESP_LOGI(
        TAG,
        "Audio HTTP status: %d",
        status_code
    );

    if (status_code != 200) {
        const std::string response =
            http->ReadAll();

        ESP_LOGE(
            TAG,
            "Audio stream HTTP %d: %s",
            status_code,
            response.c_str()
        );

        http->Close();
        return false;
    }

    const std::string content_type =
        http->GetResponseHeader(
            "Content-Type"
        );

    ESP_LOGI(
        TAG,
        "Stream opened, content-type=%s",
        content_type.c_str()
    );

    auto& audio_service =
        Application::GetInstance()
            .GetAudioService();

    audio_service.ResetDecoder();

    OggDemuxer demuxer;

    uint32_t timestamp = 0;
    size_t opus_packet_count = 0;
    bool queue_error = false;

    demuxer.OnDemuxerFinished(
        [
            &audio_service,
            &timestamp,
            &opus_packet_count,
            &queue_error
        ](
            const uint8_t* data,
            int sample_rate,
            size_t length
        ) {
            if (
                data == nullptr ||
                length == 0 ||
                queue_error
            ) {
                return;
            }

            auto packet =
                std::make_unique<AudioStreamPacket>();

            packet->sample_rate = sample_rate;
            packet->frame_duration =
                OPUS_FRAME_DURATION_MS;
            packet->timestamp = timestamp;

            packet->payload.assign(
                data,
                data + length
            );

            timestamp +=
                OPUS_FRAME_DURATION_MS;

            if (
                !audio_service.PushPacketToDecodeQueue(
                    std::move(packet),
                    true
                )
            ) {
                ESP_LOGE(
                    TAG,
                    "Failed to queue Opus packet"
                );

                queue_error = true;
                return;
            }

            opus_packet_count++;

            if (opus_packet_count % 20 == 0) {
                ESP_LOGI(
                    TAG,
                    "Queued %u Opus packets",
                    static_cast<unsigned>(
                        opus_packet_count
                    )
                );
            }
        }
    );

    std::array<uint8_t, READ_BUFFER_SIZE> buffer{};
    size_t total_bytes = 0;

    while (!queue_error) {
        const int bytes_read =
            http->Read(
                reinterpret_cast<char*>(
                    buffer.data()
                ),
                buffer.size()
            );

        if (bytes_read < 0) {
            ESP_LOGE(
                TAG,
                "Audio read failed after %u bytes",
                static_cast<unsigned>(
                    total_bytes
                )
            );

            http->Close();
            return false;
        }

        if (bytes_read == 0) {
            break;
        }

        total_bytes +=
            static_cast<size_t>(bytes_read);

        size_t offset = 0;

        while (
            offset <
            static_cast<size_t>(bytes_read)
        ) {
            const size_t processed =
                demuxer.Process(
                    buffer.data() + offset,
                    static_cast<size_t>(
                        bytes_read
                    ) - offset
                );

            if (processed == 0) {
                ESP_LOGE(
                    TAG,
                    "Ogg demuxer made no progress"
                );

                queue_error = true;
                break;
            }

            offset += processed;
        }
    }

    http->Close();

    ESP_LOGI(
        TAG,
        "Audio stream finished: "
        "%u bytes, %u Opus packets",
        static_cast<unsigned>(total_bytes),
        static_cast<unsigned>(
            opus_packet_count
        )
    );

    return (
        !queue_error &&
        opus_packet_count > 0
    );
}
'''

AUDIO_CC.write_text(
    audio_source,
    encoding="utf-8",
)


# =========================================================
# 2. Hentikan polling command selama audio berjalan
# =========================================================

genius_source = GENIUS_CC.read_text(
    encoding="utf-8"
)

old_poll = """        FetchNextCommand();

        vTaskDelay(
"""

new_poll = """        if (audio_task_handle_ == nullptr) {
            FetchNextCommand();
        }

        vTaskDelay(
"""

if old_poll in genius_source:
    genius_source = genius_source.replace(
        old_poll,
        new_poll,
        1,
    )
elif new_poll not in genius_source:
    raise RuntimeError(
        "Lokasi FetchNextCommand() tidak ditemukan"
    )


# =========================================================
# 3. Perbesar stack task audio
# =========================================================

genius_source = genius_source.replace(
    '''        "genius_audio",
        8192,
''',
    '''        "genius_audio",
        12288,
''',
    1,
)

GENIUS_CC.write_text(
    genius_source,
    encoding="utf-8",
)

print()
print