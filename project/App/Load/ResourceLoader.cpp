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
	if (!texMgr) return;

	texMgr->LoadTexture("white", "resources/white.png");
	texMgr->LoadTexture("monsterBall", "resources/monsterBall.png");
	texMgr->LoadTexture("circle", "resources/circle.png");
	texMgr->LoadTexture("uvChecker", "resources/uvChecker.png");
	texMgr->LoadTexture("skyboxTex", "resources/rostock_laage_airport_4k.dds");
}

void ResourceLoader::LoadModels() {
	ModelManager* modelMgr = ModelResource::GetModelManager();
	if (!modelMgr) return;

	modelMgr->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
	modelMgr->LoadModel("bunny", "resources/obj/bunny/bunny.obj");
	modelMgr->LoadModel("terrain", "resources/obj/terrain/terrain.obj");
	modelMgr->LoadModel("skydome", "resources/obj/skydome/skydome.obj");
}
