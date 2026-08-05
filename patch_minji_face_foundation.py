from pathlib import Path


ROOT = Path(r"D:\xiaozhi-esp32")

CMAKE = ROOT / "main" / "CMakeLists.txt"
GENIUS_UI_H = ROOT / "main" / "genius_ui" / "genius_ui.h"
GENIUS_UI_CPP = ROOT / "main" / "genius_ui" / "genius_ui.cpp"

MINJI_FACE_H = (
    ROOT / "main" / "genius_ui" / "minji_face.h"
)

MINJI_FACE_CPP = (
    ROOT / "main" / "genius_ui" / "minji_face.cpp"
)


def backup(path: Path) -> None:
    if not path.exists():
        return

    backup_path = path.with_suffix(
        path.suffix + ".before_minji_face"
    )

    if not backup_path.exists():
        backup_path.write_text(
            path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )

        print("Backup:", backup_path)


for path in [
    CMAKE,
    GENIUS_UI_H,
    GENIUS_UI_CPP,
]:
    backup(path)


# =========================================================
# 1. minji_face.h
# =========================================================

MINJI_FACE_H.write_text(
r'''#pragma once

#include "lvgl.h"


class MinjiFace {
public:
    enum class Emotion {
        Idle,
        Listening,
        Thinking,
        Speaking,
        Happy,
        Sleep,
        Error,
    };

    static void Init(
        lv_obj_t* screen
    );

    static bool IsReady();

    static void SetEmotion(
        Emotion emotion
    );

    static Emotion GetEmotion();

private:
    static bool IsScreenValid();
};
''',
    encoding="utf-8",
)


# =========================================================
# 2. minji_face.cpp
# =========================================================

MINJI_FACE_CPP.write_text(
r'''#include "minji_face.h"

#include "eye.h"

#include <esp_log.h>


namespace {

constexpr const char* TAG = "MinjiFace";

lv_obj_t* face_root = nullptr;

MinjiFace::Emotion current_emotion =
    MinjiFace::Emotion::Idle;

bool initialized = false;

}  // namespace


bool MinjiFace::IsScreenValid()
{
    return (
        face_root != nullptr &&
        lv_obj_is_valid(face_root)
    );
}


void MinjiFace::Init(
    lv_obj_t* screen
)
{
    initialized = false;

    if (
        screen == nullptr ||
        !lv_obj_is_valid(screen)
    ) {
        ESP_LOGW(
            TAG,
            "Init skipped: invalid screen"
        );

        return;
    }

    if (IsScreenValid()) {
        lv_obj_delete(face_root);
        face_root = nullptr;
    }

    face_root = lv_obj_create(screen);

    if (face_root == nullptr) {
        ESP_LOGE(
            TAG,
            "Failed to create face root"
        );

        return;
    }

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

    current_emotion = Emotion::Idle;
    initialized = true;

    ESP_LOGI(
        TAG,
        "Face engine initialized"
    );
}


bool MinjiFace::IsReady()
{
    return (
        initialized &&
        IsScreenValid()
    );
}


void MinjiFace::SetEmotion(
    Emotion emotion
)
{
    if (!IsReady()) {
        ESP_LOGW(
            TAG,
            "SetEmotion ignored: face not ready"
        );

        return;
    }

    current_emotion = emotion;

    // Tahap foundation:
    // belum mengubah bentuk mata.
    // Ini sengaja agar aman sebelum
    // emotion engine dihubungkan.

    ESP_LOGI(
        TAG,
        "Emotion changed: %d",
        static_cast<int>(emotion)
    );
}


MinjiFace::Emotion MinjiFace::GetEmotion()
{
    return current_emotion;
}
''',
    encoding="utf-8",
)


# =========================================================
# 3. GeniusUI menjadi bootstrap
# =========================================================

GENIUS_UI_H.write_text(
r'''#pragma once

#include "lvgl.h"


class GeniusUI {
public:
    static void Init(
        lv_obj_t* screen
    );
};
''',
    encoding="utf-8",
)


GENIUS_UI_CPP.write_text(
r'''#include "genius_ui.h"

#include "minji_face.h"


void GeniusUI::Init(
    lv_obj_t* screen
)
{
    MinjiFace::Init(screen);
}
''',
    encoding="utf-8",
)


# =========================================================
# 4. Tambahkan minji_face.cpp ke CMake
# =========================================================

cmake = CMAKE.read_text(
    encoding="utf-8"
)

anchor = (
    '            "genius_ui/genius_ui.cpp"\n'
)

replacement = (
    '            "genius_ui/genius_ui.cpp"\n'
    '            "genius_ui/minji_face.cpp"\n'
)

if '"genius_ui/minji_face.cpp"' not in cmake:
    if anchor not in cmake:
        raise RuntimeError(
            "genius_ui.cpp tidak ditemukan "
            "di CMakeLists.txt"
        )

    cmake = cmake.replace(
        anchor,
        replacement,
        1,
    )

CMAKE.write_text(
    cmake,
    encoding="utf-8",
)


print()
print("PATCH MINJIFACE FOUNDATION BERHASIL")
print("File dibuat:")
print("  main/genius_ui/minji_face.h")
print("  main/genius_ui/minji_face.cpp")
print()
print("Belum ada perubahan application.cc")
print("Risiko crash state/LVGL tetap dihindari")