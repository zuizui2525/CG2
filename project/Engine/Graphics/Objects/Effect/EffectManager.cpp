#include "EffectManager.h"
#include <random>
#include <imgui.h>

EffectManager* EffectManager::GetInstance() {
    static EffectManager instance;
    return &instance;
}

void EffectManager::Initialize() {
    effectMap_.clear();
}

void EffectManager::Finalize() {
    effectMap_.clear();
}

void EffectManager::Update() {
    for (auto& pair : effectMap_) {
        pair.second->Update();
    }
}

void EffectManager::Draw() {
    for (auto& pair : effectMap_) {
        const std::string& texName = pair.second->GetSetting().textureName;
        pair.second->Draw(texName, true);
    }
}

void EffectManager::RegisterEffect(const EffectSetting& setting) {
    auto particle = std::make_unique<ParticleObject>();
    
    // 初期化処理
    particle->Initialize();
    
    // デフォルトのEmitterによる自動発生をOFFにする
    particle->SetEmitterMode(false);

    // 設定を適用
    particle->SetSetting(setting);
    
    effectMap_[setting.name] = std::move(particle);
}

void EffectManager::PlayEffect(const std::string& name, const Vector3& position) {
    auto it = effectMap_.find(name);
    if (it != effectMap_.end()) {
        it->second->SetEmitterMode(false); // 単発再生

        const EffectSetting& setting = it->second->GetSetting();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> distCount(setting.emitCountMin, setting.emitCountMax);
        uint32_t count = distCount(gen);

        if (count > 0) {
            it->second->EmitAt(position, count);
        }
    }
}

void EffectManager::PlayEmitter(const std::string& name, const Vector3& position) {
    auto it = effectMap_.find(name);
    if (it != effectMap_.end()) {
        it->second->SetEmitterMode(true); // 継続発生モード
        it->second->SetPosition(position);
    }
}

void EffectManager::ImGuiControl(const std::string& name) {
#ifdef _USEIMGUI
    ImGui::Begin((name + " List").c_str());
    ImGui::Checkbox((name + " Settings").c_str(), &isWindowOpen_);
    ImGui::End();

    if (isWindowOpen_) {
        if (ImGui::Begin((name + " Control").c_str(), &isWindowOpen_)) {
            for (auto& pair : effectMap_) {
                std::string effectName = pair.first;
                EffectSetting& setting = pair.second->GetSettingRef();

                if (ImGui::CollapsingHeader(effectName.c_str())) {
                    std::string label = "##" + effectName;
                    
                    ImGui::Text("Base Settings");
                    int emitMin = setting.emitCountMin;
                    int emitMax = setting.emitCountMax;
                    if(ImGui::DragInt(("Emit Count Min" + label).c_str(), &emitMin, 1, 1, 100)) setting.emitCountMin = emitMin;
                    if(ImGui::DragInt(("Emit Count Max" + label).c_str(), &emitMax, 1, 1, 100)) setting.emitCountMax = emitMax;
                    ImGui::DragFloat(("LifeTime Min" + label).c_str(), &setting.lifeTimeMin, 0.1f, 0.1f, 10.0f);
                    ImGui::DragFloat(("LifeTime Max" + label).c_str(), &setting.lifeTimeMax, 0.1f, 0.1f, 10.0f);

                    ImGui::SeparatorText("Transform");
                    ImGui::DragFloat3(("Position Offset" + label).c_str(), &setting.positionOffset.x, 0.1f);
                    ImGui::DragFloat3(("Spawn Area Min" + label).c_str(), &setting.spawnAreaMin.x, 0.1f);
                    ImGui::DragFloat3(("Spawn Area Max" + label).c_str(), &setting.spawnAreaMax.x, 0.1f);
                    ImGui::DragFloat3(("Velocity Min" + label).c_str(), &setting.velocityMin.x, 0.1f);
                    ImGui::DragFloat3(("Velocity Max" + label).c_str(), &setting.velocityMax.x, 0.1f);
                    ImGui::DragFloat3(("Scale Start Min" + label).c_str(), &setting.scaleMin.x, 0.05f);
                    ImGui::DragFloat3(("Scale Start Max" + label).c_str(), &setting.scaleMax.x, 0.05f);
                    ImGui::DragFloat3(("Scale End Min" + label).c_str(), &setting.scaleEndMin.x, 0.05f);
                    ImGui::DragFloat3(("Scale End Max" + label).c_str(), &setting.scaleEndMax.x, 0.05f);
                    ImGui::DragFloat3(("Rotation Min" + label).c_str(), &setting.rotationMin.x, 0.1f);
                    ImGui::DragFloat3(("Rotation Max" + label).c_str(), &setting.rotationMax.x, 0.1f);

                    ImGui::SeparatorText("Features");
                    ImGui::Checkbox(("Billboard" + label).c_str(), &setting.isBillboard);
                    ImGui::Checkbox(("Emitter Mode" + label).c_str(), &setting.isEmitter);
                    if (setting.isEmitter) {
                        ImGui::DragFloat(("Emit Frequency" + label).c_str(), &setting.emitFrequency, 0.1f, 0.1f, 10.0f);
                    }


                    ImGui::SeparatorText("Color");
                    ImGui::ColorEdit4(("Color Start Min" + label).c_str(), &setting.colorStartMin.x);
                    ImGui::ColorEdit4(("Color Start Max" + label).c_str(), &setting.colorStartMax.x);
                    ImGui::ColorEdit4(("Color End Min" + label).c_str(), &setting.colorEndMin.x);
                    ImGui::ColorEdit4(("Color End Max" + label).c_str(), &setting.colorEndMax.x);
                }
            }
        }
        ImGui::End();
    }
#endif
}
