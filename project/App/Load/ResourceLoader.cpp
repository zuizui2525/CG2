#include "App/Load/ResourceLoader.h"
#include "Engine/Base/BaseResource.h"
#include "Engine/Graphics/Texture/TextureManager.h"
#include "Engine/Graphics/Objects/3d/Model/ModelManager.h"

void ResourceLoader::LoadAll() {
    LoadTextures();
    LoadModels();
}

void ResourceLoader::LoadTextures() {
    TextureManager* texMgr = TextureResource::GetTextureManager();

    if (texMgr) {
        texMgr->LoadTexture("white", "resources/white.png");
        texMgr->LoadTexture("monsterBall", "resources/monsterBall.png");
        texMgr->LoadTexture("circle", "resources/circle.png");
        texMgr->LoadTexture("uvChecker", "resources/uvChecker.png");
    }
}

void ResourceLoader::LoadModels() {
    ModelManager* modelMgr = ModelResource::GetModelManager();

    if (modelMgr) {
        modelMgr->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
        modelMgr->LoadModel("bunny", "resources/obj/bunny/bunny.obj");
        modelMgr->LoadModel("terrain", "resources/obj/terrain/terrain.obj");
        modelMgr->LoadModel("skydome", "resources/obj/skydome/skydome.obj");
    }
}
