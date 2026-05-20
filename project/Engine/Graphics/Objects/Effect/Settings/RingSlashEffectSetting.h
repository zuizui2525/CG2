#pragma once
#include "EffectSetting.h"

// Ring(FlatRing)を使った斬撃エフェクトの設定
struct RingSlashEffectSetting : public EffectSetting {
    RingSlashEffectSetting() {
        name = "RingSlash";
        textureName = "slashTex";
        meshType = "flat_ring"; 

        emitCountMin = 1;
        emitCountMax = 1;
        // 斬撃なので非常に短い時間で消える
        lifeTimeMin = 0.15f;
        lifeTimeMax = 0.25f;

        // 初期スケール（細長い楕円形からスタート）
        scaleMin = { 0.4f, 0.1f, 0.1f };
        scaleMax = { 0.6f, 0.15f, 0.15f };

        // 終了スケール (斬撃の軌跡の方向に細長く一気に広がる)
        scaleEndMin = { 8.0f, 1.5f, 1.0f };
        scaleEndMax = { 10.0f, 2.0f, 1.0f };

        // 三日月型のテクスチャ(slashTex)を使うことで「らせんがん」ではなく「斬撃」になるため、
        // どの角度から出てもカッコいい斬撃として見えます。
        // Z軸(画面上のナナメ): -3.14〜3.14で完全にランダムな角度から斬る！
        rotationMin = { 0.2f, -0.2f, -3.14f };
        rotationMax = { 0.4f,  0.2f,  3.14f };

        // 色（鋭い白〜水色）
        colorStartMin = { 0.8f, 0.9f, 1.0f, 1.0f };
        colorStartMax = { 1.0f, 1.0f, 1.0f, 1.0f };
        colorEndMin = { 0.0f, 0.2f, 1.0f, 0.0f }; // 最後は完全に透明
        colorEndMax = { 0.0f, 0.5f, 1.0f, 0.0f };

        isBillboard = false; // 空間上に角度を固定して広がる
        isEmitter = false;
    }
};
