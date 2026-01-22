#include "BaseResource.h"

Zuizui* EngineResource::sEngine = nullptr;
TextureManager* TextureResource::sTexMgr = nullptr;
Camera* CameraResource::sCamera = nullptr;
DirectionalLightObject* LightResource::sDirLight = nullptr;
PointLightObject* LightResource::sPointLight = nullptr;
SpotLightObject* LightResource::sSpotLight = nullptr;
ModelManager* ModelResource::sModelMgr = nullptr;
