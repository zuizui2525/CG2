#include "CameraManager.h"
#include "Zuizui.h"
#include "BaseResource.h"
#include "Function.h"
#include "WindowApp.h"
#include "Matrix.h"

void CameraManager::Initialize() {
    // Engine
    auto engine = EngineResource::GetEngine();
    assert(engine != nullptr);

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
    resource_ = CreateBufferResource(engine->GetDevice(), sizeof(CameraForGPU));
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

void CameraManager::ImGuiControl() {
#ifdef _USEIMGUI
    ImGui::Begin("Camera List");

    for (auto& [name, camera] : cameras_) {
        bool isActive = (activeCamera_ == camera.get());

        // 1. ラベルを作成 (例: "MainCamera (Active)")
        std::string label = name + (isActive ? " (Active)" : "");

        // 2. ラジオボタンを縦に並べる (SameLineを呼ばない)
        // ##name をつけることで、内部IDを固定し、表示文字列が変わっても挙動を安定させる
        if (ImGui::RadioButton((label + "##" + name).c_str(), isActive)) {
            SetActiveCamera(name);
        }

        // 3. 設定ウィンドウを開くチェックボックスは少し右にずらすか、名前の横に置く
        ImGui::SameLine(ImGui::GetWindowWidth() - 100); // 右側に寄せる
        camera->ImGuiControl(name); // BaseCamera側のチェックボックス

        ImGui::Separator(); // カメラごとに区切り線を入れると見やすい
    }

    ImGui::End();
#endif
}

void CameraManager::Clear() {
    cameras_.clear();
    activeCamera_ = nullptr;
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
