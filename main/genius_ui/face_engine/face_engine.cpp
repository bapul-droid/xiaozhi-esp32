#include "face_engine.h"

#include <esp_log.h>

namespace {

constexpr const char* TAG = "FaceEngine";

lv_obj_t* active_screen = nullptr;
FaceTheme* active_theme = nullptr;

FaceState active_state = FaceState::Idle;
bool initialized = false;

const char* StateName(
    FaceState state
)
{
    switch (state) {
        case FaceState::Boot:
            return "boot";

        case FaceState::Idle:
            return "idle";

        case FaceState::Blink:
            return "blink";

        case FaceState::Listening:
            return "listening";

        case FaceState::Thinking:
            return "thinking";

        case FaceState::Speaking:
            return "speaking";

        case FaceState::Happy:
            return "happy";

        case FaceState::Sleeping:
            return "sleeping";

        case FaceState::Error:
            return "error";
    }

    return "unknown";
}

}  // namespace

bool FaceEngine::Init(
    lv_obj_t* screen,
    FaceTheme* theme
)
{
    initialized = false;
    active_screen = nullptr;
    active_theme = nullptr;
    active_state = FaceState::Idle;

    if (
        screen == nullptr ||
        !lv_obj_is_valid(screen)
    ) {
        ESP_LOGE(
            TAG,
            "Initialization failed: invalid screen"
        );

        return false;
    }

    if (theme == nullptr) {
        ESP_LOGE(
            TAG,
            "Initialization failed: null theme"
        );

        return false;
    }

    if (!theme->Init(screen)) {
        ESP_LOGE(
            TAG,
            "Initialization failed: theme init failed"
        );

        return false;
    }

    active_screen = screen;
    active_theme = theme;
    initialized = true;

    active_theme->ApplyState(
        active_state
    );

    ESP_LOGI(
        TAG,
        "Initialized with state=%s",
        StateName(active_state)
    );

    return true;
}

bool FaceEngine::IsReady()
{
    return (
        initialized &&
        IsScreenValid() &&
        active_theme != nullptr &&
        active_theme->IsReady()
    );
}

bool FaceEngine::SetState(
    FaceState state
)
{
    if (!IsReady()) {
        ESP_LOGW(
            TAG,
            "SetState ignored: engine not ready"
        );

        return false;
    }

    if (state == active_state) {
        return true;
    }

    const FaceState previous_state =
        active_state;

    active_state = state;

    active_theme->ApplyState(
        active_state
    );

    ESP_LOGI(
        TAG,
        "State: %s -> %s",
        StateName(previous_state),
        StateName(active_state)
    );

    return true;
}

FaceState FaceEngine::GetState()
{
    return active_state;
}

FaceTheme* FaceEngine::GetTheme()
{
    return active_theme;
}

bool FaceEngine::IsScreenValid()
{
    return (
        active_screen != nullptr &&
        lv_obj_is_valid(active_screen)
    );
}
