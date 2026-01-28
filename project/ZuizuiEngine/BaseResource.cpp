#include "BaseResource.h"

Zuizui* EngineResource::sEngine = nullptr;
TextureManager* TextureResource::sTexMgr = nullptr;
Camera* CameraResource::sCamera = nullptr;
LightManager* LightResource::sLightMgr = nullptr;
ModelManager* ModelResource::sModelMgr = nullptr;

Zuizui* EngineResource::GetEngine() { return sEngine; }
TextureManager* TextureResource::GetTextureManager() { return sTexMgr; }
Camera* CameraResource::GetCamera() { return sCamera; }
LightManager* LightResource::GetLightManager() { return sLightMgr; }
ModelManager* ModelResource::GetModelManager() { return sModelMgr; }
