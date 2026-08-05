from pathlib import Path


ROOT = Path(r"D:\xiaozhi-esp32")
CMAKE = ROOT / "main" / "CMakeLists.txt"
LCD_CC = ROOT / "main" / "display" / "lcd_display.cc"


def backup(path: Path) -> None:
    backup_path = path.with_suffix(
        path.suffix + ".before_minji_eyes"
    )

    if not backup_path.exists():
        backup_path.write_text(
            path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )

    print(f"Backup: {backup_path}")


def insert_before_function_closing_brace(
    source: str,
    marker: str,
    insertion: str,
    start_position: int,
) -> tuple[str, int]:
    function_start = source.find(
        marker,
        start_position,
    )

    if function_start == -1:
        return source, -1

    opening_brace = source.find(
        "{",
        function_start,
    )

    if opening_brace == -1:
        raise RuntimeError(
            f"Kurung pembuka fungsi tidak ditemukan: {marker}"
        )

    depth = 0
    index = opening_brace
    in_string = False
    in_char = False
    escaped = False
    in_line_comment = False
    in_block_comment = False

    while index < len(source):
        character = source[index]
        next_character = (
            source[index + 1]
            if index + 1 < len(source)
            else ""
        )

        if in_line_comment:
            if character == "\n":
                in_line_comment = False

        elif in_block_comment:
            if (
                character == "*"
                and next_character == "/"
            ):
                in_block_comment = False
                index += 1

        elif in_string:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                in_string = False

        elif in_char:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == "'":
                in_char = False

        else:
            if (
                character == "/"
                and next_character == "/"
            ):
                in_line_comment = True
                index += 1

            elif (
                character == "/"
                and next_character == "*"
            ):
                in_block_comment = True
                index += 1

            elif character == '"':
                in_string = True

            elif character == "'":
                in_char = True

            elif character == "{":
                depth += 1

            elif character == "}":
                depth -= 1

                if depth == 0:
                    before = source[:index]
                    after = source[index:]

                    if insertion.strip() not in before[
                        max(opening_brace, index - 300):
                        index
                    ]:
                        source = (
                            before
                            + insertion
                            + after
                        )
                        index += len(insertion)

                    return source, index + 1

        index += 1

    raise RuntimeError(
        f"Akhir fungsi tidak ditemukan: {marker}"
    )


backup(CMAKE)
backup(LCD_CC)


# =========================================================
# 1. Tambahkan source Genius UI ke CMake
# =========================================================

cmake_source = CMAKE.read_text(
    encoding="utf-8"
)

cmake_anchor = '''            "display/lcd_display.cc"
'''

cmake_replacement = '''            "display/lcd_display.cc"
            "genius_ui/genius_ui.cpp"
            "genius_ui/eye.cpp"
'''

if '"genius_ui/genius_ui.cpp"' not in cmake_source:
    if cmake_anchor not in cmake_source:
        raise RuntimeError(
            "Posisi display/lcd_display.cc "
            "tidak ditemukan di CMakeLists.txt"
        )

    cmake_source = cmake_source.replace(
        cmake_anchor,
        cmake_replacement,
        1,
    )

CMAKE.write_text(
    cmake_source,
    encoding="utf-8",
)


# =========================================================
# 2. Tambahkan include GeniusUI
# =========================================================

lcd_source = LCD_CC.read_text(
    encoding="utf-8"
)

include_line = (
    '#include "genius_ui/genius_ui.h"\n'
)

if include_line not in lcd_source:
    include_anchor = '#include "lcd_display.h"\n'

    if include_anchor not in lcd_source:
        raise RuntimeError(
            "Include lcd_display.h tidak ditemukan"
        )

    lcd_source = lcd_source.replace(
        include_anchor,
        include_anchor + include_line,
        1,
    )


# =========================================================
# 3. Aktifkan GeniusUI pada semua versi SetupUI
# =========================================================

setup_marker = "void LcdDisplay::SetupUI()"
insertion = (
    "\n"
    "    // Minji animated eyes overlay.\n"
    "    GeniusUI::Init(screen);\n"
)

search_position = 0
setup_count = 0

while True:
    lcd_source, next_position = (
        insert_before_function_closing_brace(
            lcd_source,
            setup_marker,
            insertion,
            search_position,
        )
    )

    if next_position == -1:
        break

    setup_count += 1
    search_position = next_position

if setup_count == 0:
    raise RuntimeError(
        "Fungsi LcdDisplay::SetupUI tidak ditemukan"
    )

LCD_CC.write_text(
    lcd_source,
    encoding="utf-8",
)

print()
print("PATCH MATA MINJI BERHASIL")
print(
    "SetupUI yang diperbarui:",
    setup_count,
)
print("Source GeniusUI sudah masuk CMake")
print("GeniusUI sudah diaktifkan pada LCD")