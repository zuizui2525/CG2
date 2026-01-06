#pragma once
#include "ModelObject.h"
#include "Camera.h"
#include "DxCommon.h"
#include "TextureManager.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include <memory>

class Skydome {
public:
    void Initialize(DxCommon* dxCommon, TextureManager* textureManager);
    void Update(Camera* camera);
    void Draw(DxCommon* dxCommon, TextureManager* textureManager, PSOManager* psoManager, DirectionalLightObject* dirLight);

private:
    std::unique_ptr<ModelObject> model_;
};
