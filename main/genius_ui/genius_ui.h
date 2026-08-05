#pragma once

#include "lvgl.h"

class GeniusUI {
public:
    static void Init(lv_obj_t* screen);

private:
    static lv_obj_t* left_eye_;
    static lv_obj_t* right_eye_;
};