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

	texMgr->LoadTexture("white", "resources/Textures/white.png");
	texMgr->LoadTexture("monsterBall", "resources/Textures/monsterBall.png");
	texMgr->LoadTexture("circle", "resources/Textures/circle.png");
	texMgr->LoadTexture("uvChecker", "resources/Textures/uvChecker.png");
	texMgr->LoadTexture("skyboxTex", "resources/Textures/skybox.dds");
	texMgr->LoadTexture("forestTex", "resources/Textures/forest.dds");
	texMgr->LoadTexture("gradationLine", "resources/Textures/gradationLine.png");
	texMgr->LoadTexture("slashTex", "resources/Textures/slashTex.png");
}

void ResourceLoader::LoadModels() {
	ModelManager* modelMgr = ModelResource::GetModelManager();
	if (!modelMgr) return;

	modelMgr->LoadModel("cube", "resources/obj/cube/cube.obj");
	modelMgr->LoadModel("teapot", "resources/obj/teapot/teapot.obj");
	modelMgr->LoadModel("bunny", "resources/obj/bunny/bunny.obj");
	modelMgr->LoadModel("terrain", "resources/obj/terrain/terrain.obj");
	modelMgr->LoadModel("skydome", "resources/obj/skydome/skydome.obj");
}
