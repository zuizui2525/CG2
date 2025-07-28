#pragma once

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

//Vector4
struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};

//Matrix3x3
struct Matrix3x3 {
	float m[3][3];
};

//Matrix4x4
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
	float radius; //!< 半径
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
	Vector4 color; //!< ライトの色
	Vector3 direction; //!< ライトの方向
	float intensity; //!< ライトの強度
};

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		// リソースリークチェック
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};