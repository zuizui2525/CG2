#pragma once
#include "EffectSetting.h"

class DragonBreathEffectSetting : public EffectSetting {
public:
    DragonBreathEffectSetting() {
        name = "DragonBreath";
        meshType = "cube";
        isBillboard = false;
        emitFrequency = 0.02f;
        emitCountMin = 1;
        emitCountMax = 2;
        lifeTimeMin = 0.8f;
        lifeTimeMax = 1.2f;
        velocityMin = { -1.5f, -1.0f, 15.0f };
        velocityMax = {  1.5f,  1.0f, 25.0f };
        scaleMin = { 0.2f, 0.2f, 0.2f };
        scaleMax = { 0.4f, 0.4f, 0.4f };
        scaleEndMin = { 1.5f, 1.5f, 1.5f };
        scaleEndMax = { 2.5f, 2.5f, 2.5f };
        rotationMin = { -3.14f, -3.14f, -3.14f };
        rotationMax = {  3.14f,  3.14f,  3.14f };
        colorStartMin = { 1.0f, 1.0f, 1.0f, 1.0f };
        colorStartMax = { 1.0f, 1.0f, 0.5f, 1.0f };
        colorEndMin = { 1.0f, 0.2f, 0.0f, 0.0f };
        colorEndMax = { 0.5f, 0.0f, 0.0f, 0.0f };
    }
};
