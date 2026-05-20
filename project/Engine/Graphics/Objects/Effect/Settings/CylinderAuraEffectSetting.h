#pragma once
#include "EffectSetting.h"

// 円柱状に立ち昇るオーラエフェクトの設定
struct CylinderAuraEffectSetting : public EffectSetting {
    CylinderAuraEffectSetting() {
        name = "CylinderAura";
        textureName = "gradationLine";
        meshType = "cylinder";

        emitCountMin = 1;
        emitCountMax = 1;

        // スポーン地点らしく、長く存在させる
        lifeTimeMin = 2.0f;
        lifeTimeMax = 2.5f;

        // 初期位置
        positionOffset = { 0.0f, 0.0f, 0.0f };
        spawnAreaMin = { 0.0f, 0.0f, 0.0f };
        spawnAreaMax = { 0.0f, 0.0f, 0.0f };

        // 速度はゼロ
        velocityMin = { 0.0f, 0.0f, 0.0f };
        velocityMax = { 0.0f, 0.0f, 0.0f };

        // ゆっくりとした回転 (Y軸)
        rotationVelocityMin = { 0.0f, 0.5f, 0.0f };
        rotationVelocityMax = { 0.0f, 0.8f, 0.0f };

        // 初期スケール (サイン波の最小値。少し残しておく)
        scaleMin = { 1.0f, 0.0f, 1.0f };
        scaleMax = { 1.5f, 0.0f, 1.5f };

        // 終了スケール (サイン波の最大値。大きく膨らむ)
        scaleEndMin = { 3.0f, 3.0f, 3.0f };
        scaleEndMax = { 4.0f, 4.0f, 4.0f };

        // 色（神々しい金〜白のオーラ）
        colorStartMin = { 1.0f, 0.9f, 0.5f, 1.0f };
        colorStartMax = { 1.0f, 1.0f, 0.8f, 1.0f };
        colorEndMin = { 1.0f, 0.5f, 0.0f, 0.0f };
        colorEndMax = { 1.0f, 0.8f, 0.2f, 0.0f };

        isBillboard = false;
        isEmitter = false;

        useEaseInScale = false;
        useEaseOutScale = false; 
        useEaseInAlpha = false;
        useEaseOutAlpha = false;
        useSineScaleY = true;  // 高さを波打たせる
        useSineScaleXZ = true; // 半径を波打たせる
        isUniformScaleXZ = true; // 常に正円を保つ
    }
};
