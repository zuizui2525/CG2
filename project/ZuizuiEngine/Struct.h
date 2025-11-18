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

struct VertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct Sphere {
    Vector3 center; //!< 中心点
    float radius;   //!< 半径
};

struct Material {
    Vector4 color;
    int32_t enableLighting;
    float padding[3];
    Matrix4x4 uvtransform;
};

struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 world;
};

struct DirectionalLight {
    Vector4 color;     //!< ライトの色
    Vector3 direction; //!< ライトの方向
    float intensity;   //!< ライトの強度
};

struct MaterialData {
    std::string textureFilePath;
};

struct ModelData {
    std::vector<VertexData> vertices;
    MaterialData material;
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

// 音声データ
struct SoundData {
    std::vector<BYTE> audioData;     //!< 音声データ本体
    WAVEFORMATEX* wfex = nullptr;    //!< フォーマット情報
    IXAudio2SourceVoice* sourceVoice = nullptr; //!< ソースボイス
};
