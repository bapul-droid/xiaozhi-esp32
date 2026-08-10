#include "eye.h"
#include <cstdio>

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
constexpr int kMouthSmallWidth = 15;
constexpr int kMouthSmallHeight = 13;
constexpr int kMouthMediumWidth = 22;
constexpr int kMouthMediumHeight = 18;
constexpr uint32_t kMouthFramePeriodMs = 160;

// Short organic happy/laugh burst.  This is intentionally lightweight:
// only geometry changes; no bitmap animation is used.
constexpr int kHappyEyeWidth = 42;
constexpr int kHappyEyeHeightSoft = 30;
constexpr int kHappyEyeHeightSquint = 12;
constexpr int kHappyEyeInsetX = 2;
constexpr int kHappyBounceY = 5;
constexpr uint32_t kHappyFramePeriodMs = 110;
constexpr int kHappyFrameCount = 10;

lv_obj_t* left_eye = nullptr;
lv_obj_t* right_eye = nullptr;
lv_obj_t* left_pupil = nullptr;
lv_obj_t* right_pupil = nullptr;
lv_obj_t* left_highlight = nullptr;
lv_obj_t* right_highlight = nullptr;
lv_obj_t* mouth = nullptr;

lv_timer_t* blink_timer = nullptr;
lv_timer_t* gaze_timer = nullptr;
lv_timer_t* mouth_timer = nullptr;
lv_timer_t* happy_timer = nullptr;

bool eyes_closed = false;
bool mouth_talking = false;
bool happy_active = false;
int gaze_state = 0;
int mouth_frame = 0;
int happy_frame = 0;


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
    LV_RADIUS_CIRCLE,
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


void SetMouthSmile()
{
    if (!IsValid(mouth)) {
        return;
    }

    lv_obj_set_size(
        mouth,
        kMouthWidth,
        kMouthHeight
    );

    lv_arc_set_bg_angles(
        mouth,
        20,
        160
    );

    lv_arc_set_angles(
        mouth,
        20,
        160
    );

    lv_obj_set_style_arc_width(
        mouth,
        kMouthThickness,
        LV_PART_INDICATOR
    );

    lv_obj_align(
        mouth,
        LV_ALIGN_CENTER,
        0,
        kMouthOffsetY
    );
}


void SetMouthOpen(
    int width,
    int height
)
{
    if (!IsValid(mouth)) {
        return;
    }

    lv_obj_set_size(
        mouth,
        width,
        height
    );

    lv_arc_set_bg_angles(
        mouth,
        0,
        360
    );

    lv_arc_set_angles(
        mouth,
        0,
        360
    );

    lv_obj_set_style_arc_width(
        mouth,
        kMouthThickness,
        LV_PART_INDICATOR
    );

    lv_obj_align(
        mouth,
        LV_ALIGN_CENTER,
        0,
        kMouthOffsetY + 2
    );
}


void MouthTimerCallback(
    lv_timer_t* timer
)
{
    if (!mouth_talking || !IsValid(mouth)) {
        lv_timer_pause(timer);
        SetMouthSmile();
        return;
    }

    switch (mouth_frame % 4) {
        case 0:
        case 2:
            SetMouthSmile();
            break;

        case 1:
            SetMouthOpen(
                kMouthSmallWidth,
                kMouthSmallHeight
            );
            break;

        case 3:
            SetMouthOpen(
                kMouthMediumWidth,
                kMouthMediumHeight
            );
            break;
    }

    mouth_frame++;
}



void AlignEyes(int vertical_shift = 0, int inward_shift = 0)
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    lv_obj_align(
        left_eye,
        LV_ALIGN_CENTER,
        -kEyeOffsetX + inward_shift,
        kEyeOffsetY + vertical_shift
    );

    lv_obj_align(
        right_eye,
        LV_ALIGN_CENTER,
        kEyeOffsetX - inward_shift,
        kEyeOffsetY + vertical_shift
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

    lv_obj_set_style_radius(left_eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_radius(right_eye, LV_RADIUS_CIRCLE, 0);

    AlignEyes();
}


void ApplyHappyGeometry(
    int height,
    int vertical_shift,
    int inward_shift,
    bool show_pupils
)
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    lv_obj_set_size(left_eye, kHappyEyeWidth, height);
    lv_obj_set_size(right_eye, kHappyEyeWidth, height);

    lv_obj_set_style_radius(left_eye, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_radius(right_eye, LV_RADIUS_CIRCLE, 0);

    AlignEyes(vertical_shift, inward_shift);
    SetEyeChildrenVisible(show_pupils);

    if (show_pupils) {
        AlignPupils(0);
    }
}


void FinishHappyBurst()
{
    happy_active = false;
    happy_frame = 0;

    RestoreOpenEyeGeometry();
    SetEyeChildrenVisible(true);
    AlignPupils(0);

    if (blink_timer != nullptr) {
        lv_timer_resume(blink_timer);
        lv_timer_set_period(
            blink_timer,
            static_cast<uint32_t>(lv_rand(2200, 4200))
        );
    }

    if (gaze_timer != nullptr) {
        lv_timer_resume(gaze_timer);
        lv_timer_set_period(
            gaze_timer,
            static_cast<uint32_t>(lv_rand(1200, 2200))
        );
    }
}


void HappyTimerCallback(lv_timer_t* timer)
{
    if (!happy_active || !IsValid(left_eye) || !IsValid(right_eye)) {
        FinishHappyBurst();

        if (timer != nullptr) {
            lv_timer_delete(timer);
        }
        happy_timer = nullptr;
        return;
    }

    // A tiny squash -> bounce -> rebound sequence.
    // The asymmetry in timing keeps it from feeling like a mechanical blink.
    switch (happy_frame) {
        case 0:
            ApplyHappyGeometry(
                kHappyEyeHeightSoft,
                0,
                kHappyEyeInsetX,
                true
            );
            lv_timer_set_period(timer, 90);
            break;

        case 1:
            ApplyHappyGeometry(
                kHappyEyeHeightSquint,
                -kHappyBounceY,
                kHappyEyeInsetX,
                false
            );
            lv_timer_set_period(timer, 135);
            break;

        case 2:
            ApplyHappyGeometry(
                kHappyEyeHeightSquint + 3,
                kHappyBounceY,
                kHappyEyeInsetX,
                false
            );
            lv_timer_set_period(timer, 95);
            break;

        case 3:
            ApplyHappyGeometry(
                kHappyEyeHeightSquint,
                -kHappyBounceY + 1,
                kHappyEyeInsetX + 1,
                false
            );
            lv_timer_set_period(timer, 125);
            break;

        case 4:
            ApplyHappyGeometry(
                kHappyEyeHeightSquint + 2,
                kHappyBounceY - 1,
                kHappyEyeInsetX,
                false
            );
            lv_timer_set_period(timer, 90);
            break;

        case 5:
            ApplyHappyGeometry(
                kHappyEyeHeightSquint + 1,
                -3,
                kHappyEyeInsetX,
                false
            );
            lv_timer_set_period(timer, 115);
            break;

        case 6:
            ApplyHappyGeometry(
                kHappyEyeHeightSoft - 4,
                2,
                1,
                true
            );
            lv_timer_set_period(timer, 100);
            break;

        case 7:
            ApplyHappyGeometry(
                kHappyEyeHeightSoft + 2,
                -1,
                0,
                true
            );
            lv_timer_set_period(timer, 105);
            break;

        case 8:
            RestoreOpenEyeGeometry();
            SetEyeChildrenVisible(true);
            AlignPupils(0);
            lv_timer_set_period(timer, 90);
            break;

        default:
            FinishHappyBurst();
            lv_timer_delete(timer);
            happy_timer = nullptr;
            return;
    }

    happy_frame++;

    if (happy_frame >= kHappyFrameCount) {
        FinishHappyBurst();
        lv_timer_delete(timer);
        happy_timer = nullptr;
    }
}


void StartHappyBurst()
{
    if (!IsValid(left_eye) || !IsValid(right_eye)) {
        return;
    }

    if (happy_timer != nullptr) {
        lv_timer_delete(happy_timer);
        happy_timer = nullptr;
    }

    // Keep blink/gaze from fighting the temporary laugh geometry.
    if (blink_timer != nullptr) {
        lv_timer_pause(blink_timer);
    }

    if (gaze_timer != nullptr) {
        lv_timer_pause(gaze_timer);
    }

    eyes_closed = false;
    happy_active = true;
    happy_frame = 0;

    SetEyeChildrenVisible(true);
    AlignPupils(0);

    happy_timer = lv_timer_create(
        HappyTimerCallback,
        kHappyFramePeriodMs,
        nullptr
    );

    lv_timer_ready(happy_timer);
}


void CancelHappyBurst()
{
    if (happy_timer != nullptr) {
        lv_timer_delete(happy_timer);
        happy_timer = nullptr;
    }

    if (happy_active) {
        FinishHappyBurst();
    }
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
    if (happy_active) {
        return;
    }

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
    if (eyes_closed || happy_active) {
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

    if (mouth_timer != nullptr) {
        lv_timer_delete(mouth_timer);
        mouth_timer = nullptr;
    }

    if (happy_timer != nullptr) {
        lv_timer_delete(happy_timer);
        happy_timer = nullptr;
    }

    happy_active = false;
    happy_frame = 0;

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

    mouth_talking = false;
    mouth_frame = 0;
    SetMouthSmile();

    mouth_timer = lv_timer_create(
        MouthTimerCallback,
        kMouthFramePeriodMs,
        nullptr
    );

    lv_timer_pause(mouth_timer);
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
    printf("MINJI_MOUTH Talk=%d\n", enable ? 1 : 0);

    if (!IsValid(mouth)) {
        return;
    }

    mouth_talking = enable;
    mouth_frame = 0;

    if (mouth_timer == nullptr) {
        mouth_timer = lv_timer_create(
            MouthTimerCallback,
            kMouthFramePeriodMs,
            nullptr
        );
    }

    if (enable) {
        SetMouthSmile();

        // First-stage test hook: one short organic happy/laugh motion at the
        // start of speaking.  This lets us validate the motion without
        // changing eye.h or the application state/event plumbing yet.
        StartHappyBurst();

        lv_timer_resume(mouth_timer);
        lv_timer_ready(mouth_timer);
        return;
    }

    CancelHappyBurst();
    lv_timer_pause(mouth_timer);
    SetMouthSmile();
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

    CancelHappyBurst();
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
    LV_RADIUS_CIRCLE,
    0
);

lv_obj_set_style_radius(
    right_eye,
    LV_RADIUS_CIRCLE,
    0
);

        AlignEyes();
        AlignPupils(0);
        return;
    }

    RestoreOpenEyeGeometry();
    AlignPupils(0);
}
