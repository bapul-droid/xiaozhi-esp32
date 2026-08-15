from pathlib import Path


ROOT = Path(r"D:\xiaozhi-esp32")

CMAKE = ROOT / "main" / "CMakeLists.txt"
GENIUS_UI_CPP = ROOT / "main" / "genius_ui" / "genius_ui.cpp"

BOOT_H = ROOT / "main" / "genius_ui" / "boot_manager.h"
BOOT_CPP = ROOT / "main" / "genius_ui" / "boot_manager.cpp"


def backup(path: Path) -> None:
    backup_path = path.with_suffix(
        path.suffix + ".before_boot_manager_v01"
    )

    if path.exists() and not backup_path.exists():
        backup_path.write_text(
            path.read_text(encoding="utf-8"),
            encoding="utf-8",
        )
        print("Backup:", backup_path)


for path in [CMAKE, GENIUS_UI_CPP]:
    backup(path)


# =========================================================
# boot_manager.h
# =========================================================

BOOT_H.write_text(
r'''#pragma once

#include "lvgl.h"


class BootManager {
public:
    using FinishedCallback = void (*)(
        lv_obj_t* screen
    );

    static void Start(
        lv_obj_t* screen,
        FinishedCallback callback
    );

    static bool IsRunning();

    static void Finish();

private:
    static void OnTimer(
        lv_timer_t* timer
    );

    static void UpdateSpinner();
    static void Destroy();
};
''',
    encoding="utf-8",
)


# =========================================================
# boot_manager.cpp
# =========================================================

BOOT_CPP.write_text(
r'''#include "boot_manager.h"

#include <esp_log.h>


namespace {

constexpr const char* TAG = "BootManager";

constexpr uint32_t TIMER_PERIOD_MS = 300;
constexpr uint32_t BOOT_DURATION_MS = 2400;

lv_obj_t* boot_root = nullptr;
lv_obj_t* boot_screen = nullptr;

lv_obj_t* dot_left = nullptr;
lv_obj_t* dot_center = nullptr;
lv_obj_t* dot_right = nullptr;

lv_timer_t* boot_timer = nullptr;

BootManager::FinishedCallback finished_callback =
    nullptr;

uint32_t elapsed_ms = 0;
uint8_t spinner_index = 0;
bool running = false;


bool IsValid(lv_obj_t* object)
{
    return (
        object != nullptr &&
        lv_obj_is_valid(object)
    );
}


void ConfigureDot(
    lv_obj_t* dot,
    int x_offset
)
{
    lv_obj_remove_style_all(dot);

    lv_obj_set_size(
        dot,
        10,
        10
    );

    lv_obj_set_style_radius(
        dot,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        dot,
        lv_color_white(),
        0
    );

    lv_obj_set_style_bg_opa(
        dot,
        LV_OPA_30,
        0
    );

    lv_obj_align(
        dot,
        LV_ALIGN_CENTER,
        x_offset,
        38
    );
}

}  // namespace


void BootManager::Start(
    lv_obj_t* screen,
    FinishedCallback callback
)
{
    Destroy();

    if (
        screen == nullptr ||
        !lv_obj_is_valid(screen)
    ) {
        ESP_LOGW(
            TAG,
            "Start skipped: invalid screen"
        );

        if (callback != nullptr) {
            callback(screen);
        }

        return;
    }

    boot_screen = screen;
    finished_callback = callback;

    elapsed_ms = 0;
    spinner_index = 0;
    running = true;

    boot_root = lv_obj_create(screen);

    lv_obj_remove_style_all(boot_root);

    lv_obj_set_size(
        boot_root,
        LV_HOR_RES,
        LV_VER_RES
    );

    lv_obj_align(
        boot_root,
        LV_ALIGN_CENTER,
        0,
        0
    );

    lv_obj_set_style_bg_color(
        boot_root,
        lv_color_black(),
        0
    );

    lv_obj_set_style_bg_opa(
        boot_root,
        LV_OPA_COVER,
        0
    );

    lv_obj_remove_flag(
        boot_root,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_t* title = lv_label_create(
        boot_root
    );

    lv_label_set_text(
        title,
        "MINJI"
    );

    lv_obj_set_style_text_color(
        title,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_font(
        title,
        LV_FONT_DEFAULT
        0
    );

    lv_obj_align(
        title,
        LV_ALIGN_CENTER,
        0,
        -30
    );

    lv_obj_t* subtitle = lv_label_create(
        boot_root
    );

    lv_label_set_text(
        subtitle,
        "Memulai..."
    );

    lv_obj_set_style_text_color(
        subtitle,
        lv_color_white(),
        0
    );

    lv_obj_set_style_text_opa(
        subtitle,
        LV_OPA_70,
        0
    );

    lv_obj_align(
        subtitle,
        LV_ALIGN_CENTER,
        0,
        8
    );

    dot_left = lv_obj_create(boot_root);
    dot_center = lv_obj_create(boot_root);
    dot_right = lv_obj_create(boot_root);

    ConfigureDot(dot_left, -20);
    ConfigureDot(dot_center, 0);
    ConfigureDot(dot_right, 20);

    UpdateSpinner();

    lv_obj_move_foreground(boot_root);
    lv_obj_invalidate(boot_root);

    boot_timer = lv_timer_create(
        OnTimer,
        TIMER_PERIOD_MS,
        nullptr
    );

    ESP_LOGI(
        TAG,
        "Boot screen started"
    );
}


bool BootManager::IsRunning()
{
    return (
        running &&
        IsValid(boot_root)
    );
}


void BootManager::UpdateSpinner()
{
    lv_obj_t* dots[] = {
        dot_left,
        dot_center,
        dot_right,
    };

    for (uint8_t index = 0; index < 3; ++index) {
        if (!IsValid(dots[index])) {
            continue;
        }

        lv_obj_set_style_bg_opa(
            dots[index],
            index == spinner_index
                ? LV_OPA_COVER
                : LV_OPA_30,
            0
        );
    }

    spinner_index =
        static_cast<uint8_t>(
            (spinner_index + 1) % 3
        );
}


void BootManager::OnTimer(
    lv_timer_t* timer
)
{
    (void)timer;

    if (!IsRunning()) {
        Destroy();
        return;
    }

    elapsed_ms += TIMER_PERIOD_MS;

    UpdateSpinner();

    if (elapsed_ms >= BOOT_DURATION_MS) {
        Finish();
    }
}


void BootManager::Finish()
{
    if (!running) {
        return;
    }

    lv_obj_t* screen = boot_screen;
    FinishedCallback callback =
        finished_callback;

    Destroy();

    ESP_LOGI(
        TAG,
        "Boot screen finished"
    );

    if (
        callback != nullptr &&
        screen != nullptr &&
        lv_obj_is_valid(screen)
    ) {
        callback(screen);
    }
}


void BootManager::Destroy()
{
    running = false;

    if (boot_timer != nullptr) {
        lv_timer_delete(boot_timer);
        boot_timer = nullptr;
    }

    if (IsValid(boot_root)) {
        lv_obj_delete(boot_root);
    }

    boot_root = nullptr;
    boot_screen = nullptr;

    dot_left = nullptr;
    dot_center = nullptr;
    dot_right = nullptr;

    finished_callback = nullptr;

    elapsed_ms = 0;
    spinner_index = 0;
}
''',
    encoding="utf-8",
)


# =========================================================
# genius_ui.cpp
# =========================================================

GENIUS_UI_CPP.write_text(
r'''#include "genius_ui.h"

#include "boot_manager.h"
#include "minji_face.h"


namespace {

void StartMinjiFace(
    lv_obj_t* screen
)
{
    MinjiFace::Init(screen);
}

}  // namespace


void GeniusUI::Init(
    lv_obj_t* screen
)
{
    BootManager::Start(
        screen,
        StartMinjiFace
    );
}
''',
    encoding="utf-8",
)


# =========================================================
# CMakeLists.txt
# =========================================================

cmake = CMAKE.read_text(
    encoding="utf-8"
)

anchor = (
    '            "genius_ui/genius_ui.cpp"\n'
)

replacement = (
    '            "genius_ui/genius_ui.cpp"\n'
    '            "genius_ui/boot_manager.cpp"\n'
)

if '"genius_ui/boot_manager.cpp"' not in cmake:
    if anchor not in cmake:
        raise RuntimeError(
            "genius_ui.cpp tidak ditemukan "
            "di main/CMakeLists.txt"
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
print("PATCH BOOT MANAGER V0.1 BERHASIL")
print()
print("File baru:")
print("  main/genius_ui/boot_manager.h")
print("  main/genius_ui/boot_manager.cpp")
print()
print("Perubahan:")
print("  GeniusUI -> BootManager -> MinjiFace")
print("  Durasi boot: 2400 ms")
print("  Spinner: 3 titik LVGL ringan")