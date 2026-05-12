#include "App/Scene/Debug/DebugScene.h"
#include "App/Scene/Core/SceneManager.h"

void DebugScene::Initialize() {
    // 1. 各マネージャのポインタを取得して保持する
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // 2. カメラの生成と設定
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera("Main", mainCamera_);

    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera("Debug", debugCamera_);
    cameraMgr_->SetActiveCamera("Main");

    // 3. ライトの生成
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    auto effectMgr = EffectManager::GetInstance();
    effectMgr->Initialize();

    // 1. 今あるパーティクル（Default）
    EffectSetting defaultSetting;
    defaultSetting.name = "Default";
    defaultSetting.textureName = "circle";
    defaultSetting.velocityMin = { -20.0f, -20.0f, -20.0f };
    defaultSetting.velocityMax = {  20.0f,  20.0f,  20.0f };
    defaultSetting.lifeTimeMin = 1.0f;
    defaultSetting.lifeTimeMax = 10.0f;
    defaultSetting.colorStartMin = { 0.0f, 0.0f, 0.0f, 1.0f };
    defaultSetting.colorStartMax = { 1.0f, 1.0f, 1.0f, 1.0f };
    defaultSetting.emitCountMin = 1; // 毎フレーム呼ぶので少なめ
    defaultSetting.emitCountMax = 3;
    effectMgr->RegisterEffect(defaultSetting);
    // 初期化時にエミッターとして稼働開始
    effectMgr->PlayEmitter("Default", { 0.0f, 0.0f, 10.0f });

    // 2. ヒットエフェクト
    EffectSetting hitSetting;
    hitSetting.name = "Hit";
    hitSetting.textureName = "circle";
    hitSetting.scaleMin = { 0.05f, 1.0f, 1.0f };
    hitSetting.scaleMax = { 0.05f, 1.0f, 1.0f };
    hitSetting.rotationMin = { 0.0f, 0.0f, -3.141592f };
    hitSetting.rotationMax = { 0.0f, 0.0f,  3.141592f };
    hitSetting.velocityMin = { 0.0f, 0.0f, 0.0f };
    hitSetting.velocityMax = { 0.0f, 0.0f, 0.0f };
    hitSetting.lifeTimeMin = 1.0f;
    hitSetting.lifeTimeMax = 1.0f;
    hitSetting.emitCountMin = 8;
    hitSetting.emitCountMax = 8;
    effectMgr->RegisterEffect(hitSetting);

    // 3. 剣撃エフェクト
    EffectSetting slashSetting;
    slashSetting.name = "Slash";
    slashSetting.textureName = "circle";
    slashSetting.scaleMin = { 0.05f, 0.4f, 1.0f };
    slashSetting.scaleMax = { 0.05f, 1.5f, 1.0f };
    slashSetting.rotationMin = { 0.0f, 0.0f, -1.0f };
    slashSetting.rotationMax = { 0.0f, 0.0f,  1.0f };
    slashSetting.velocityMin = { 0.0f, 0.0f, 0.0f };
    slashSetting.velocityMax = { 0.0f, 0.0f, 0.0f };
    slashSetting.lifeTimeMin = 1.0f;
    slashSetting.lifeTimeMax = 1.0f;
    slashSetting.emitCountMin = 3;
    slashSetting.emitCountMax = 3;
    effectMgr->RegisterEffect(slashSetting);

    // 4. 炎エフェクト (Fire)
    EffectSetting fireSetting;
    fireSetting.name = "Fire";
    fireSetting.textureName = "circle";
    fireSetting.isBillboard = true;
    fireSetting.isEmitter = true;
    fireSetting.emitFrequency = 0.02f; // さらに密度を上げる
    fireSetting.emitCountMin = 1;
    fireSetting.emitCountMax = 3;
    fireSetting.lifeTimeMin = 0.4f;
    fireSetting.lifeTimeMax = 0.8f;
    fireSetting.spawnAreaMin = { -0.1f, 0.0f, -0.1f }; // 発生源を狭くして中心に寄せる
    fireSetting.spawnAreaMax = {  0.1f, 0.0f,  0.1f };
    fireSetting.velocityMin = { -0.2f, 3.0f, -0.2f }; // 横への広がりを抑える
    fireSetting.velocityMax = {  0.2f, 5.0f,  0.2f };
    fireSetting.scaleMin = { 0.8f, 0.8f, 0.8f }; // 開始時は少し大きく
    fireSetting.scaleMax = { 1.2f, 1.2f, 1.2f };
    fireSetting.scaleEndMin = { 0.0f, 0.0f, 0.0f }; // 頂点では完全に細く
    fireSetting.scaleEndMax = { 0.1f, 0.1f, 0.1f };
    
    // 黄色〜オレンジから始まり、赤くなりながら消える
    fireSetting.colorStartMin = { 1.0f, 0.5f, 0.0f, 1.0f };
    fireSetting.colorStartMax = { 1.0f, 1.0f, 0.2f, 1.0f }; 
    fireSetting.colorEndMin = { 0.5f, 0.0f, 0.0f, 0.0f }; 
    fireSetting.colorEndMax = { 1.0f, 0.1f, 0.0f, 0.0f }; 
    effectMgr->RegisterEffect(fireSetting);
    // 初期化時にエミッターとして稼働開始（Defaultとは別の場所に配置）
    effectMgr->PlayEmitter("Fire", { 5.0f, 0.0f, 0.0f });
}

void DebugScene::ImGuiControl() {
#ifdef _USEIMGUI
    // シーン内のオブジェクトのデバッグ表示
    cameraMgr_->ImGuiControl();
    dirLight_->ImGuiControl("dirLight");
    EffectManager::GetInstance()->ImGuiControl("Effects");
#endif
}

void DebugScene::Update() {
    // シーン切り替え
    if (input_->Trigger(DIK_N)) {
        SceneManager::GetInstance()->ChangeScene("Title");
    }

    // エフェクトのテスト呼び出し
    if (input_->Trigger(DIK_SPACE)) {
        // ヒットエフェクトをカメラ手前で発生させる
        EffectManager::GetInstance()->PlayEffect("Hit", { -2.0f, 0.0f, 2.0f });
    }
    if (input_->Trigger(DIK_RETURN)) {
        // 剣撃エフェクトをカメラ手前で少しずらして発生させる
        EffectManager::GetInstance()->PlayEffect("Slash", { 2.0f, 0.0f, 2.0f });
    }

    // モード切り替え（TABキー）
    if (input_->Trigger(DIK_TAB)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? "Main" : "Debug");
    }

    // オブジェクトの更新
    lightMgr_->Update();
    dirLight_->Update();
    
    EffectManager::GetInstance()->Update();
    
    // カメラの更新
    BaseCamera* active = cameraMgr_->GetActiveCamera();
    DebugCamera* dc = dynamic_cast<DebugCamera*>(active);

    if (dc) {
        dc->SetActive(true);
        dc->Update(input_);
    } else {
        debugCamera_->SetActive(false);
        active->Update();
    }
}

void DebugScene::Draw() {
    EffectManager::GetInstance()->Draw();
}
