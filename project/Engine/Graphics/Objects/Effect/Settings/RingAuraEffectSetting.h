#pragma once
#include "EffectSetting.h"

// Ring(FlatRing)を使ったオーラ/衝撃波エフェクトの設定
struct RingAuraEffectSetting : public EffectSetting {
    RingAuraEffectSetting() {
        name = "RingAura";
        textureName = "gradationLine";
        meshType = "flat_ring"; // 新しく追加した平面リング

        emitCountMin = 1;
        emitCountMax = 1;
        lifeTimeMin = 1.0f;
        lifeTimeMax = 1.2f;

        // 初期スケール
        scaleMin = { 0.1f, 0.1f, 0.1f };
        scaleMax = { 0.2f, 0.2f, 0.2f };

        // 終了スケール (大きく広がる)
        scaleEndMin = { 5.0f, 5.0f, 5.0f };
        scaleEndMax = { 6.0f, 6.0f, 6.0f };

        // 床に水平に置くためにX軸で90度(1.5708ラジアン)回転させる
        rotationMin = { 1.5708f, 0.0f, 0.0f };
        rotationMax = { 1.5708f, 0.0f, 0.0f };

        // 色（青っぽく光る）
        colorStartMin = { 0.2f, 0.8f, 1.0f, 1.0f };
        colorStartMax = { 0.4f, 1.0f, 1.0f, 1.0f };
        colorEndMin = { 0.0f, 0.2f, 1.0f, 0.0f }; // 最後は透明に
        colorEndMax = { 0.0f, 0.4f, 1.0f, 0.0f };

        // その他の設定
        isBillboard = false; // XY平面に置くためビルボードはOFFにするか、カメラに向けるならON。今回は床に広がるようにOFFを想定
        isEmitter = false;
    }
};
