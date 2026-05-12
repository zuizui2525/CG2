#pragma once
#include "EffectSetting.h"
#include "BaseParticleObject.h"
#include <unordered_map>
#include <memory>
#include <string>

class EffectManager {
public:
    static EffectManager* GetInstance();

    void Initialize();
    void Finalize();
    void Update();
    void Draw();

    void ImGuiControl(const std::string& name);

    // エフェクト設定の登録
    void RegisterEffect(const EffectSetting& setting);

    // --- 新しい再生API ---
    
    // 2Dエフェクト再生 (isLoop=trueでエミッターモード)
    void PlayEffect2D(
        const std::string& name, 
        const Vector3& position, 
        bool isLoop = false, 
        const std::string& textureKey = "", 
        const Vector3& velocityOverride = {0,0,0}
    );

    // 3Dエフェクト再生 (isLoop=trueでエミッターモード)
    void PlayEffect3D(
        const std::string& name, 
        const Vector3& position, 
        bool isLoop = false, 
        const std::string& modelKey = "", 
        const std::string& textureKey = "", 
        const Vector3& velocityOverride = {0,0,0}
    );

    // 旧APIとの互換性（必要に応じて）
    void PlayEffect(const std::string& name, const Vector3& position) { PlayEffect2D(name, position, false); }
    void PlayEmitter(const std::string& name, const Vector3& position) { PlayEffect2D(name, position, true); }

private:
    EffectManager() = default;
    ~EffectManager() = default;
    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;

    std::unordered_map<std::string, std::unique_ptr<BaseParticleObject>> effectMap_;
    bool isWindowOpen_ = false;
};
