#pragma once

#include "lvgl.h"
#include "face_state.h"

class FaceTheme {
public:
    virtual ~FaceTheme() = default;

    virtual bool Init(
        lv_obj_t* screen
    ) = 0;

    virtual bool IsReady() const = 0;

    virtual void ApplyState(
        FaceState state
    ) = 0;
};
