#include "BaseResource.h"

Zuizui* EngineResource::sEngine = nullptr;
Input* InputResource::sInput = nullptr;
TextureManager* TextureResource::sTexMgr = nullptr;
CameraManager* CameraResource::sCameraMgr = nullptr;
LightManager* LightResource::sLightMgr = nullptr;
ModelManager* ModelResource::sModelMgr = nullptr;

Zuizui* EngineResource::GetEngine() { return sEngine; }
Input* InputResource::GetInput() { return sInput; }
TextureManager* TextureResource::GetTextureManager() { return sTexMgr; }
CameraManager* CameraResource::GetCameraManager() { return sCameraMgr; }
LightManager* LightResource::GetLightManager() { return sLightMgr; }
ModelManager* ModelResource::GetModelManager() { return sModelMgr; }
