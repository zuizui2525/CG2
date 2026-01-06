#include "Function.h"
#include "DxCommon.h"
#include "WindowApp.h"
#include "Log.h"
#include "ImguiManager.h"
#include "Input.h"
#include "Audio.h"
#include "PSOManager.h"
#include "DirectionalLight.h"
#include "App/Scene/SceneManager.h"
#include "TextureManager.h"
#include "ParticleManager.h"
#include <ctime>

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakChecker leakCheck;
	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// Window
	WindowApp window;
	if (!window.Initialize(L"LE2B_02_イトウカズイ_岩集")) return -1;
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

	// SceneManager
	SceneManager sceneManager;
	sceneManager.Initialize(SceneLabel::Title, &dxCommon, psoManager.get(), textureManager.get(), input.get());

	// ランダム
	srand(static_cast<unsigned int>(time(nullptr)));

	// ゲームループ
	while (window.ProcessMessage()) {
		// ゲームの処理
		// フレームの開始時刻を記録
		dxCommon.FrameStart();
#ifdef _DEBUG
		// 開始
		imgui->Begin();

		// ImGui
		sceneManager.ImGuiControl();

		// 終了
		imgui->End();
#endif
		// Inputの更新処理
		input->Update();

		// SceneManager
		sceneManager.Update();

		// 描画前処理
		dxCommon.BeginFrame();
		dxCommon.PreDraw();

		// SceneManager
		sceneManager.Draw();

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
	
	// COMの終了処理
	CoUninitialize();

	return 0;
}
