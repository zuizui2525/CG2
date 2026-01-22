#pragma once
#include <Windows.h>      // WAVEFORMATEX のため
#include <mmreg.h>        // WAVEFORMATEX 詳細定義
#include <string>
#include <vector>
#include <wrl.h>          // ComPtr
#include <dxgidebug.h>    // DXGI デバッグ
#include <dxgi1_3.h>      // DXGIGetDebugInterface1 のため
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <xaudio2.h>

#pragma comment(lib, "dxguid.lib") // DXGI デバッグ用ライブラリ
#pragma comment(lib, "dxgi.lib")   // DXGI ライブラリ
#pragma comment(lib, "xaudio2.lib") // XAudio2ライブラリ
#pragma comment(lib, "mfplat.lib")  // Media Foundation
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// Vector2
struct Vector2 {
    float x;
    float y;
};

// Vector3
struct Vector3 {
    float x;
    float y;
    float z;
};

// Vector4
struct Vector4 {
    float x;
    float y;
    float z;
    float w;
};

// Matrix3x3
struct Matrix3x3 {
    float m[3][3];
};

// Matrix4x4
struct Matrix4x4 {
    float m[4][4];
};

// Transform
struct Transform {
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
};

struct Segment {
    Vector3 origin; //!< 始点
    Vector3 diff; //!< 終点への差分ベクトル
    unsigned int color; //!< 色
};

struct Triangle {
    Vector3 vertex[3]; //!< 頂点
    unsigned int color; //!< 色
};

struct Plane {
    Vector3 normal; //!< 法線
    float distance; //!< 距離
    unsigned int color; //!< 色
};

struct Sphere {
    Vector3 center; //!< 中心点
    float radius; //!< 半径
    unsigned int color; //!< 色
};

struct AABB {
    Vector3 min; //!< 最小点
    Vector3 max; //!< 最大点
    unsigned int color; //!< 色
};

struct OBB {
    Vector3 center; //!< 中心点
    Vector3 orientations[3]; //!< 座標軸「。正規化・直交必須
    Vector3 size; //!< 座標軸方向の長さの半分。中心から面までの距離
    unsigned int color; //!< 色
};

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
    float padding;
};

struct DirectionalLight {
    Vector4 color;     //!< ライトの色
    Vector3 direction; //!< ライトの方向
    float intensity;   //!< ライトの強度
};

struct PointLight {
    Vector4 color;     //!< ライトの色
    Vector3 position;  //!< ライトの座標
    float intensity;   //!< 輝度
    float radius;      //!< ライトの届く最大距離
    float decay;       //!< 減衰率（値が大きいほど急激に暗くなる）
    float padding[2];  //!< 16バイトアライメントのためのパディング
};

struct SpotLight {
    Vector4 color;             //!< ライトの色 (RGBA)
    Vector3 position;          //!< ライトの位置
    float intensity;           //!< 輝度
    Vector3 direction;         //!< ライトの方向
    float distance;            //!< ライトの届く最大距離
    float decay;               //!< 減衰率
    float cosAngle;            //!< スポットライトの余弦 (外角)
    float cosFalloffStart;     //!< 減衰開始の余弦 (内角)
    float padding;             //!< 16バイトアライメント用パディング
};

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

// D3D12 リソースリークチェッカー
struct D3DResourceLeakChecker {
    ~D3DResourceLeakChecker() {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        }
    }
};

// チャンクヘッダ
struct ChunkHeader {
    char id[4];   // チャンクID
    int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
    ChunkHeader chunk; // "RIFF"
    char type[4];      // "WAVE"
};

// FMTチャンク
struct FormatChunk {
    ChunkHeader chunk; // "fmt"
    WAVEFORMATEX fmt;  // 波形フォーマット
};
