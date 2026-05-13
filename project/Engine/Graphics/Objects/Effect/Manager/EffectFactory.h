#pragma once
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"

// エフェクトの初期設定と登録を一元管理するファクトリクラス
class EffectFactory {
public:
    static EffectFactory* GetInstance();

    // デフォルトで用意している各種エフェクトを一括で登録する
    void RegisterAllEffects();

private:
    EffectFactory() = default;
    ~EffectFactory() = default;
    EffectFactory(const EffectFactory&) = delete;
    EffectFactory& operator=(const EffectFactory&) = delete;
};
