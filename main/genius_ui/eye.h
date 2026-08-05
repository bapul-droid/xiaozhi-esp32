#pragma once

#include "lvgl.h"


class Eye {
public:
    static void Create(lv_obj_t* parent);

    static void Blink();

    static void Talk(bool enable);

    static void LookLeft();

    static void LookRight();

    static void LookCenter();

private:
    static void SetClosed(bool closed);
};
