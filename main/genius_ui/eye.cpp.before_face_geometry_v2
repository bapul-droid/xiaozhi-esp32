#include "eye.h"


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

void Eye::SetListening(
    bool listening
)
{
    if (
        left_eye == nullptr ||
        right_eye == nullptr
    ) {
        return;
    }

    if (listening) {
        SetClosedState(false);

        lv_obj_set_size(
            left_eye,
            45,
            45
        );

        lv_obj_set_size(
            right_eye,
            45,
            45
        );

        AlignEyes(0);
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

        AlignEyes(0);
    }
}
