#pragma once
#include "Engine/Math/MathStructs.h"
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <string>

// --- 定数バッファ用構造体 (16バイトアライメントに注意) ---

struct Material {
    Vector4 color;
    int32_t enableLighting;
    float padding[3];
    Matrix4x4 uvtransform;
    float shininess;
};

struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 world;
    Matrix4x4 WorldInverseTranspose;
};

struct ParticleForGPU {
    Matrix4x4 WVP;
    Matrix4x4 world;
    Vector4 color;
};

struct CameraForGPU {
    Vector3 worldPosition;
    float padding; // 16バイト境界合わせ
};

// --- モデルデータ・頂点定義 ---

struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct MaterialData {
    std::string textureFilePath;
};

struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData material;
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
    D3D12_VERTEX_BUFFER_VIEW vbv;
};
