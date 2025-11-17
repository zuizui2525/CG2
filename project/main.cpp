#include "Function/Function.h"
#include "DxCommon/DxCommon.h"
#include "WindowApp/WindowApp.h"
#include "Log/Log.h"
#include "Imgui/ImguiManager.h"
#include "Input/Input.h"
#include "Audio/Audio.h"
#include "PSO/PSO.h"
#include "Light/DirectionalLight/DirectionalLight.h"
#include "3d/Triangle/TriangleObject.h"
#include "2d/Sprite/SpriteObject.h"
#include "3d/Sphere/SphereObject.h"
#include "3d/Model/ModelObject.h"
#include "Camera/Camera.h"
#include "Texture/TextureManager.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakChecker leakCheck;
	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// Window
	WindowApp window;
	if (!window.Initialize(L"CreateEngine!!")) return -1;
	window.Show();

	// Log
	Log logger;
	logger.Write("Engine Start");

	// DxCommon
	DxCommon dxCommon;
	dxCommon.Initialize(window.GetHWND(), WindowApp::kClientWidth, WindowApp::kClientHeight);
	logger.Write("DxCommon Initialize");

	//PSO
	std::unique_ptr<PSO> pso = std::make_unique<PSO>(dxCommon.GetDevice(), dxCommon.GetDxcUtils(), dxCommon.GetDxcCompiler(), dxCommon.GetIncludeHandler(), logger.GetLogStream());
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
	camera->Initialize();
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
	std::unique_ptr<ModelObject> teapot = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "teapot.obj", Vector3{ 1.0f, 0.0f, 0.0f });
	logger.Write("teapot Initialize");

	// モデル生成（例: multiMaterial.obj を読み込む）
	std::unique_ptr<ModelObject> multiMaterial = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "multiMaterial.obj", Vector3{ 0.0f, 0.0f, 0.0f });
	logger.Write("MultiMaterial Initialize");

	// モデル生成（例: suzanne.obj を読み込む）
	std::unique_ptr<ModelObject> suzanne = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "suzanne.obj", Vector3{ 0.0f, 0.0f, 0.0f });
	logger.Write("Suzanne Initialize");

	// モデル生成（例: bunny.obj を読み込む）
	std::unique_ptr<ModelObject> bunny = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "bunny.obj", Vector3{ 0.0f, 0.0f, 0.0f });
	logger.Write("Bunny Initialize");

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

	textureManager->LoadTexture("teapot", teapot->GetModelData()->material.textureFilePath);
	textureManager->LoadTexture("multiMaterial", multiMaterial->GetModelData()->material.textureFilePath);
	textureManager->LoadTexture("suzanne", suzanne->GetModelData()->material.textureFilePath);
	textureManager->LoadTexture("bunny", bunny->GetModelData()->material.textureFilePath);

	// 音声出力
	SoundData soundData1 = audio.LoadSound("resources/fanfare.wav");
	audio.PlaySound(soundData1, true);

	bool drawTriangle = false;
	bool drawSprite = false;
	bool drawSphere = false;
	bool drawModel = true;
	bool drawModel2 = false;
	bool drawModel3 = false;
	bool drawModel4 = false;

	sphere->GetTransform().rotate.y = 4.7f;
	teapot->GetTransform().rotate.y = 3.0f;
	multiMaterial->GetTransform().rotate.y = 3.0f;
	suzanne->GetTransform().rotate.y = 3.0f;
	bunny->GetTransform().rotate.y = 3.0f;

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
					triangle->ImGuiControl();
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sphere")) {
				ImGui::Checkbox("Draw(Sphere)", &drawSphere);
				if (drawSphere) {
					sphere->ImGuiControl();
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
					teapot->ImGuiControl();
				}
				ImGui::Checkbox("Draw(multiMaterial)", &drawModel2);
				if (drawModel2) {
					multiMaterial->ImGuiControl();
				}
				ImGui::Checkbox("Draw(suzanne)", &drawModel3);
				if (drawModel3) {
					suzanne->ImGuiControl();
				}
				ImGui::Checkbox("Draw(bunny)", &drawModel4);
				if (drawModel4) {
					bunny->ImGuiControl();
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
		int blendIndex = static_cast<int>(pso->blendMode_);
		ImGui::Combo("BlendMode", &blendIndex, "None\0Normal\0Add\0Subtract\0Multiply\0Screen\0");
		pso->blendMode_ = static_cast<BlendMode>(blendIndex);
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
		triangle->Update(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D());

		// スプライト
		sprite->Update(camera->GetViewMatrix2D(), camera->GetProjectionMatrix2D());

		// 球
		sphere->Update(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D());

		// モデル
		teapot->Update(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D());

		// モデル2
		multiMaterial->Update(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D());

		// モデル3
		suzanne->Update(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D());

		// モデル4
		bunny->Update(camera->GetViewMatrix3D(), camera->GetProjectionMatrix3D());

		// 描画前処理
		dxCommon.BeginFrame();
		dxCommon.PreDraw(
			pso->GetPipelineState(),
			pso->GetRootSignature()
		);

		// 三角形の描画
		triangle->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("white"), dirLight->GetGPUVirtualAddress(), drawTriangle);

		// Spriteの描画
		sprite->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("uvChecker"), dirLight->GetGPUVirtualAddress(), drawSprite);

		// Sphereの描画
		sphere->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("monsterball"), dirLight->GetGPUVirtualAddress(), drawSphere);

		// Modelの描画
		teapot->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("teapot"), dirLight->GetGPUVirtualAddress(), drawModel);

		// Model2の描画
		multiMaterial->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("multiMaterial"), dirLight->GetGPUVirtualAddress(), drawModel2);

		// Model3の描画
		suzanne->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("suzanne"), dirLight->GetGPUVirtualAddress(), drawModel3);

		// Model4の描画
		bunny->Draw(dxCommon.GetCommandList(), textureManager->GetGpuHandle("bunny"), dirLight->GetGPUVirtualAddress(), drawModel4);

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
