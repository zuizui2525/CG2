#include "Function.h"
#include "DxCommon.h"
#include "WindowApp.h"
#include "Log.h"
#include "ImguiManager.h"
#include "Input.h"
#include "Audio.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include "TriangleObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "ModelObject.h"
#include "Camera.h"
#include "TextureManager.h"
#include "ParticleManager.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakChecker leakCheck;
	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// Window
	WindowApp window;
	if (!window.Initialize(L"LE2B_02_イトウカズイ")) return -1;
	window.Show();

	// Log
	Log logger;
	logger.Write("Engine Start");

	// DxCommon
	DxCommon dxCommon;
	dxCommon.Initialize(window.GetHWND(), WindowApp::kClientWidth, WindowApp::kClientHeight);
	logger.Write("DxCommon Initialize");

	//PSO
	std::unique_ptr<PSOManager> psoManager = std::make_unique<PSOManager>(dxCommon.GetDevice());
	psoManager->Initialize(
		dxCommon.GetDxcUtils(),
		dxCommon.GetDxcCompiler(),
		dxCommon.GetIncludeHandler()
	);
	logger.Write("PSO Initialize");

	// Input
	std::unique_ptr<Input> input = std::make_unique<Input>();
	input->Initialize(window.GetInstance(), window.GetHWND());
	logger.Write("Input Initialize");

	// Audio
	Audio audio;
	audio.Initialize();
	logger.Write("Audio Initialize");

	// Camera
	std::unique_ptr<Camera> camera = std::make_unique<Camera>();
	camera->Initialize(dxCommon.GetDevice());
	logger.Write("Camera Initialize");

	// DirectionalLight
	std::unique_ptr<DirectionalLightObject> dirLight = std::make_unique<DirectionalLightObject>();
	dirLight->Initialize(dxCommon.GetDevice());
	logger.Write("DirectionalLight Initialize");

	// 三角形の初期化
	std::unique_ptr<TriangleObject> triangle = std::make_unique<TriangleObject>(dxCommon.GetDevice());
	logger.Write("Triangle Initialize");

	// スプライトの初期化
	std::unique_ptr<SpriteObject> sprite = std::make_unique<SpriteObject>(dxCommon.GetDevice(), 640, 360);
	logger.Write("Sprite Initialize");

	// 球の初期化
	std::unique_ptr<SphereObject> sphere = std::make_unique<SphereObject>(dxCommon.GetDevice(), 16, 1.0f);
	logger.Write("Sphere Initialize");

	// モデル生成（例: teapot.obj を読み込む）
	std::unique_ptr<ModelObject> teapot = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources/obj/teapot/teapot.obj", Vector3{ 1.0f, 0.0f, 0.0f });
	logger.Write("teapot Initialize");

	// モデル生成（例: multiMaterial.obj を読み込む）
	std::unique_ptr<ModelObject> multiMaterial = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources/obj/multiMaterial/multiMaterial.obj", Vector3{ 0.0f, 0.0f, 0.0f });
	logger.Write("MultiMaterial Initialize");

	// モデル生成（例: suzanne.obj を読み込む）
	std::unique_ptr<ModelObject> suzanne = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources/obj/suzanne/suzanne.obj", Vector3{ 0.0f, 0.0f, 0.0f });
	logger.Write("Suzanne Initialize");

	// モデル生成（例: bunny.obj を読み込む）
	std::unique_ptr<ModelObject> bunny = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources/obj/bunny/bunny.obj", Vector3{ 0.0f, 0.0f, 0.0f });
	logger.Write("Bunny Initialize");

	// パーティクル
	std::unique_ptr<ParticleManager> particle = std::make_unique<ParticleManager>(&dxCommon, Vector3{ -10.0f, 0.0f, 20.0f}, 10);
	logger.Write("Particle Initialize");

	// パーティクル2
	std::unique_ptr<ParticleManager> particle2 = std::make_unique<ParticleManager>(&dxCommon, Vector3{ -5.0f, 0.0f, 20.0f });
	logger.Write("Particle2 Initialize");

	// パーティクル3
	std::unique_ptr<ParticleManager> particle3 = std::make_unique<ParticleManager>(&dxCommon, Vector3{ 0.0f, 0.0f, 20.0f }, 1000);
	logger.Write("Particle3 Initialize");

	// パーティクル4
	std::unique_ptr<ParticleManager> particle4 = std::make_unique<ParticleManager>(&dxCommon, Vector3{ 5.0f, 0.0f, 20.0f }, 10000);
	logger.Write("Particle4 Initialize");

	// パーティクル5
	std::unique_ptr<ParticleManager> particle5 = std::make_unique<ParticleManager>(&dxCommon, Vector3{ 10.0f, 0.0f, 20.0f }, 100000);
	logger.Write("Particle5 Initialize");

#ifdef _DEBUG
	// Imgui
	std::unique_ptr<ImguiManager> imgui = std::make_unique<ImguiManager>();
	imgui->Initialize(
		window.GetHWND(),
		dxCommon.GetDevice(),
		dxCommon.GetBackBufferCount(),
		dxCommon.GetRtvFormat(),
		dxCommon.GetRtvHeap(),
		dxCommon.GetSrvHeap());
	logger.Write("Imgui Initialize");
#endif

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = dxCommon.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = dxCommon.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = dxCommon.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeRTV, 0);

	// テクスチャ関連
	std::unique_ptr<TextureManager> textureManager = std::make_unique<TextureManager>();
	textureManager->Initialize(dxCommon.GetDevice(), dxCommon.GetCommandList(), dxCommon.GetSrvHeap());
	logger.Write("Texture Initialize");

	textureManager->LoadTexture("uvChecker", "resources/uvChecker.png");
	textureManager->LoadTexture("monsterball", "resources/monsterball.png");
	textureManager->LoadTexture("white", "resources/white.png");
	textureManager->LoadTexture("circle", "resources/circle.png");

	textureManager->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);
	textureManager->LoadTexture("multiMaterial", multiMaterial->GetModelData()->material.textureFilePath);
	textureManager->LoadTexture("suzanne", suzanne->GetModelData()->material.textureFilePath);
	textureManager->LoadTexture("bunny", bunny->GetModelData()->material.textureFilePath);

	// 音声出力
	SoundData soundData1 = audio.LoadSound("resources/bgm.mp3");
	audio.PlaySound(soundData1, 1.0f, true);

	bool drawTriangle = false;
	bool drawSprite = false;
	bool drawSphere = false;
	bool drawModel = false;
	bool drawModel2 = false;
	bool drawModel3 = false;
	bool drawModel4 = false;
	bool drawParticle = false;
	bool drawParticle2 = false;
	bool drawParticle3 = true;
	bool drawParticle4 = false;
	bool drawParticle5 = false;

	sphere->GetTransform().rotate.y = 4.7f;
	teapot->GetTransform().rotate.y = 3.0f;
	multiMaterial->GetTransform().rotate.y = 3.0f;
	suzanne->GetTransform().rotate.y = 3.0f;
	bunny->GetTransform().rotate.y = 3.0f;

	static BlendMode currentBlendMode = kBlendModeNormal;
	const char* items = "None\0Normal\0Add\0Subtract\0Multiply\0Screen\0";

	// ゲームループ
	while (window.ProcessMessage()) {
		// ゲームの処理
		// フレームの開始時刻を記録
		dxCommon.FrameStart();
		// 開始
#ifdef _DEBUG
		imgui->Begin();
		ImGui::Begin("infomation");
		if (ImGui::BeginTabBar("infomation")) {
			if (ImGui::BeginTabItem("Camera & DirectionalLight")) {
				camera->ImGuiControl();
				dirLight->ImGuiControl();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Triangle")) {
				ImGui::Checkbox("Draw(Triangle)", &drawTriangle);
				if (drawTriangle) {
					triangle->ImGuiControl("Triangle");
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sphere")) {
				ImGui::Checkbox("Draw(Sphere)", &drawSphere);
				if (drawSphere) {
					sphere->ImGuiControl("Sphere");
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sprite")) {
				ImGui::Checkbox("Draw(Sprite)", &drawSprite);
				if (drawSprite) {
					sprite->ImGuiControl();
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Model")) {
				ImGui::Checkbox("Draw(teapot)", &drawModel);
				if (drawModel) {
					teapot->ImGuiControl("teapot");
				}
				ImGui::Checkbox("Draw(multiMaterial)", &drawModel2);
				if (drawModel2) {
					multiMaterial->ImGuiControl("multiMaterial");
				}
				ImGui::Checkbox("Draw(suzanne)", &drawModel3);
				if (drawModel3) {
					suzanne->ImGuiControl("suzanne");
				}
				ImGui::Checkbox("Draw(bunny)", &drawModel4);
				if (drawModel4) {
					bunny->ImGuiControl("bunny");
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Particles")) {
				ImGui::Checkbox("Draw(particle)", &drawParticle);
				if (drawParticle) {
					particle->ImGuiControl("particle");
				}
				ImGui::Checkbox("Draw(particle2)", &drawParticle2);
				if (drawParticle2) {
					particle2->ImGuiControl("particle2");
				}
				ImGui::Checkbox("Draw(particle3)", &drawParticle3);
				if (drawParticle3) {
					particle3->ImGuiControl("particle3");
				}
				ImGui::Checkbox("Draw(particle4)", &drawParticle4);
				if (drawParticle4) {
					particle4->ImGuiControl("particle4");
				}
				ImGui::Checkbox("Draw(particle5)", &drawParticle5);
				if (drawParticle5) {
					particle5->ImGuiControl("particle5");
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		ImGui::Begin("FPS");
		ImGui::Text("FPS: %.1f", 1.0f / dxCommon.GetDeltaTime());
		ImGui::End();

		ImGui::Begin("BlendMode");
		int blendIndex = static_cast<int>(currentBlendMode);

		if (ImGui::Combo("BlendMode", &blendIndex, items)) {
			// ユーザーが変更した場合
			BlendMode newMode = static_cast<BlendMode>(blendIndex);

			// PSOManagerの専用関数を呼び出す
			HRESULT hr = psoManager->UpdateBlendMode("Object3D", newMode);

			if (SUCCEEDED(hr)) {
				// 成功した場合のみ状態を更新
				currentBlendMode = newMode;
			} else {
				// エラー処理 (例: ログ出力)
			}
		}
		ImGui::End();
		
		// 終了
		imgui->End();
#endif
		// Inputの更新処理
		input->Update();

		// Camera
		camera->Update(input.get());

		//directionalLight
		dirLight->Update();

		// 三角形の回転処理
		triangle->GetTransform().rotate.y += 0.03f;

		// 三角形
		triangle->Update(camera.get());

		// スプライト
		sprite->Update(camera.get());

		// 球
		sphere->Update(camera.get());

		// モデル
		teapot->Update(camera.get());

		// モデル2
		multiMaterial->Update(camera.get());

		// モデル3
		suzanne->Update(camera.get());

		// モデル4
		bunny->Update(camera.get());

		// パーティクル
		particle->Update(camera.get());

		// パーティクル2
		particle2->Update(camera.get());

		// パーティクル3
		particle3->Update(camera.get());

		// パーティクル4
		particle4->Update(camera.get());

		// パーティクル5
		particle5->Update(camera.get());

		// 描画前処理
		dxCommon.BeginFrame();
		dxCommon.PreDraw();

		// 三角形の描画
		triangle->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("white"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawTriangle);

		// Spriteの描画
		sprite->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("uvChecker"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawSprite);

		// Sphereの描画
		sphere->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("monsterball"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawSphere);

		// Modelの描画
		teapot->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("teapot"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawModel);

		// Model2の描画
		multiMaterial->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("multiMaterial"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawModel2);

		// Model3の描画
		suzanne->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("suzanne"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawModel3);

		// Model4の描画
		bunny->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("bunny"), dirLight->GetGPUVirtualAddress(), camera->GetGPUVirtualAddress(), psoManager->GetPSO("Object3D"), psoManager->GetRootSignature("Object3D"), drawModel4);

		// パーティクルの描画
		particle->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("circle"), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), drawParticle);

		// パーティクル2の描画
		particle2->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("circle"), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), drawParticle2);

		// パーティクル3の描画
		particle3->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("circle"), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), drawParticle3);

		// パーティクル4の描画
		particle4->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("circle"), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), drawParticle4);
		
		// パーティクル5の描画
		particle5->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("circle"), dirLight->GetGPUVirtualAddress(), psoManager->GetPSO("Particle"), psoManager->GetRootSignature("Particle"), drawParticle5);

		// ImGui表示
		dxCommon.DrawImGui();

		// 描画後処理
		dxCommon.EndFrame();

		// 待機処理を行い、FPS固定
		dxCommon.FrameEnd(60);
	}

	// ImGuiの終了処理
#ifdef _DEBUG
	imgui->Shutdown();
#endif
	// 音の終了処理
	audio.Unload(soundData1);

	// COMの終了処理
	CoUninitialize();

	return 0;
}
