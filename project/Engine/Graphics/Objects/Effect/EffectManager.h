#pragma once
#include "EffectSetting.h"
#include "ParticleObject.h"
#include <unordered_map>
#include <memory>
#include <string>

class EffectManager {
public:
    static EffectManager* GetInstance();

    // 初期化・更新・描画
    void Initialize();
    void Finalize();
    void Update();
    void Draw();

    // ImGui
    void ImGuiControl(const std::string& name);

    // エフェクト設定の登録
    void RegisterEffect(const EffectSetting& setting);

    // エフェクトの再生
    void PlayEffect(const std::string& name, const Vector3& position);

    // エミッターとして継続再生
    void PlayEmitter(const std::string& name, const Vector3& position);

private:
    EffectManager() = default;
    ~EffectManager() = default;
    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<ParticleObject>> effectMap_;
    bool isWindowOpen_ = false;
};
