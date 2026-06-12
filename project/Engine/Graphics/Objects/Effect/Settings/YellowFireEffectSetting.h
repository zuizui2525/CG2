#pragma once
#include "EffectSetting.h"

class YellowFireEffectSetting : public EffectSetting {
public:
    YellowFireEffectSetting() {
        name = "YellowFire";
        textureName = "circle";
        isBillboard = true;
        isEmitter = true;
        emitFrequency = 0.02f;
        emitCountMin = 1;
        emitCountMax = 3;
        lifeTimeMin = 0.4f;
        lifeTimeMax = 0.8f;
        spawnAreaMin = { -0.1f, 0.0f, -0.1f };
        spawnAreaMax = {  0.1f, 0.0f,  0.1f };
        velocityMin = { -0.2f, 3.0f, -0.2f };
        velocityMax = {  0.2f, 5.0f,  0.2f };
        scaleMin = { 0.8f, 0.8f, 0.8f };
        scaleMax = { 1.2f, 1.2f, 1.2f };
        scaleEndMin = { 0.0f, 0.0f, 0.0f };
        scaleEndMax = { 0.1f, 0.1f, 0.1f };
        colorStartMin = { 1.0f, 0.8f, 0.0f, 1.0f };
        colorStartMax = { 1.0f, 1.0f, 0.3f, 1.0f }; 
        colorEndMin = { 0.4f, 0.3f, 0.0f, 0.0f }; 
        colorEndMax = { 0.8f, 0.6f, 0.0f, 0.0f }; 
    }
};
