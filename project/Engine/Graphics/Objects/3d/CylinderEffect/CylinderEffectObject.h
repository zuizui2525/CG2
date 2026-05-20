#pragma once
#include "Engine/Graphics/Objects/3d/Object3D.h"

// 側面のみの円柱（筒型）エフェクト用Primitive
class CylinderEffectObject : public Object3D {
public:
    // 分割数、半径、高さを指定して初期化
    CylinderEffectObject(uint32_t subdivision = 32, float radius = 1.0f, float height = 2.0f);
    ~CylinderEffectObject() = default;

    void Initialize(int lightingMode);
    void Update();
    void Draw(const std::string& textureKey = "white", const std::string& envMapKey = "");

    // パラメータ変更用セッター
    void SetSubdivision(uint32_t subdivision) { subdivision_ = subdivision; needsUpdate_ = true; }
    void SetRadius(float radius) { radius_ = radius; needsUpdate_ = true; }
    void SetHeight(float height) { height_ = height; needsUpdate_ = true; }

private:
    void CreateMesh();

private:
    uint32_t subdivision_;
    float radius_;
    float height_;

    bool needsUpdate_ = false;

    // GPUリソース
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;

    // バッファビュー
    D3D12_VERTEX_BUFFER_VIEW vbView_{};
    D3D12_INDEX_BUFFER_VIEW ibView_{};

    uint32_t indexCount_ = 0;
};
