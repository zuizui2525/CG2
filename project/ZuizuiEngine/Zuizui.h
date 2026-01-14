#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "DxCommon.h"
#include "WindowApp.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "Log.h"
#include "Input.h"
#include "Audio.h"
#include "ModelObject.h"
#include "SphereObject.h"
#include "SpriteObject.h"
#include "TriangleObject.h"
#include "ParticleManager.h"

class Zuizui {
public:
    static Zuizui* GetInstance();

    // 基盤の初期化と終了
    void Initialize();
    void Update();
    void Finalize();

    // フレーム制御
    bool ProcessMessage() { return window->ProcessMessage(); }
    void BeginFrame();
    void EndFrame();

    // 描画関数
    void DrawModel(ModelObject* model, const std::string& textureKey, Vector3 position, bool drawFlag = true);
    void DrawSphere(SphereObject* sphere, const std::string& textureKey, Vector3 position, float radius, bool drawFlag = true);
    void DrawSprite(Vector3 position, SpriteObject* sprite, const std::string& textureKey, bool drawFlag = true);
    void DrawTriangle(Vector3 position, TriangleObject* triangle, const std::string& textureKey, bool drawFlag = true);
    void DrawParticle(Vector3 position, ParticleManager* particle, const std::string& textureKey, bool drawFlag = true);
    
    // キーボード入力
    bool TriggerKey(BYTE key) const;
    bool PressKey(BYTE key) const;
    bool ReleaseKey(BYTE key) const;

    // ゲッター
    ID3D12Device* GetDevice() { return dxCommon->GetDevice(); }
    DxCommon* GetDxCommon() { return dxCommon.get(); }
    Camera* GetCamera() { return camera.get(); }
    PSOManager* GetPSOManager() { return psoManager.get(); }
    TextureManager* GetTextureManager() { return textureManager.get(); }

private:
    Zuizui() = default;
    static Zuizui* instance;

    std::unique_ptr<WindowApp> window;
    std::unique_ptr<Log> logger;
    std::unique_ptr<DxCommon> dxCommon;
    std::unique_ptr<Audio> audio;
    std::unique_ptr<Input> input;
    std::unique_ptr<PSOManager> psoManager;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<DirectionalLightObject> dirLight;

#ifdef _DEBUG
    std::unique_ptr<class ImguiManager> imgui;
#endif
};
