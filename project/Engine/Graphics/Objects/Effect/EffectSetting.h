#pragma once
#include "Engine/Math/MathStructs.h"
#include <string>

// エフェクトの種類ごとに定義するパラメータ構造体
struct EffectSetting {
    std::string name;            // エフェクトの識別名（"Hit", "Slash" など）
    std::string textureName;     // 使用するテクスチャキー ("white", "circle2" など)

    // 1回の再生で発生するパーティクル数
    uint32_t emitCountMin;
    uint32_t emitCountMax;

    // 寿命 (秒)
    float lifeTimeMin;
    float lifeTimeMax;

    // 発生範囲 (再生座標からのオフセットと、さらにランダムなばらつき範囲)
    Vector3 positionOffset;
    Vector3 spawnAreaMin;
    Vector3 spawnAreaMax;

    // 初速度
    Vector3 velocityMin;
    Vector3 velocityMax;

    // 初期スケールと終了スケール
    Vector3 scaleMin;
    Vector3 scaleMax;
    Vector3 scaleEndMin;
    Vector3 scaleEndMax;

    // 初期回転 (ラジアン等)
    Vector3 rotationMin;
    Vector3 rotationMax;

    // 初期カラーと終了カラー
    Vector4 colorStartMin;
    Vector4 colorStartMax;
    Vector4 colorEndMin;
    Vector4 colorEndMax;

    // --- 機能フラグ ---
    bool isBillboard;       // 常にカメラを向くか
    bool isEmitter;         // 継続して発生するか（エミッターモード）
    float emitFrequency;    // エミッターモード時の発生間隔（秒）

    // デフォルトコンストラクタ（無難な値）
    EffectSetting() :
        name("Default"), textureName("white"),
        emitCountMin(1), emitCountMax(1),
        lifeTimeMin(1.0f), lifeTimeMax(1.0f),
        positionOffset{ 0.0f, 0.0f, 0.0f }, 
        spawnAreaMin{ 0.0f, 0.0f, 0.0f }, spawnAreaMax{ 0.0f, 0.0f, 0.0f },
        velocityMin{ 0.0f, 0.0f, 0.0f }, velocityMax{ 0.0f, 0.0f, 0.0f },
        scaleMin{ 1.0f, 1.0f, 1.0f }, scaleMax{ 1.0f, 1.0f, 1.0f },
        scaleEndMin{ 1.0f, 1.0f, 1.0f }, scaleEndMax{ 1.0f, 1.0f, 1.0f },
        rotationMin{ 0.0f, 0.0f, 0.0f }, rotationMax{ 0.0f, 0.0f, 0.0f },
        colorStartMin{ 1.0f, 1.0f, 1.0f, 1.0f }, colorStartMax{ 1.0f, 1.0f, 1.0f, 1.0f },
        colorEndMin{ 1.0f, 1.0f, 1.0f, 1.0f }, colorEndMax{ 1.0f, 1.0f, 1.0f, 1.0f },
        isBillboard(true), isEmitter(false), emitFrequency(0.5f)
    {}
};
