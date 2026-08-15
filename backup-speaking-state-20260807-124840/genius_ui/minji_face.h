#pragma once

#include "lvgl.h"


class MinjiFace {
public:
    enum class Emotion {
        Idle,
        Listening,
        Thinking,
        Speaking,
        Happy,
        Sleep,
        Error,
    };

    static void Init(
        lv_obj_t* screen
    );

    static bool IsReady();

    static void SetEmotion(
        Emotion emotion
    );

    static Emotion GetEmotion();

private:
    static bool IsScreenValid();
};
