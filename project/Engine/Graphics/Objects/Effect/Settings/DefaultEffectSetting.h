#pragma once
#include "EffectSetting.h"

class DefaultEffectSetting : public EffectSetting {
public:
    DefaultEffectSetting() {
        name = "Default";
        textureName = "circle";
        velocityMin = { -20.0f, -20.0f, -20.0f };
        velocityMax = {  20.0f,  20.0f,  20.0f };
        lifeTimeMin = 1.0f;
        lifeTimeMax = 10.0f;
        colorStartMin = { 0.0f, 0.0f, 0.0f, 1.0f };
        colorStartMax = { 1.0f, 1.0f, 1.0f, 1.0f };
        emitCountMin = 1;
        emitCountMax = 3;
    }
};
