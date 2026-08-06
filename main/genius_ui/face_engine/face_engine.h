#pragma once

#include "lvgl.h"
#include "face_state.h"
#include "face_theme.h"

class FaceEngine {
public:
    static bool Init(
        lv_obj_t* screen,
        FaceTheme* theme
    );

    static bool IsReady();

    static bool SetState(
        FaceState state
    );

    static FaceState GetState();

    static FaceTheme* GetTheme();

private:
    static bool IsScreenValid();
};
