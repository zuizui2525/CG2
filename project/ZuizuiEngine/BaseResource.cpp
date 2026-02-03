#include "BaseResource.h"

Zuizui* EngineResource::sEngine = nullptr;
TextureManager* TextureResource::sTexMgr = nullptr;
CameraManager* CameraResource::sCameraMgr = nullptr;
LightManager* LightResource::sLightMgr = nullptr;
ModelManager* ModelResource::sModelMgr = nullptr;

Zuizui* EngineResource::GetEngine() { return sEngine; }
TextureManager* TextureResource::GetTextureManager() { return sTexMgr; }
CameraManager* CameraResource::GetCameraManager() { return sCameraMgr; }
LightManager* LightResource::GetLightManager() { return sLightMgr; }
ModelManager* ModelResource::GetModelManager() { return sModelMgr; }
