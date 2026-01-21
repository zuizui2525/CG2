#pragma once

class Zuizui;
class Camera;
class DirectionalLightObject;
class PointLightObject;
class TextureManager;
class ModelManager;

class EngineResource {
public:
    static void SetEngine(Zuizui* engine) { sEngine = engine; }
protected:
    static Zuizui* sEngine;
};

class TextureResource {
public:
    static void SetTextureManager(TextureManager* texMgr) { sTexMgr = texMgr; }
protected:
    static TextureManager* sTexMgr;
};

class CameraResource {
public:
    static void SetCamera(Camera* camera) { sCamera = camera; }
protected:
    static Camera* sCamera;
};

class LightResource {
public:
    static void SetLight(DirectionalLightObject* light) { sDirLight = light; }
    static void SetPointLight(PointLightObject* light) { sPointLight = light; }
protected:
    static DirectionalLightObject* sDirLight;
    static PointLightObject* sPointLight;
};

class ModelResource {
public:
    static void SetModelManager(ModelManager* modelMgr) { sModelMgr = modelMgr; }
protected:
    static ModelManager* sModelMgr;
};

// 2D用 (エンジン、テクスチャ、カメラ)
class Base2D : public EngineResource, public TextureResource, public CameraResource {};

// 3D用 (エンジン、テクスチャ、カメラ、ライト)
class Base3D : public EngineResource, public TextureResource, public CameraResource, public LightResource {};

// モデル用 (モデルマネージャのみ。Object3Dと多重継承する用)
class BaseModel : public ModelResource {};
