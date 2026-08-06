#pragma once

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
