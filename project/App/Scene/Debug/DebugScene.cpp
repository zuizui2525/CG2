#include "App/Scene/Debug/DebugScene.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectFactory.h"

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

    // エフェクトの一括登録
    EffectFactory::GetInstance()->RegisterAllEffects();

    // 初期化時にエミッターとして稼働開始するもの
    EffectPlayParam defaultParam;
    defaultParam.position = { 0.0f, 0.0f, 10.0f };
    defaultParam.isLoop = true;
    effectMgr->PlayEffect2D("Default", defaultParam);

    EffectPlayParam fireParam;
    fireParam.position = { 5.0f, 0.0f, 0.0f };
    fireParam.isLoop = true;
    effectMgr->PlayEffect2D("Fire", fireParam);
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
        EffectPlayParam hitParam;
        hitParam.position = { -2.0f, 0.0f, 2.0f };
        EffectManager::GetInstance()->PlayEffect2D("Hit", hitParam);
    }
    if (input_->Trigger(DIK_RETURN)) {
        // 剣撃エフェクトをカメラ手前で少しずらして発生させる
        EffectPlayParam slashParam;
        slashParam.position = { 2.0f, 0.0f, 2.0f };
        // 例: スケールを大きくして回転も加えてみる
        slashParam.scale = { 2.0f, 2.0f, 2.0f };
        slashParam.rotation = { 0.0f, 0.0f, 0.5f };
        EffectManager::GetInstance()->PlayEffect2D("Slash", slashParam);
    }
    if (input_->Trigger(DIK_P)) {
        // 3D爆発エフェクトを原点で発生させる
        EffectPlayParam expParam;
        expParam.position = { 0.0f, 0.0f, 0.0f };
        EffectManager::GetInstance()->PlayEffect3D("Explosion", expParam);
    }

    // --- 花火エフェクト ---
    // [U] キー：昇る単体
    if (input_->Trigger(DIK_U)) {
        EffectPlayParam param;
        param.position = { -5.0f, 0.0f, 5.0f };
        EffectManager::GetInstance()->PlayEffect3D("FireworksAscend", param);
    }
    // [Y] キー：弾ける単体
    if (input_->Trigger(DIK_Y)) {
        EffectPlayParam param;
        param.position = { 5.0f, 22.5f, 5.0f };
        EffectManager::GetInstance()->PlayEffect3D("FireworksBurst", param);
    }
    // [I] キー：セット（昇ってから弾ける）
    if (input_->Trigger(DIK_I)) {
        EffectPlayParam param;
        // 1. ランダムな色を作る (例: 虹色のどこか)
        float r = static_cast<float>(rand()) / RAND_MAX;
        float g = static_cast<float>(rand()) / RAND_MAX;
        float b = static_cast<float>(rand()) / RAND_MAX;
        param.colorOverride = { r, g, b, 1.0f };
        param.position = { static_cast<float>(rand() % 100 + 1), 0.0f, 5.0f };
        EffectManager::GetInstance()->PlayEffect3D("FireworksSet", param);
    }

    // ドラゴンブレスの操作 (Oキー押しっぱなしで放出)
    if (input_->Press(DIK_O)) {
        // エミッターとして再生
        EffectPlayParam breathParam;
        breathParam.position = { 0.0f, 0.0f, 2.0f };
        breathParam.isLoop = true;
        EffectManager::GetInstance()->PlayEffect3D("DragonBreath", breathParam);
    } else {
        // 離したら停止
        EffectManager::GetInstance()->StopEffect("DragonBreath");
    }

    // KキーでRingエフェクトの発生
    if (input_->Trigger(DIK_K)) {
        EffectPlayParam ringParam;
        ringParam.position = { 0.0f, 0.0f, 0.0f };
        EffectManager::GetInstance()->PlayEffect3D("RingAura", ringParam);
    }

    // Lキーで斬撃（RingSlash）エフェクトの発生
    if (input_->Trigger(DIK_L)) {
        EffectPlayParam slashParam;
        slashParam.position = { 0.0f, 2.0f, 0.0f }; // 少し高めの位置
        EffectManager::GetInstance()->PlayEffect3D("RingSlash", slashParam);
    }

    // Jキーで円柱（CylinderAura）エフェクトの発生
    if (input_->Trigger(DIK_J)) {
        EffectPlayParam cylParam;
        cylParam.position = { 0.0f, 0.0f, 0.0f };
        EffectManager::GetInstance()->PlayEffect3D("CylinderAura", cylParam);
    }

    // Hキーで雨（水滴落下）エフェクトの切り替え
    if (input_->Trigger(DIK_H)) {
        auto effectMgr = EffectManager::GetInstance();
        auto effect = effectMgr->GetEffect("WaterDrop");
        
        if (effect) {
            auto& setting = effect->GetSettingRef();
            bool isRaining = !setting.isEmitter; // トグル
            setting.isEmitter = isRaining;

            if (isRaining) {
                EffectPlayParam dropParam;
                dropParam.position = { 0.0f, 0.0f, 0.0f }; 
                dropParam.isLoop = true;
                effectMgr->PlayEffect3D("WaterDrop", dropParam);
            }
        }
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
