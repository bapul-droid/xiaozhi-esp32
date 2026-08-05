from pathlib import Path

ROOT = Path(r"D:\xiaozhi-esp32")

HEADER = ROOT / "main" / "genius_client" / "genius_client.h"
MCP_CC = ROOT / "main" / "mcp_server.cc"

header = HEADER.read_text(encoding="utf-8")
mcp = MCP_CC.read_text(encoding="utf-8")

# Backup
HEADER.with_suffix(".h.before_mcp_media").write_text(
    header,
    encoding="utf-8",
)

MCP_CC.with_suffix(".cc.before_mcp_media").write_text(
    mcp,
    encoding="utf-8",
)

# =========================================================
# 1. Tambahkan fungsi public PlayLocal()
# =========================================================

public_anchor = """    // Aman dipanggil kembali setelah Wi-Fi reconnect.
    void Start();
"""

public_replacement = """    // Aman dipanggil kembali setelah Wi-Fi reconnect.
    void Start();

    // Dipanggil oleh MCP tool untuk memutar musik lokal.
    void PlayLocal(
        const std::string& filename
    ) {
        StartLocalAudio(filename);
    }
"""

if "void PlayLocal(" not in header:
    if public_anchor not in header:
        raise RuntimeError(
            "Posisi public Start() tidak ditemukan di genius_client.h"
        )

    header = header.replace(
        public_anchor,
        public_replacement,
        1,
    )

# =========================================================
# 2. Tambahkan include GeniusClient ke mcp_server.cc
# =========================================================

include_line = '#include "genius_client/genius_client.h"'

if include_line not in mcp:
    anchor = '#include "mcp_server.h"'

    if anchor not in mcp:
        raise RuntimeError(
            "Include mcp_server.h tidak ditemukan"
        )

    mcp = mcp.replace(
        anchor,
        anchor + "\n" + include_line,
        1,
    )

# =========================================================
# 3. Tambahkan MCP tool self.media.play_local
# =========================================================

tool_code = r'''
    // GeniusAI media control
    AddUserOnlyTool(
        "self.media.play_local",
        "Play a local song on this device. "
        "Use this tool when the user asks to play or put on a song. "
        "The track parameter must be the local filename, for example Pamungkas.mp3.",
        PropertyList({
            Property(
                "track",
                kPropertyTypeString,
                "Local music filename, for example Pamungkas.mp3"
            )
        }),
        [](const PropertyList& properties) -> ReturnValue {
            const std::string track =
                properties["track"].value<std::string>();

            ESP_LOGI(
                TAG,
                "MCP play_local requested: %s",
                track.c_str()
            );

            GeniusClient::GetInstance()
                .PlayLocal(track);

            return true;
        }
    );

'''

tool_anchor = "    // Display control\n"

if '"self.media.play_local"' not in mcp:
    if tool_anchor not in mcp:
        raise RuntimeError(
            "Posisi sebelum Display control tidak ditemukan"
        )

    mcp = mcp.replace(
        tool_anchor,
        tool_code + tool_anchor,
        1,
    )

HEADER.write_text(header, encoding="utf-8")
MCP_CC.write_text(mcp, encoding="utf-8")

print("PATCH MCP PLAY_LOCAL BERHASIL")
print("Tool: self.media.play_local")
print("Backup header dan mcp_server sudah dibuat")