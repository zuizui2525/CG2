#pragma once
#include "EffectSetting.h"

class HitEffectSetting : public EffectSetting {
public:
    HitEffectSetting() {
        name = "Hit";
        textureName = "circle";
        scaleMin = { 0.05f, 1.0f, 1.0f };
        scaleMax = { 0.05f, 1.0f, 1.0f };
        rotationMin = { 0.0f, 0.0f, -3.141592f };
        rotationMax = { 0.0f, 0.0f,  3.141592f };
        velocityMin = { 0.0f, 0.0f, 0.0f };
        velocityMax = { 0.0f, 0.0f, 0.0f };
        lifeTimeMin = 1.0f;
        lifeTimeMax = 1.0f;
        emitCountMin = 8;
        emitCountMax = 8;
    }
};
