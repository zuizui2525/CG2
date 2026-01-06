#pragma once
#include "ModelObject.h"
#include "Camera.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include "Struct.h"
#include <memory>

class Clear {
public:
    // 初期化：座標を受け取ってモデルを生成
    void Initialize(DxCommon* dxCommon, TextureManager* textureManager, const Vector3& position);

    // 更新：回転などの演出
    void Update(Camera* camera);

    // 描画
    void Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight);

    // 当たり判定用AABB取得
    AABB GetAABB() const;

    const Vector3& GetPosition() const { return position_; }

private:
    std::unique_ptr<ModelObject> model_;
    Vector3 position_;
    Vector3 rotation_ = { 0.0f, 0.0f, 0.0f };

    // 当たり判定のサイズ（モデルに合わせて調整してください）
    static inline const float kWidth = 0.09f;
    static inline const float kHeight = 0.09f;
};
