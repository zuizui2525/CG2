#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>
#include "Struct.h"
#include "Object3D.h"

class Zuizui;
class Camera;
class DirectionalLightObject;

class TriangleObject: public Object3D {
public:
    TriangleObject() = default;
    ~TriangleObject() = default;

    void Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, int lightingMode = 0);

    // 更新処理
    void Update();

    // 描画処理
    void Draw(const std::string& textureKey, bool draw = true);
  
private:
    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};

    Camera* camera_ = nullptr;
    DirectionalLightObject* DirectionalLight_ = nullptr;
};
