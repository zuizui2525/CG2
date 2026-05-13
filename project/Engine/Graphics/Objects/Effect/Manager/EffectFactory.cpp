#include "EffectFactory.h"
#include "../Settings/DefaultEffectSetting.h"
#include "../Settings/HitEffectSetting.h"
#include "../Settings/SlashEffectSetting.h"
#include "../Settings/FireEffectSetting.h"
#include "../Settings/ExplosionEffectSetting.h"
#include "../Settings/DragonBreathEffectSetting.h"
#include "../Settings/FireworksBurstEffectSetting.h"
#include "../Settings/FireworksSparkleEffectSetting.h"
#include "../Settings/FireworksFlameEffectSetting.h"
#include "../Settings/FireworksAscendEffectSetting.h"
#include "../Settings/FireworksSetEffectSetting.h"

EffectFactory* EffectFactory::GetInstance() {
    static EffectFactory instance;
    return &instance;
}

void EffectFactory::RegisterAllEffects() {
    auto effectMgr = EffectManager::GetInstance();

    // 各クラスのインスタンスを生成して登録
    effectMgr->RegisterEffect(DefaultEffectSetting());
    effectMgr->RegisterEffect(HitEffectSetting());
    effectMgr->RegisterEffect(SlashEffectSetting());
    effectMgr->RegisterEffect(FireEffectSetting());
    effectMgr->RegisterEffect(ExplosionEffectSetting());
    effectMgr->RegisterEffect(DragonBreathEffectSetting());
    effectMgr->RegisterEffect(FireworksBurstEffectSetting());
    effectMgr->RegisterEffect(FireworksSparkleEffectSetting());
    effectMgr->RegisterEffect(FireworksFlameEffectSetting());
    effectMgr->RegisterEffect(FireworksAscendEffectSetting());
    effectMgr->RegisterEffect(FireworksSetEffectSetting());
}
