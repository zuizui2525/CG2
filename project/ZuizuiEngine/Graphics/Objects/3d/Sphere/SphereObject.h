#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include <string>
#include "Struct.h"
#include "Object3D.h"

class Zuizui;
class Camera;
class DirectionalLightObject;

class SphereObject : public Object3D {
public:
    SphereObject() = default;
    ~SphereObject() = default;

    void Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, int lightingMode = 2);

    // 更新処理
    void Update();

    // 描画処理
    void Draw(const std::string& textureKey, bool draw = true);

    // Getter
    float GetRadius() const { return radius_; }
    uint32_t GetSubdivision() const { return subdivision_; }

    // Setter (変更があったらフラグを立てる)
    void SetRadius(float radius) { radius_ = radius; needsUpdate_ = true; }
    void SetSubdivision(uint32_t subdivision) { subdivision_ = subdivision; needsUpdate_ = true; }

private:
    // メッシュ（頂点・インデックスバッファ）の生成
    void CreateMesh();

private:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    // パラメータ
    uint32_t subdivision_ = 16;
    float radius_ = 1.0f;
    bool needsUpdate_ = false; // 再生成フラグ

    Camera* camera_ = nullptr;
    DirectionalLightObject* DirectionalLight_ = nullptr;
};
