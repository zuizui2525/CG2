#pragma once
#include "EffectSetting.h"

class ExplosionEffectSetting : public EffectSetting {
public:
    ExplosionEffectSetting() {
        name = "Explosion";
        meshType = "cube";
        isBillboard = false;
        emitCountMin = 20;
        emitCountMax = 30;
        lifeTimeMin = 0.5f;
        lifeTimeMax = 1.2f;
        velocityMin = { -15.0f, -15.0f, -15.0f };
        velocityMax = {  15.0f,  15.0f,  15.0f };
        scaleMin = { 0.2f, 0.2f, 0.2f };
        scaleMax = { 0.6f, 0.6f, 0.6f };
        scaleEndMin = { 0.0f, 0.0f, 0.0f };
        scaleEndMax = { 0.1f, 0.1f, 0.1f };
        rotationMin = { -3.14f, -3.14f, -3.14f };
        rotationMax = {  3.14f,  3.14f,  3.14f };
        colorStartMin = { 1.0f, 0.8f, 0.0f, 1.0f };
        colorStartMax = { 1.0f, 1.0f, 0.5f, 1.0f };
        colorEndMin = { 0.2f, 0.0f, 0.0f, 0.0f };
        colorEndMax = { 0.5f, 0.1f, 0.0f, 0.0f };
    }
};
