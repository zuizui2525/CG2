#pragma once
#include "ModelObject.h"
#include "Camera.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include "Input.h"
#include <memory>

class Player {
public:
    void Initialize(DxCommon* dxCommon, TextureManager* textureManager, Input* input);
    void Update(Camera* camera);
    void Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight);

private:
    Input* input_ = nullptr;
    std::unique_ptr<ModelObject> model_;
    Vector3 position_{};
    float speed_ = 0.1f;
};
