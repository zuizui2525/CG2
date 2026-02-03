#include "CameraManager.h"
#include "Function.h"
#include "WindowApp.h"
#include "Matrix.h"

void CameraManager::Initialize(ID3D12Device* device) {
    // --- 1. 2D用行列の初期化 (固定) ---
    viewMatrix2D_ = Math::MakeIdentity();
    projectionMatrix2D_ = Math::MakeOrthographicMatrix(
        0.0f, 0.0f,
        static_cast<float>(WindowApp::kClientWidth),
        static_cast<float>(WindowApp::kClientHeight),
        0.0f, 100.0f
    );

    // --- 2. GPU転送用リソースの作成 ---
    // CameraForGPU構造体のサイズでバッファを確保
    resource_ = CreateBufferResource(device, sizeof(CameraForGPU));
    // 常時マッピングしておく
    resource_->Map(0, nullptr, reinterpret_cast<void**>(&data_));
}

void CameraManager::Update() {
    // アクティブなカメラがなければ何もしない
    if (!activeCamera_) return;

    // --- 3. 3Dカメラ(アクティブ)の固有更新 ---
    // ここで各カメラクラス(DebugCamera等)の行列計算が行われる
    activeCamera_->Update();

    // --- 4. GPU定数バッファへのデータ転送 ---
    // 定数バッファは、常に「現在アクティブなカメラ」の情報を指すように上書き
    data_->view = activeCamera_->GetViewMatrix();
    data_->projection = activeCamera_->GetProjectionMatrix();
    data_->worldPosition = activeCamera_->GetPosition();
}

void CameraManager::AddCamera(const std::string& name, std::shared_ptr<BaseCamera> camera) {
    cameras_[name] = camera;

    // 最初の1つ目が登録されたら、自動的にそれをアクティブにする
    if (!activeCamera_) {
        activeCamera_ = camera.get();
    }
}

void CameraManager::SetActiveCamera(const std::string& name) {
    // 指定された名前のカメラがマップに存在するか確認
    auto it = cameras_.find(name);
    if (it != cameras_.end()) {
        activeCamera_ = it->second.get();
    }
}
