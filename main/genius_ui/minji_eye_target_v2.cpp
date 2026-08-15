#include "eye.h"


namespace {

constexpr uint32_t kEyeBlue = 0x42A5F5;
constexpr uint32_t kEyeBlueLight = 0x86D7FF;
constexpr uint32_t kPupilColor = 0x071521;

// Geometry tuned for a 128x160 LCD.
constexpr int kEyeWidth = 40;
constexpr int kEyeHeight = 44;
constexpr int kEyeClosedHeight = 6;
constexpr int kEyeRadius = 18;

constexpr int kEyeOffsetX = 28;
constexpr int kEyeOffsetY = -12;

constexpr int kListeningEyeWidth = 44;
constexpr int kListeningEyeHeight = 48;

constexpr int kPupilSize = 15;
constexpr int kPupilCenterY = 1;
constexpr int kPupilMoveX = 6;
constexpr int kPupilMoveY = 2;

constexpr int kHighlightSize = 4;
constexpr int kHighlightOffsetX = -4;
constexpr int kHighlightOffsetY = -4;

constexpr int kMouthWidth = 40;
constexpr int kMouthHeight = 25;
constexpr int kMouthOffsetY = 31;
constexpr int kMouthThickness = 4;

lv_obj_t* left_eye = nullptr;
lv_obj_t* right_eye = nullptr;
lv_obj_t* left_pupil = nullptr;
lv_obj_t* right_pupil = nullptr;
lv_obj_t* left_highlight = nullptr;
lv_obj_t* right_highlight = nullptr;
lv_obj_t* mouth = nullptr;

lv_timer_t* blink_timer = nullptr;
lv_timer_t* gaze_timer = nullptr;

bool eyes_closed = false;
int gaze_state = 0;


bool IsValid(lv_obj_t* object)
{
    return object != nullptr && lv_obj_is_valid(object);
}


void RemoveObject(lv_obj_t*& object)
{
    if (IsValid(object)) {
        lv_obj_delete(object);
    }

    object = nullptr;
}


void ConfigureEye(lv_obj_t* eye)
{
    lv_obj_remove_style_all(eye);

    lv_obj_set_size(
        eye,
        kEyeWidth,
        kEyeHeight
    );

    lv_obj_set_style_radius(
        eye,
        kEyeRadius,
        0
    );

    lv_obj_set_style_bg_color(
        eye,
        lv_color_hex(kEyeBlue),
        0
    );

    lv_obj_set_style_bg_grad_color(
        eye,
        lv_color_hex(kEyeBlueLight),
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

    lv_obj_remove_flag(
        eye,
        LV_OBJ_FLAG_CLICKABLE
    );
}


void ConfigurePupil(lv_obj_t* pupil)
{
    lv_obj_remove_style_all(pupil);

    lv_obj_set_size(
        pupil,
        kPupilSize,
        kPupilSize
    );

    lv_obj_set_style_radius(
        pupil,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        pupil,
        lv_color_hex(kPupilColor),
        0
    );

    lv_obj_set_style_bg_opa(
        pupil,
        LV_OPA_COVER,
        0
    );

    lv_obj_remove_flag(
        pupil,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_remove_flag(
        pupil,
        LV_OBJ_FLAG_CLICKABLE
    );
}


void ConfigureHighlight(lv_obj_t* highlight)
{
    lv_obj_remove_style_all(highlight);

    lv_obj_set_size(
        highlight,
        kHighlightSize,
        kHighlightSize
    );

    lv_obj_set_style_radius(
        highlight,
        LV_RADIUS_CIRCLE,
        0
    );

    lv_obj_set_style_bg_color(
        highlight,
        lv_color_white(),
        0
    );

    lv_obj_set_style_bg_opa(
        highlight,
        LV_OPA_COVER,
        0
    );

    lv_obj_remove_flag(
        highlight,
        LV_OBJ_FLAG_SCROLLABLE
    );

    lv_obj_remove_flag(
        highlight,
        LV_OBJ_FLAG_CLICKABLE
    );

    lv_obj_align(
        highlight,
        LV_ALIGN_CENTER,
        kHighlightOffsetX,
        kHighlightOffsetY
    );
}


void ConfigureMouth(lv_obj_t* arc)
{
    lv_obj_remove_style(
        arc,
        nullptr,
        LV_PART_KNOB
    );

    lv_obj_set_size(
        arc,
        kMouthWidth,
        kMouthHeight
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
        lv_color_hex(kEyeBlue),
        LV_PART_INDICATOR
    );

    lv_obj_set_style_arc_width(
        arc,
        kMouthThickness,
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


void AlignEyes()
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    lv_obj_align(
        left_eye,
        LV_ALIGN_CENTER,
        -kEyeOffsetX,
        kEyeOffsetY
    );

    lv_obj_align(
        right_eye,
        LV_ALIGN_CENTER,
        kEyeOffsetX,
        kEyeOffsetY
    );
}


void AlignPupils(int horizontal_shift, int vertical_shift = 0)
{
    if (!IsValid(left_pupil) || !IsValid(right_pupil)) {
        return;
    }

    lv_obj_align(
        left_pupil,
        LV_ALIGN_CENTER,
        horizontal_shift,
        kPupilCenterY + vertical_shift
    );

    lv_obj_align(
        right_pupil,
        LV_ALIGN_CENTER,
        horizontal_shift,
        kPupilCenterY + vertical_shift
    );
}


void SetEyeChildrenVisible(bool visible)
{
    lv_obj_t* objects[] = {
        left_pupil,
        right_pupil,
    };

    for (lv_obj_t* object : objects) {
        if (!IsValid(object)) {
            continue;
        }

        if (visible) {
            lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
        }
    }
}


void RestoreOpenEyeGeometry()
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    lv_obj_set_size(left_eye, kEyeWidth, kEyeHeight);
    lv_obj_set_size(right_eye, kEyeWidth, kEyeHeight);

    lv_obj_set_style_radius(left_eye, kEyeRadius, 0);
    lv_obj_set_style_radius(right_eye, kEyeRadius, 0);

    AlignEyes();
}


void SetClosedState(bool closed)
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    eyes_closed = closed;

    if (closed) {
        SetEyeChildrenVisible(false);

        lv_obj_set_size(left_eye, kEyeWidth, kEyeClosedHeight);
        lv_obj_set_size(right_eye, kEyeWidth, kEyeClosedHeight);

        lv_obj_set_style_radius(left_eye, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_radius(right_eye, LV_RADIUS_CIRCLE, 0);

        AlignEyes();
        return;
    }

    RestoreOpenEyeGeometry();
    SetEyeChildrenVisible(true);
    AlignPupils(0);
}


void BlinkTimerCallback(lv_timer_t* timer)
{
    if (!eyes_closed) {
        SetClosedState(true);
        lv_timer_set_period(timer, 150);
        return;
    }

    SetClosedState(false);

    lv_timer_set_period(
        timer,
        static_cast<uint32_t>(
            lv_rand(2800, 5600)
        )
    );
}


void GazeTimerCallback(lv_timer_t* timer)
{
    if (eyes_closed) {
        return;
    }

    gaze_state++;

    switch (gaze_state % 7) {
        case 1:
            AlignPupils(-kPupilMoveX);
            break;

        case 3:
            AlignPupils(kPupilMoveX);
            break;

        case 5:
            AlignPupils(0, -kPupilMoveY);
            break;

        default:
            AlignPupils(0);
            break;
    }

    lv_timer_set_period(
        timer,
        static_cast<uint32_t>(
            lv_rand(1400, 3000)
        )
    );
}

}  // namespace


void Eye::Create(lv_obj_t* parent)
{
    if (parent == nullptr || !lv_obj_is_valid(parent)) {
        return;
    }

    if (blink_timer != nullptr) {
        lv_timer_delete(blink_timer);
        blink_timer = nullptr;
    }

    if (gaze_timer != nullptr) {
        lv_timer_delete(gaze_timer);
        gaze_timer = nullptr;
    }

    RemoveObject(left_eye);
    RemoveObject(right_eye);
    RemoveObject(mouth);

    left_pupil = nullptr;
    right_pupil = nullptr;
    left_highlight = nullptr;
    right_highlight = nullptr;

    left_eye = lv_obj_create(parent);
    right_eye = lv_obj_create(parent);

    ConfigureEye(left_eye);
    ConfigureEye(right_eye);
    AlignEyes();

    left_pupil = lv_obj_create(left_eye);
    right_pupil = lv_obj_create(right_eye);

    ConfigurePupil(left_pupil);
    ConfigurePupil(right_pupil);
    AlignPupils(0);

    left_highlight = lv_obj_create(left_pupil);
    right_highlight = lv_obj_create(right_pupil);

    ConfigureHighlight(left_highlight);
    ConfigureHighlight(right_highlight);

    mouth = lv_arc_create(parent);
    ConfigureMouth(mouth);

    lv_obj_align(
        mouth,
        LV_ALIGN_CENTER,
        0,
        kMouthOffsetY
    );

    eyes_closed = false;
    gaze_state = 0;

    blink_timer = lv_timer_create(
        BlinkTimerCallback,
        static_cast<uint32_t>(
            lv_rand(2800, 5600)
        ),
        nullptr
    );

    gaze_timer = lv_timer_create(
        GazeTimerCallback,
        1800,
        nullptr
    );
}


void Eye::SetClosed(bool closed)
{
    SetClosedState(closed);
}


void Eye::Blink()
{
    SetClosedState(true);
}


void Eye::Talk(bool enable)
{
    if (!IsValid(mouth)) {
        return;
    }

    lv_obj_set_style_arc_width(
        mouth,
        enable
            ? kMouthThickness + 3
            : kMouthThickness,
        LV_PART_INDICATOR
    );
}


void Eye::LookLeft()
{
    AlignPupils(-kPupilMoveX);
}


void Eye::LookRight()
{
    AlignPupils(kPupilMoveX);
}


void Eye::LookCenter()
{
    AlignPupils(0);
}


void Eye::SetListening(bool listening)
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    SetClosedState(false);

    if (listening) {
        lv_obj_set_size(
            left_eye,
            kListeningEyeWidth,
            kListeningEyeHeight
        );

        lv_obj_set_size(
            right_eye,
            kListeningEyeWidth,
            kListeningEyeHeight
        );

        lv_obj_set_style_radius(
            left_eye,
            kEyeRadius + 2,
            0
        );

        lv_obj_set_style_radius(
            right_eye,
            kEyeRadius + 2,
            0
        );

        AlignEyes();
        AlignPupils(0);
        return;
    }

    RestoreOpenEyeGeometry();
    AlignPupils(0);
}
