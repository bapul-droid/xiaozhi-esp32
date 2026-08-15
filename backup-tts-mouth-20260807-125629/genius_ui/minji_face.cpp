#include "minji_face.h"

#include "eye.h"

#include <esp_log.h>


namespace {

constexpr const char* TAG = "MinjiFace";

lv_obj_t* face_root = nullptr;

MinjiFace::Emotion current_emotion =
    MinjiFace::Emotion::Idle;

bool initialized = false;
bool speaking_active = false;

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
    speaking_active = false;
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

    switch (emotion) {
        case Emotion::Listening:
            Eye::Talk(false);
            Eye::SetListening(true);
            Eye::LookCenter();
            break;

        case Emotion::Speaking:
            Eye::SetListening(false);
            Eye::Talk(true);
            break;

        case Emotion::Sleep:
            Eye::Talk(false);
            Eye::SetListening(false);
            Eye::Blink();
            break;

        case Emotion::Thinking:
            Eye::Talk(false);
            Eye::SetListening(false);
            break;

        case Emotion::Happy:
            Eye::Talk(false);
            Eye::SetListening(false);
            break;

        case Emotion::Error:
            Eye::Talk(false);
            Eye::SetListening(false);
            break;

        case Emotion::Idle:
        default:
            Eye::Talk(false);
            Eye::SetListening(false);
            Eye::LookCenter();
            break;
    }

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
