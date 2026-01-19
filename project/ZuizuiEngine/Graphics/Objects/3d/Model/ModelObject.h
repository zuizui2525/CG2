#pragma once
#include "Object3D.h"
#include "Zuizui.h"
#include "Struct.h"
#include <memory>
#include <string>
#include <vector>

class Zuizui;
class Camera;
class DirectionalLightObject;
class TextureManager;

// objモデル用クラス
class ModelObject : public Object3D {
public:
    ModelObject() = default;
    ~ModelObject() = default;

    // 頂点バッファビュー取得
    D3D12_VERTEX_BUFFER_VIEW GetVBV() const { return vbv_; }

    // モデルデータ取得（テクスチャパスなどを参照する用）
    const std::shared_ptr<ModelData>& GetModelData() const { return modelData_; }

    void Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, TextureManager* texture, const std::string& filename, int lightingMode = 2);

    // 毎フレーム更新（Transform から行列計算して WVP 反映）
    void Update();

    // モデル描画用関数
    void Draw(const std::string& textureKey, bool draw = true);

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    std::shared_ptr<ModelData> modelData_;

    Camera* camera_ = nullptr;
    DirectionalLightObject* dirLight_ = nullptr;
    TextureManager* texture_ = nullptr;
};
