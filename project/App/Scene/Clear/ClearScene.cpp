#include "App/Scene/Clear/ClearScene.h"
#include "Engine/Base/BaseResource.h"
#include "App/Scene/Core/SceneManager.h"
#include "Engine/Graphics/PostProcess/PostProcess.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectFactory.h"
#include "Engine/Graphics/Objects/Effect/Manager/EffectManager.h"
#include <cstdlib>

/**
 * @brief クリアシーンの初期化処理
 * カメラ、ライト、Cubeオブジェクトの生成と初期パラメータ設定を行います。
 */
void ClearScene::Initialize() {
    // ポストプロセスのポインタを取得してメンバ変数に保持
    postProcess_ = SceneManager::GetInstance()->GetPostProcess();

    // 各マネージャへのポインタをリソース管理者から取得
    cameraMgr_ = CameraResource::GetCameraManager();
    lightMgr_ = LightResource::GetLightManager();
    input_ = InputResource::GetInput();

    // メインカメラの生成とマネージャへの登録
    mainCamera_ = std::make_shared<BaseCamera>();
    mainCamera_->Initialize();
    cameraMgr_->AddCamera(kMainCameraName, mainCamera_);

    // デバッグ確認用のフリーカメラの生成とマネージャへの登録
    debugCamera_ = std::make_shared<DebugCamera>();
    debugCamera_->Initialize();
    cameraMgr_->AddCamera(kDebugCameraName, debugCamera_);

    // シーン遷移時の初期カメラをメインカメラに指定
    cameraMgr_->SetActiveCamera(kMainCameraName);

    // 平行光源の生成、初期化、マネージャへの追加
    dirLight_ = std::make_unique<DirectionalLightObject>();
    dirLight_->Initialize();
    lightMgr_->AddDirectionalLight(dirLight_.get());

    // エフェクト管理システムの初期化とエフェクトの全登録
    auto effectMgr = EffectManager::GetInstance();
    effectMgr->Initialize();
    EffectFactory::GetInstance()->RegisterAllEffects();
}

/**
 * @brief ImGuiによるデバッグ表示処理
 */
void ClearScene::ImGuiControl() {
#ifdef _USEIMGUI
    // カメラ切り替え等のマネージャパラメータを表示
    cameraMgr_->ImGuiControl();
    // ポストプロセスのパラメータ調整用ImGuiコントロール

    // ポストプロセスのパラメータ調整用ImGuiコントロール
    if (postProcess_) {
        postProcess_->ImGuiControl();
    }
#endif
}

/**
 * @brief 毎フレーム更新処理（キー入力によるカメラ切り替え、オブジェクトの座標・パラメータ更新）
 */
void ClearScene::Update() {
#ifdef _USEIMGUI
    // TABキーによりメインカメラとデバッグカメラを切り替える
    static constexpr int kCameraToggleKey = DIK_TAB; // カメラ切り替え用キー定数
    if (input_->Trigger(kCameraToggleKey)) {
        bool isCurrentlyDebug = (cameraMgr_->GetActiveCamera() == debugCamera_.get());
        cameraMgr_->SetActiveCamera(isCurrentlyDebug ? kMainCameraName : kDebugCameraName);
    }
#endif
    // シーン切り替え
    if (input_->Trigger(DIK_SPACE)) {
        SceneManager::GetInstance()->ChangeScene("Title");
    }

    // ライトの更新
    dirLight_->Update();

    // エフェクトの更新
    EffectManager::GetInstance()->Update();

    // 現在アクティブなカメラを判定し、それぞれのカメラ種別に応じた更新処理を呼ぶ
    BaseCamera* activeCamera = cameraMgr_->GetActiveCamera();
    DebugCamera* debugCamPtr = dynamic_cast<DebugCamera*>(activeCamera);

    if (debugCamPtr) {
        // デバッグカメラ有効化とキー操作による移動更新
        debugCamPtr->SetActive(true);
        debugCamPtr->Update(input_);
    } else {
        // 通常のメインカメラの更新処理
        debugCamera_->SetActive(false);
        activeCamera->Update();
    }
}

/**
 * @brief 毎フレーム描画処理（3Dオブジェクトのレンダリングコマンド発行）
 */
void ClearScene::Draw() {
    // エフェクトの描画
    EffectManager::GetInstance()->Draw();
}

