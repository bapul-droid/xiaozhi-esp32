from pathlib import Path

root = Path(r"D:\xiaozhi-esp32")
header_path = root / "main" / "genius_client" / "genius_client.h"
source_path = root / "main" / "genius_client" / "genius_client.cc"
mcp_path = root / "main" / "mcp_server.cc"

header = header_path.read_text(encoding="utf-8")
source = source_path.read_text(encoding="utf-8")
mcp = mcp_path.read_text(encoding="utf-8")

header_path.with_suffix(".h.before_stop_radio").write_text(header, encoding="utf-8")
source_path.with_suffix(".cc.before_stop_radio").write_text(source, encoding="utf-8")
mcp_path.with_suffix(".cc.before_stop_radio").write_text(mcp, encoding="utf-8")

# Tambahkan fungsi public ke GeniusClient.
anchor = """    void PlayLocal(
        const std::string& filename
    ) {
"""

if "void StopAudio();" not in header:
    pos = header.find(anchor)
    if pos == -1:
        raise RuntimeError("Fungsi PlayLocal tidak ditemukan di genius_client.h")

    end = header.find("\n    }", pos)
    if end == -1:
        raise RuntimeError("Akhir fungsi PlayLocal tidak ditemukan")

    end += len("\n    }")

    public_code = """

    void StopAudio();

    void PlayRadio(
        const std::string& station_id
    );
"""

    header = header[:end] + public_code + header[end:]

# Tambahkan implementasi.
if "void GeniusClient::StopAudio()" not in source:
    implementation = r'''

void GeniusClient::StopAudio()
{
    Application::GetInstance()
        .GetAudioService()
        .ResetDecoder();

    ESP_LOGI(TAG, "Audio stopped by MCP");
}


void GeniusClient::PlayRadio(
    const std::string& station_id
)
{
    if (audio_task_handle_ != nullptr) {
        ESP_LOGW(TAG, "Audio playback is already running");
        return;
    }

    auto* args = new LocalAudioTaskArgs{
        this,
        std::string("__radio__:") + station_id,
    };

    BaseType_t result = xTaskCreate(
        AudioTaskEntry,
        "genius_audio",
        12288,
        args,
        3,
        &audio_task_handle_
    );

    if (result != pdPASS) {
        audio_task_handle_ = nullptr;
        delete args;
        ESP_LOGE(TAG, "Failed to create radio playback task");
    }
}
'''

    marker = "void GeniusClient::StartLocalAudio("
    index = source.find(marker)
    if index == -1:
        raise RuntimeError("StartLocalAudio tidak ditemukan")

    source = source[:index] + implementation + "\n" + source[index:]

# Ubah AudioTaskEntry agar mendukung radio.
old_url = """    const std::string url =
        std::string(GENIUS_SERVER_URL) +
        "/api/audio/local/" +
        filename;
"""

new_url = """    std::string url;

    const std::string radio_prefix = "__radio__:";

    if (filename.rfind(radio_prefix, 0) == 0) {
        const std::string station_id =
            filename.substr(radio_prefix.size());

        url =
            std::string(GENIUS_SERVER_URL) +
            "/api/radio-stream/" +
            station_id;
    } else {
        url =
            std::string(GENIUS_SERVER_URL) +
            "/api/audio/local/" +
            filename;
    }
"""

if old_url in source:
    source = source.replace(old_url, new_url, 1)
elif new_url not in source:
    raise RuntimeError("Bagian URL AudioTaskEntry tidak ditemukan")

# Tambahkan MCP tools.
tool_anchor = "    // Display control\n"

tools = r'''
    AddTool(
        "self.media.stop",
        "Stop the currently playing song or radio on this device. "
        "Use this tool when the user says stop, berhenti, matikan musik, or matikan radio.",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            ESP_LOGI(TAG, "MCP stop requested");

            GeniusClient::GetInstance()
                .StopAudio();

            return true;
        }
    );

    AddTool(
        "self.media.play_radio",
        "Play an internet radio station on this device. "
        "Use this tool when the user asks to play or turn on a radio station.",
        PropertyList({
            Property(
                "station_id",
                kPropertyTypeString,
                "Radio station identifier, for example prambors"
            )
        }),
        [](const PropertyList& properties) -> ReturnValue {
            const std::string station_id =
                properties["station_id"].value<std::string>();

            ESP_LOGI(
                TAG,
                "MCP play_radio requested: %s",
                station_id.c_str()
            );

            GeniusClient::GetInstance()
                .PlayRadio(station_id);

            return true;
        }
    );

'''

if '"self.media.stop"' not in mcp:
    if tool_anchor not in mcp:
        raise RuntimeError("Posisi MCP tool tidak ditemukan")

    mcp = mcp.replace(
        tool_anchor,
        tools + tool_anchor,
        1,
    )

header_path.write_text(header, encoding="utf-8")
source_path.write_text(source, encoding="utf-8")
mcp_path.write_text(mcp, encoding="utf-8")

print("PATCH STOP DAN RADIO BERHASIL")
print("Tool: self.media.stop")
print("Tool: self.media.play_radio")