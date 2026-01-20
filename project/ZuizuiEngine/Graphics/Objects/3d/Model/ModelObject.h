#pragma once
#include "Object3D.h"
#include <string>

class Zuizui;
class Camera;
class DirectionalLightObject;
class TextureManager;
class ModelManager;

class ModelObject : public Object3D {
public:
    void Initialize(Zuizui* engine, Camera* camera, DirectionalLightObject* light, TextureManager* texMgr, ModelManager* modelMgr, int lightingMode = 2);
    void Update();
    void Draw(const std::string& modelKey, const std::string& textureKey = "", bool draw = true);

private:
    Camera* camera_ = nullptr;
    DirectionalLightObject* dirLight_ = nullptr;
    TextureManager* texMgr_ = nullptr;
    ModelManager* modelMgr_ = nullptr;
};
