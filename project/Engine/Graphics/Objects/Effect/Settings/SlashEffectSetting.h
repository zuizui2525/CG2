#pragma once
#include "EffectSetting.h"

class SlashEffectSetting : public EffectSetting {
public:
    SlashEffectSetting() {
        name = "Slash";
        textureName = "circle";
        scaleMin = { 0.05f, 0.4f, 1.0f };
        scaleMax = { 0.05f, 1.5f, 1.0f };
        rotationMin = { 0.0f, 0.0f, -1.0f };
        rotationMax = { 0.0f, 0.0f,  1.0f };
        velocityMin = { 0.0f, 0.0f, 0.0f };
        velocityMax = { 0.0f, 0.0f, 0.0f };
        lifeTimeMin = 1.0f;
        lifeTimeMax = 1.0f;
        emitCountMin = 3;
        emitCountMax = 3;
    }
};
