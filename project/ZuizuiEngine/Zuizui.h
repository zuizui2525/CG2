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

// 前方宣言
class ModelObject;
class SpriteObject;
class SphereObject;
class TriangleObject;
class ParticleManager;

class Zuizui {
public:
    static Zuizui* GetInstance();

    // 基盤の初期化と終了
    void Initialize();
    void Finalize();

    // フレーム制御
    bool ProcessMessage() { return window.ProcessMessage(); }
    void BeginFrame();
    void EndFrame();

    // --- 簡略化された描画関数 ---
    void DrawModel(ModelObject* model, const std::string& textureKey, bool drawFlag = true);
    void DrawSphere(SphereObject* sphere, const std::string& textureKey, bool drawFlag = true);
    void DrawSprite(SpriteObject* sprite, const std::string& textureKey, bool drawFlag = true);
    void DrawTriangle(TriangleObject* triangle, const std::string& textureKey, bool drawFlag = true);
    void DrawParticle(ParticleManager* particle, const std::string& textureKey, bool drawFlag = true);

    // ゲッター
    ID3D12Device* GetDevice() { return dxCommon.GetDevice(); }
    DxCommon* GetDxCommon() { return &dxCommon; }
    Camera* GetCamera() { return camera.get(); }
    PSOManager* GetPSOManager() { return psoManager.get(); }
    TextureManager* GetTextureManager() { return textureManager.get(); }

private:
    Zuizui() = default;
    static Zuizui* instance;

    WindowApp window;
    Log logger;
    DxCommon dxCommon;
    Audio audio;
    std::unique_ptr<Input> input;
    std::unique_ptr<PSOManager> psoManager;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<DirectionalLightObject> dirLight;

#ifdef _DEBUG
    std::unique_ptr<class ImguiManager> imgui;
#endif
};
