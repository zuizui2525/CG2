#pragma once
#include "Object3D.h"
#include "BaseResource.h"
#include <string>

class ModelObject : public Object3D, public BaseModel {
public:
    void Initialize(int lightingMode = 2);
    void Update();
    void Draw(const std::string& modelKey, const std::string& textureKey = "", bool draw = true);
};
