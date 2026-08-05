from pathlib import Path


ROOT = Path(r"D:\xiaozhi-esp32")
CMAKE = ROOT / "main" / "CMakeLists.txt"
LCD_CC = ROOT / "main" / "display" / "lcd_display.cc"
EYE_H = ROOT / "main" / "genius_ui" / "eye.h"
EYE_CPP = ROOT / "main" / "genius_ui" / "eye.cpp"
UI_CPP = ROOT / "main" / "genius_ui" / "genius_ui.cpp"


def make_backup(path: Path) -> None:
    backup = path.with_suffix(
        path.suffix + ".before_cute_minji"
    )

    if not backup.exists():
        backup.write_text(
            path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )

    print("Backup:", backup)


for file_path in [
    CMAKE,
    LCD_CC,
    EYE_H,
    EYE_CPP,
    UI_CPP,
]:
    make_backup(file_path)


# =========================================================
# 1. Eye API
# =========================================================

EYE_H.write_text(
r'''#pragma once

#include "lvgl.h"


class Eye {
public:
    static void Create(lv_obj_t* parent);

    static void Blink();

    static void Talk(bool enable);

    static void LookLeft();

    static void LookRight();

    static void LookCenter();

private:
    static void SetClosed(bool closed);
};
''',
    encoding="utf-8",
)


# =========================================================
# 2. Cute animated face
# =========================================================

EYE_CPP.write_text(
r'''#include "eye.h"


namespace {

constexpr uint32_t kFaceBlue = 0x42A5F5;
constexpr uint32_t kFaceBlueLight = 0x67C3FF;

constexpr int kEyeSize = 38;
constexpr int kEyeClosedHeight = 9;
constexpr int kEyeOffsetX = 29;
constexpr int kEyeOffsetY = -10;

lv_obj_t* left_eye = nullptr;
lv_obj_t* right_eye = nullptr;

lv_obj_t* left_smile = nullptr;
lv_obj_t* right_smile = nullptr;
lv_obj_t* mouth = nullptr;

lv_timer_t* blink_timer = nullptr;
lv_timer_t* gaze_timer = nullptr;

bool eyes_closed = false;
int gaze_state = 0;


void RemoveObject(lv_obj_t*& object)
{
    if (
        object != nullptr &&
        lv_obj_is_valid(object)
    ) {
        lv_obj_delete(object);
    }

    object = nullptr;
}


void ConfigureCircleEye(lv_obj_t* eye)
{
    lv_obj_remove_style_all(eye);

    lv_obj_set_size(
        eye,
        kEyeSize,
        kEyeSize
    );

    lv_obj_set_style_radius(
        eye,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        eye,
        lv_color_hex(kFaceBlue),
        0
    );

    lv_obj_set_style_bg_grad_color(
        eye,
        lv_color_hex(kFaceBlueLight),
        0
    );

    lv_obj_set_style_bg_grad_dir(
        eye,
        LV_GRAD_DIR_VER,
        0
    );

    lv_obj_set_style_bg_opa(
        eye,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        eye,
        0,
        0
    );

    lv_obj_remove_flag(
        eye,
        LV_OBJ_FLAG_SCROLLABLE
    );
}


lv_obj_t* CreateHappyArc(
    lv_obj_t* parent
)
{
    lv_obj_t* arc = lv_arc_create(parent);

    lv_obj_remove_style(
        arc,
        nullptr,
        LV_PART_KNOB
    );

    lv_obj_set_size(
        arc,
        24,
        17
    );

    lv_arc_set_bg_angles(
        arc,
        200,
        340
    );

    lv_arc_set_angles(
        arc,
        200,
        340
    );

    lv_obj_set_style_arc_opa(
        arc,
        LV_OPA_TRANSP,
        LV_PART_MAIN
    );

    lv_obj_set_style_arc_color(
        arc,
        lv_color_black(),
        LV_PART_INDICATOR
    );

    lv_obj_set_style_arc_width(
        arc,
        5,
        LV_PART_INDICATOR
    );

    lv_obj_remove_flag(
        arc,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_remove_flag(
        arc,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_center(arc);

    return arc;
}


void ConfigureMouth(
    lv_obj_t* arc
)
{
    lv_obj_remove_style(
        arc,
        nullptr,
        LV_PART_KNOB
    );

    lv_obj_set_size(
        arc,
        43,
        28
    );

    lv_arc_set_bg_angles(
        arc,
        20,
        160
    );

    lv_arc_set_angles(
        arc,
        20,
        160
    );

    lv_obj_set_style_arc_opa(
        arc,
        LV_OPA_TRANSP,
        LV_PART_MAIN
    );

    lv_obj_set_style_arc_color(
        arc,
        lv_color_hex(kFaceBlue),
        LV_PART_INDICATOR
    );

    lv_obj_set_style_arc_width(
        arc,
        5,
        LV_PART_INDICATOR
    );

    lv_obj_remove_flag(
        arc,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_remove_flag(
        arc,
        LV_OBJ_FLAG_SCROLLABLE
    );
}


void AlignEyes(
    int horizontal_shift
)
{
    if (
        left_eye == nullptr ||
        right_eye == nullptr
    ) {
        return;
    }

    lv_obj_align(
        left_eye,
        LV_ALIGN_CENTER,
        -kEyeOffsetX + horizontal_shift,
        kEyeOffsetY
    );

    lv_obj_align(
        right_eye,
        LV_ALIGN_CENTER,
        kEyeOffsetX + horizontal_shift,
        kEyeOffsetY
    );
}


void SetClosedState(
    bool closed
)
{
    if (
        left_eye == nullptr ||
        right_eye == nullptr
    ) {
        return;
    }

    eyes_closed = closed;

    if (closed) {
        lv_obj_set_height(
            left_eye,
            kEyeClosedHeight
        );

        lv_obj_set_height(
            right_eye,
            kEyeClosedHeight
        );

        if (left_smile != nullptr) {
            lv_obj_add_flag(
                left_smile,
                LV_OBJ_FLAG_HIDDEN
            );
        }

        if (right_smile != nullptr) {
            lv_obj_add_flag(
                right_smile,
                LV_OBJ_FLAG_HIDDEN
            );
        }
    } else {
        lv_obj_set_size(
            left_eye,
            kEyeSize,
            kEyeSize
        );

        lv_obj_set_size(
            right_eye,
            kEyeSize,
            kEyeSize
        );

        if (left_smile != nullptr) {
            lv_obj_remove_flag(
                left_smile,
                LV_OBJ_FLAG_HIDDEN
            );
        }

        if (right_smile != nullptr) {
            lv_obj_remove_flag(
                right_smile,
                LV_OBJ_FLAG_HIDDEN
            );
        }
    }
}


void BlinkTimerCallback(
    lv_timer_t* timer
)
{
    if (!eyes_closed) {
        SetClosedState(true);

        lv_timer_set_period(
            timer,
            170
        );
    } else {
        SetClosedState(false);

        const uint32_t next_blink =
            static_cast<uint32_t>(
                lv_rand(2400, 4800)
            );

        lv_timer_set_period(
            timer,
            next_blink
        );
    }
}


void GazeTimerCallback(
    lv_timer_t* timer
)
{
    gaze_state++;

    switch (gaze_state % 5) {
        case 1:
            AlignEyes(-4);
            break;

        case 3:
            AlignEyes(4);
            break;

        default:
            AlignEyes(0);
            break;
    }

    lv_timer_set_period(
        timer,
        static_cast<uint32_t>(
            lv_rand(900, 2200)
        )
    );
}

}  // namespace


void Eye::Create(
    lv_obj_t* parent
)
{
    if (parent == nullptr) {
        return;
    }

    RemoveObject(left_eye);
    RemoveObject(right_eye);

    left_smile = nullptr;
    right_smile = nullptr;
    mouth = nullptr;

    if (blink_timer != nullptr) {
        lv_timer_delete(blink_timer);
        blink_timer = nullptr;
    }

    if (gaze_timer != nullptr) {
        lv_timer_delete(gaze_timer);
        gaze_timer = nullptr;
    }

    left_eye = lv_obj_create(parent);
    right_eye = lv_obj_create(parent);

    ConfigureCircleEye(left_eye);
    ConfigureCircleEye(right_eye);

    AlignEyes(0);

    left_smile = CreateHappyArc(left_eye);
    right_smile = CreateHappyArc(right_eye);

    mouth = lv_arc_create(parent);
    ConfigureMouth(mouth);

    lv_obj_align(
        mouth,
        LV_ALIGN_CENTER,
        0,
        31
    );

    SetClosedState(false);

    blink_timer = lv_timer_create(
        BlinkTimerCallback,
        static_cast<uint32_t>(
            lv_rand(2400, 4800)
        ),
        nullptr
    );

    gaze_timer = lv_timer_create(
        GazeTimerCallback,
        1300,
        nullptr
    );
}


void Eye::SetClosed(
    bool closed
)
{
    SetClosedState(closed);
}


void Eye::Blink()
{
    SetClosedState(true);
}


void Eye::Talk(
    bool enable
)
{
    if (mouth == nullptr) {
        return;
    }

    lv_obj_set_style_arc_width(
        mouth,
        enable ? 8 : 5,
        LV_PART_INDICATOR
    );
}


void Eye::LookLeft()
{
    AlignEyes(-5);
}


void Eye::LookRight()
{
    AlignEyes(5);
}


void Eye::LookCenter()
{
    AlignEyes(0);
}
''',
    encoding="utf-8",
)


# =========================================================
# 3. Full-screen black face root
# =========================================================

UI_CPP.write_text(
r'''#include "genius_ui.h"
#include "eye.h"


namespace {

lv_obj_t* face_root = nullptr;

}  // namespace


void GeniusUI::Init(
    lv_obj_t* screen
)
{
    if (screen == nullptr) {
        return;
    }

    if (
        face_root != nullptr &&
        lv_obj_is_valid(face_root)
    ) {
        lv_obj_delete(face_root);
    }

    face_root = lv_obj_create(screen);

    lv_obj_remove_style_all(face_root);

    lv_obj_set_size(
        face_root,
        LV_HOR_RES,
        LV_VER_RES
    );

    lv_obj_align(
        face_root,
        LV_ALIGN_CENTER,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        face_root,
        lv_color_black(),
        0
    );

    lv_obj_set_style_bg_opa(
        face_root,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        face_root,
        0,
        0
    );

    lv_obj_set_style_pad_all(
        face_root,
        0,
        0
    );

    lv_obj_remove_flag(
        face_root,
        LV_OBJ_FLAG_SCROLLABLE
    );

    Eye::Create(face_root);

    lv_obj_move_foreground(face_root);
    lv_obj_invalidate(face_root);
}
''',
    encoding="utf-8",
)


# =========================================================
# 4. Tambahkan source ke CMake
# =========================================================

cmake = CMAKE.read_text(
    encoding="utf-8"
)

cmake_anchor = (
    '            "display/lcd_display.cc"\n'
)

cmake_replacement = (
    '            "display/lcd_display.cc"\n'
    '            "genius_ui/genius_ui.cpp"\n'
    '            "genius_ui/eye.cpp"\n'
)

if '"genius_ui/genius_ui.cpp"' not in cmake:
    if cmake_anchor not in cmake:
        raise RuntimeError(
            "lcd_display.cc tidak ditemukan "
            "di CMakeLists.txt"
        )

    cmake = cmake.replace(
        cmake_anchor,
        cmake_replacement,
        1
    )

CMAKE.write_text(
    cmake,
    encoding="utf-8"
)


# =========================================================
# 5. Include GeniusUI pada lcd_display.cc
# =========================================================

lcd = LCD_CC.read_text(
    encoding="utf-8"
)

include_line = (
    '#include "genius_ui/genius_ui.h"\n'
)

if include_line not in lcd:
    include_anchor = '#include "lcd_display.h"\n'

    if include_anchor not in lcd:
        raise RuntimeError(
            "Include lcd_display.h "
            "tidak ditemukan"
        )

    lcd = lcd.replace(
        include_anchor,
        include_anchor + include_line,
        1
    )


# =========================================================
# 6. Tambahkan Init di akhir kedua SetupUI()
# =========================================================

marker = "void LcdDisplay::SetupUI()"
position = 0
updated = 0


while True:
    start = lcd.find(marker, position)

    if start == -1:
        break

    opening = lcd.find("{", start)

    if opening == -1:
        raise RuntimeError(
            "Kurung pembuka SetupUI tidak ditemukan"
        )

    depth = 0
    index = opening

    in_string = False
    escaped = False
    in_line_comment = False
    in_block_comment = False

    while index < len(lcd):
        char = lcd[index]
        next_char = (
            lcd[index + 1]
            if index + 1 < len(lcd)
            else ""
        )

        if in_line_comment:
            if char == "\n":
                in_line_comment = False

        elif in_block_comment:
            if char == "*" and next_char == "/":
                in_block_comment = False
                index += 1

        elif in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False

        else:
            if char == "/" and next_char == "/":
                in_line_comment = True
                index += 1

            elif char == "/" and next_char == "*":
                in_block_comment = True
                index += 1

            elif char == '"':
                in_string = True

            elif char == "{":
                depth += 1

            elif char == "}":
                depth -= 1

                if depth == 0:
                    insert = (
                        "\n"
                        "    // Cute animated Minji face.\n"
                        "    GeniusUI::Init(lv_screen_active());\n"
                    )

                    nearby = lcd[
                        max(opening, index - 180):
                        index
                    ]

                    if (
                        "GeniusUI::Init("
                        not in nearby
                    ):
                        lcd = (
                            lcd[:index]
                            + insert
                            + lcd[index:]
                        )

                        index += len(insert)
                        updated += 1

                    position = index + 1
                    break

        index += 1


if updated == 0 and "GeniusUI::Init(" not in lcd:
    raise RuntimeError(
        "Tidak berhasil memasang GeniusUI "
        "ke SetupUI"
    )

LCD_CC.write_text(
    lcd,
    encoding="utf-8"
)


print()
print("PATCH WAJAH LUCU MINJI BERHASIL")
print("SetupUI diperbarui:", updated)
print("Mata, senyum, blink, dan gerak idle siap")