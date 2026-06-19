#pragma once
#include "App/Scene/Core/IScene.h"
#include <memory>
#include <string>

class PostProcess;

// エンジンコンポーネントのインクルード
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/3d/Cube/CubeObject.h"
#include "Engine/Graphics/Objects/Light/Manager/LightManager.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Debug/DebugCamera.h"

/**
 * @brief ゲームオーバー画面を管理するシーンクラス
 */
class GameOverScene : public IScene {
public:
    // コンストラクタ・デストラクタ
    GameOverScene() = default;
    ~GameOverScene() override = default;

    // IScene インターフェースの仮想関数オーバーライド
    void Initialize() override;     // 初期化処理
    void ImGuiControl() override;   // デバッグ用ImGui描画処理
    void Update() override;         // 毎フレーム更新処理
    void Draw() override;           // 毎フレーム描画処理

private:
    // マジックナンバーを排除するための定数宣言
    static inline const std::string kMainCameraName = "Main";       // メインカメラの登録・選択用キー
    static inline const std::string kDebugCameraName = "Debug";     // デバッグカメラの登録・選択用キー
    static inline const std::string kCubeTextureKey = "white";      // キューブの描画に使用する白テクスチャのキー
    static inline const std::string kEmptyEnvMapKey = "";           // 環境マップなしを指定する空文字列

    // オブジェクトの初期トランスフォーム用定数（GameScene, ClearSceneと区別するためY座標を-1.0fに設定）
    static inline const Vector3 kCubeInitialPosition = { 0.0f, -1.0f, 0.0f }; // キューブの初期位置
    static inline const Vector3 kCubeInitialScale = { 1.0f, 1.0f, 1.0f };     // キューブの初期サイズ

private:
    // エンジンの各種マネージャへの生ポインタ（所有権は持たない）
    Input* input_ = nullptr;            // キー入力マネージャ
    CameraManager* cameraMgr_ = nullptr; // カメラ管理者
    LightManager* lightMgr_ = nullptr;  // ライト管理者
    PostProcess* postProcess_ = nullptr;

    // シーン内で作成・管理するオブジェクト
    std::shared_ptr<BaseCamera> mainCamera_;        // メインカメラ
    std::shared_ptr<DebugCamera> debugCamera_;      // デバッグ確認用フリーカメラ
    std::unique_ptr<DirectionalLightObject> dirLight_; // 平行光源
    std::unique_ptr<CubeObject> cube_;              // 描画用の3D立方体モデル
};
