#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Engine/Math/MathStructs.h"
#include "Engine/Debug/IGameObject.h"

static const int kMaxDirectionalLights = 2;

struct DirectionalLight {
    Vector4 color;     //!< ライトの色
    Vector3 direction; //!< ライトの方向
    float intensity;   //!< ライトの強度
};

struct DirectionalLightGroup {
    DirectionalLight lights[kMaxDirectionalLights]; // 配列
    int32_t numLights;                              // 有効なライト数
    float padding[3];                               // アライメント調整
};

class DirectionalLightObject : public IGameObject {
public:
    virtual ~DirectionalLightObject();

    // 初期化
    void Initialize();

    // 毎フレーム更新（正規化とか）
    void Update();

    // ImGui操作
    void DrawInspector() override;

    // 実体の参照を返す
    DirectionalLight& GetLightData() { return data_; }

    // デバッグ・エディット用トランスフォーム
    Vector3& GetPosition() { return position_; }
    void SetPosition(const Vector3& pos) { position_ = pos; }
    Vector3& GetRotate() { return rotation_; }
    void SetRotate(const Vector3& rot) { rotation_ = rot; }
    Vector3 GetScale() const { return {1.0f, 1.0f, 1.0f}; } // ギズモ互換用
    void SetScale(const Vector3& scale) { (void)scale; }

private:
    DirectionalLight data_ = { {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}, 1.0f };
    Vector3 position_ = { 0.0f, 10.0f, 0.0f };
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };
};
