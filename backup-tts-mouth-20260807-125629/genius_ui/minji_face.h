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

    // Speaking is an independent device state, not an emotion.
    // This lets Minji be Happy/Thinking/etc. while the mouth keeps talking.
    static void SetSpeaking(
        bool speaking
    );

    static bool IsSpeaking();

    static Emotion GetEmotion();

private:
    static bool IsScreenValid();
};
