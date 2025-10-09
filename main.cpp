#include "Function.h"
#include "DxCommon.h"
#include "WindowApp.h"
#include "Input.h"
#include "PSO.h"
#include "TriangleObject.h"
#include "SpriteObject.h"
#include "SphereObject.h"
#include "ModelObject.h"
#include "DebugCamera.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	D3DResourceLeakChecker leakCheck;
	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// Window
	WindowApp window;
	if (!window.Initialize(L"CreateEngine!!")) return -1;
	window.Show();

	// Audio
	Microsoft::WRL::ComPtr<IXAudio2> xAudio2;
	IXAudio2MasteringVoice* masterVoice;

	// ログのディレクトリ
	std::filesystem::create_directory("logs");
	// 現在時刻を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	// ログファイルの名前にコンマ何秒はいらないので、削って秒にする
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	// 日本時間(PCの設定時間)に変換
	std::chrono::zoned_time localTime{ std::chrono::current_zone(), nowSeconds };
	// formalを使って年月日_時分秒の文字列に変換
	std::string dateString = std::format("{:%Y%m%d_%H%M%S}", localTime);
	// 時刻を使ってファイル名を決定
	std::string logFileName = std::string("logs/") + dateString + ".log";
	// ファイルを作って書き込み準備
	std::ofstream logStream(logFileName);

	// エラー放置ダメ絶対
#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController; // デバッグ用のコントローラ
	// デバッグレイヤーのインターフェースを取得する
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf())))) {
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// コンプライアンスのチェックを行う
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// dxCommonを用意
	DxCommon dxCommon;
	dxCommon.Initialize(window.GetHWND(), WindowApp::kClientWidth, WindowApp::kClientHeight);

	//PSOを作成する
	PSO* pso = new PSO(dxCommon.GetDevice(), dxCommon.GetDxcUtils(), dxCommon.GetDxcCompiler(), dxCommon.GetIncludeHandler(), logStream);
	
	// XAudioエンジンのインスタンスを生成
	HRESULT hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));
	// マスターボイスを生成
	hr = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(hr));

	// Inputの初期化
	std::unique_ptr<Input> input = std::make_unique<Input>();
	input->Initialize(window.GetInstance(), window.GetHWND());

	Transform cameraTransform = { { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,0.0f,-7.0f } };

	D3D12_VIEWPORT viewport{};
	viewport.Width = WindowApp::kClientWidth;
	viewport.Height = WindowApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = WindowApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WindowApp::kClientHeight;

	// 三角形の初期化
	std::unique_ptr<TriangleObject> triangle = std::make_unique<TriangleObject>(dxCommon.GetDevice());

	// スプライトの初期化
	std::unique_ptr<SpriteObject> sprite = std::make_unique<SpriteObject>(dxCommon.GetDevice(), 640, 360);

	// 球の初期化
	std::unique_ptr<SphereObject> sphere = std::make_unique<SphereObject>(dxCommon.GetDevice(), 16, 1.0f);

	// モデル生成（例: teapot.obj を読み込む）
	std::unique_ptr<ModelObject> teapot = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "teapot.obj", Vector3{ 1.0f, 0.0f, 0.0f });

	// モデル生成（例: multiMaterial.obj を読み込む）
	std::unique_ptr<ModelObject> multiMaterial = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "multiMaterial.obj", Vector3{ 0.0f, 0.0f, 0.0f });

	// モデル生成（例: suzanne.obj を読み込む）
	std::unique_ptr<ModelObject> suzanne = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "suzanne.obj", Vector3{ 0.0f, 0.0f, 0.0f });

	// モデル生成（例: bunny.obj を読み込む）
	std::unique_ptr<ModelObject> bunny = std::make_unique<ModelObject>(dxCommon.GetDevice(), "resources", "bunny.obj", Vector3{ 0.0f, 0.0f, 0.0f });

	// DirectionalLight
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = CreateBufferResource(dxCommon.GetDevice(), sizeof(DirectionalLight));
	DirectionalLight* directionalLightData;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;

	// ImGuiの初期化。詳細はさして重要ではないので解説は省略する。
	// こういうもん
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window.GetHWND());
	ImGui_ImplDX12_Init(dxCommon.GetDevice(),
		dxCommon.GetBackBufferCount(),
		dxCommon.GetRtvFormat(),
		dxCommon.GetRtvHeap(),
		dxCommon.GetSrvHeap()->GetCPUDescriptorHandleForHeapStart(),
		dxCommon.GetSrvHeap()->GetGPUDescriptorHandleForHeapStart());

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = dxCommon.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = dxCommon.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = dxCommon.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeRTV, 0);

	// 1枚目のTextureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(dxCommon.GetDevice(), metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource.Get(), mipImages, dxCommon.GetDevice(), dxCommon.GetCommandList());
	// 2枚目のTextureを読んで転送する(2)
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterball.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(dxCommon.GetDevice(), metadata2);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2.Get(), mipImages2, dxCommon.GetDevice(), dxCommon.GetCommandList());
	// 3枚目のTextureを読んで転送する(3)
	DirectX::ScratchImage mipImages3 = LoadTexture("resources/white.png");
	const DirectX::TexMetadata& metadata3 = mipImages3.GetMetadata(); // mipImagesからmipImages3に変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource3 = CreateTextureResource(dxCommon.GetDevice(), metadata3);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource3 = UploadTextureData(textureResource3.Get(), mipImages3, dxCommon.GetDevice(), dxCommon.GetCommandList());
	// ModelのTextureを読んで転送する(Model)
	DirectX::ScratchImage mipImagesModel = LoadTexture(teapot->GetModelData().material.textureFilePath);
	const DirectX::TexMetadata& metadataModel = mipImagesModel.GetMetadata(); // mipImagesからmipImagesModelに変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel = CreateTextureResource(dxCommon.GetDevice(), metadataModel);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel = UploadTextureData(textureResourceModel.Get(), mipImagesModel, dxCommon.GetDevice(), dxCommon.GetCommandList());
	// ModelのTextureを読んで転送する(Model2)
	DirectX::ScratchImage mipImagesModel2 = LoadTexture(multiMaterial->GetModelData().material.textureFilePath);
	const DirectX::TexMetadata& metadataModel2 = mipImagesModel2.GetMetadata(); // mipImagesからmipImagesModelに変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel2 = CreateTextureResource(dxCommon.GetDevice(), metadataModel2);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel2 = UploadTextureData(textureResourceModel2.Get(), mipImagesModel2, dxCommon.GetDevice(), dxCommon.GetCommandList());
	// ModelのTextureを読んで転送する(Model3)
	// 分岐の外で宣言（←ここが重要）
	DirectX::ScratchImage mipImagesModel3;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel3;
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel3;
	if (suzanne->GetModelData().material.textureFilePath.empty()) {
		// テクスチャがない場合：白画像を使う
		mipImagesModel3 = LoadTexture("resources/white.png");
	} else {
		// テクスチャがある場合
		mipImagesModel3 = LoadTexture(suzanne->GetModelData().material.textureFilePath);
	}
	// 共通処理（読み込んだ mipImages を使ってリソースを作る）
	const DirectX::TexMetadata& metadataModel3 = mipImagesModel3.GetMetadata();
	textureResourceModel3 = CreateTextureResource(dxCommon.GetDevice(), metadataModel3);
	intermediateResourceModel3 = UploadTextureData(
		textureResourceModel3.Get(), mipImagesModel3, dxCommon.GetDevice(), dxCommon.GetCommandList()
	);

	// ModelのTextureを読んで転送する(Model4)
	DirectX::ScratchImage mipImagesModel4 = LoadTexture(bunny->GetModelData().material.textureFilePath);
	const DirectX::TexMetadata& metadataModel4 = mipImagesModel4.GetMetadata(); // mipImagesからmipImagesModelに変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel4 = CreateTextureResource(dxCommon.GetDevice(), metadataModel4);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel4 = UploadTextureData(textureResourceModel4.Get(), mipImagesModel4, dxCommon.GetDevice(), dxCommon.GetCommandList());

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format; // フォーマット
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels); // ミップマップの数
	// metaDataを基にSRVの設定(2)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format; // フォーマット
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels); // ミップマップの数
	// metaDataを基にSRVの設定(3)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc3{};
	srvDesc3.Format = metadata3.format; // フォーマット
	srvDesc3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDesc3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc3.Texture2D.MipLevels = UINT(metadata3.mipLevels); // ミップマップの数
	// metaDataを基にSRVの設定(Model)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescModel{};
	srvDescModel.Format = metadataModel.format; // フォーマット
	srvDescModel.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDescModel.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDescModel.Texture2D.MipLevels = UINT(metadataModel.mipLevels); // ミップマップの数
	// metaDataを基にSRVの設定(Model2)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescModel2{};
	srvDescModel2.Format = metadataModel2.format; // フォーマット
	srvDescModel2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDescModel2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDescModel2.Texture2D.MipLevels = UINT(metadataModel2.mipLevels); // ミップマップの数
	// metaDataを基にSRVの設定(Model3)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescModel3{};
	srvDescModel3.Format = metadataModel3.format; // フォーマット
	srvDescModel3.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDescModel3.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDescModel3.Texture2D.MipLevels = UINT(metadataModel3.mipLevels); // ミップマップの数
	// metaDataを基にSRVの設定(Model4)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescModel4{};
	srvDescModel4.Format = metadataModel4.format; // フォーマット
	srvDescModel4.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング
	srvDescModel4.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDescModel4.Texture2D.MipLevels = UINT(metadataModel4.mipLevels); // ミップマップの数

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 1);
	// SRVを作成する
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResource.Get(), // テクスチャリソース
		&srvDesc, // SRVの設定
		textureSrvHandleCPU); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(2)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 2);
	// SRVを作成する(2)
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResource2.Get(), // テクスチャリソース
		&srvDesc2, // SRVの設定
		textureSrvHandleCPU2); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(3)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU3 = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 3);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU3 = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 3);
	// SRVを作成する(3)
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResource3.Get(), // テクスチャリソース
		&srvDesc3, // SRVの設定
		textureSrvHandleCPU3); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 4);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 4);
	// SRVを作成する(Model)
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResourceModel.Get(), // テクスチャリソース
		&srvDescModel, // SRVの設定
		textureSrvHandleCPUModel); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model2)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel2 = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 5);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel2 = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 5);
	// SRVを作成する(Model2)
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResourceModel2.Get(), // テクスチャリソース
		&srvDescModel2, // SRVの設定
		textureSrvHandleCPUModel2); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model3)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel3 = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 6);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel3 = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 6);
	// SRVを作成する(Model3)
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResourceModel3.Get(), // テクスチャリソース
		&srvDescModel3, // SRVの設定
		textureSrvHandleCPUModel3); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model4)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel4 = GetCPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 7);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel4 = GetGPUDescriptorHandle(dxCommon.GetSrvHeap(), descriptorSizeSRV, 7);
	// SRVを作成する(Model4)
	dxCommon.GetDevice()->CreateShaderResourceView(
		textureResourceModel4.Get(), // テクスチャリソース
		&srvDescModel4, // SRVの設定
		textureSrvHandleCPUModel4); // SRVのディスクリプタハンドル

	// 音声読み込み
	SoundData soundData1 = SoundLoadWave("resources/fanfare.wav");
	// 音声再生
	SoundPlayWave(xAudio2.Get(), soundData1);

	// DebugCameraのインスタンス化
	DebugCamera debugCamera;
	debugCamera.Initialize();

	bool isRotate = true; // 回転するかどうかのフラグ
	bool useDebugCamera = false;

	bool wasDebugCameraLastFrame = useDebugCamera; // 毎フレームの最後に更新

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
		// フレームが始まる
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("infomation");
		if (ImGui::BeginTabBar("infomation")) {
			if (ImGui::BeginTabItem("Camera & DirectionalLight")) {
				ImGui::Text("Camera");
				ImGui::DragFloat3("scale(Camera)", &cameraTransform.scale.x, 0.01f); // カメラの拡縮を変更するUI
				ImGui::DragFloat3("Rotate(Camera)", &cameraTransform.rotate.x, 0.01f); // カメラの回転を変更するUI
				ImGui::DragFloat3("Translate(Camera)", &cameraTransform.translate.x, 0.01f); // カメラの位置を変更するUI
				ImGui::Separator();
				ImGui::Text("DirectionalLight");
				ImGui::ColorEdit4("LightColor", &directionalLightData->color.x, true); // 色の値を変更するUI
				ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.01f); // 角度を変更するUI
				ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.01f); // 光度を変更するUI
				ImGui::Separator();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Triangle")) {
				ImGui::Checkbox("Draw(Triangle)", &drawTriangle);
				if (drawTriangle) {
					if (ImGui::CollapsingHeader("SRT")) {
						ImGui::DragFloat3("scale", &triangle->GetTransform().scale.x, 0.01f); // Triangleの拡縮を変更するUI
						ImGui::DragFloat3("rotate", &triangle->GetTransform().rotate.x, 0.01f); // Triangleの回転を変更するUI
						ImGui::DragFloat3("Translate", &triangle->GetTransform().translate.x, 0.01f); // Triangleの位置を変更するUI
						ImGui::Checkbox("isRotate", &isRotate); // 回転するかどうかのUI
					}
					if (ImGui::CollapsingHeader("color")) {
						ImGui::ColorEdit4("Color", &triangle->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting")) {
						ImGui::RadioButton("None", &triangle->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert", &triangle->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert", &triangle->GetMaterialData()->enableLighting, 2);
					}
				}
				ImGui::Separator();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sphere")) {
				ImGui::Checkbox("Draw(Sphere)", &drawSphere);
				if (drawSphere) {
					if (ImGui::CollapsingHeader("SRT")) {
						ImGui::DragFloat3("Scale", &sphere->GetTransform().scale.x, 0.01f); // 球の拡縮を変更するUI
						ImGui::DragFloat3("Rotate", &sphere->GetTransform().rotate.x, 0.01f); // 球の回転を変更するUI
						ImGui::DragFloat3("Translate", &sphere->GetTransform().translate.x, 0.01f); // 球の位置を変更するUI
					}
					if (ImGui::CollapsingHeader("color")) {
						ImGui::ColorEdit4("Color", &sphere->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting")) {
						ImGui::RadioButton("None", &sphere->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert", &sphere->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert", &sphere->GetMaterialData()->enableLighting, 2);
					}
				}
				ImGui::Separator();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sprite")) {
				ImGui::Checkbox("Draw(Sprite)", &drawSprite);
				if (drawSprite) {
					if (ImGui::CollapsingHeader("SRT")) {
						ImGui::DragFloat3("Scale", &sprite->GetTransform().scale.x, 0.01f); // 球の拡縮を変更するUI
						ImGui::DragFloat3("Rotate", &sprite->GetTransform().rotate.x, 0.01f); // 球の回転を変更するUI
						ImGui::DragFloat3("Translate", &sprite->GetTransform().translate.x, 1.0f); // 球の位置を変更するUI
					}
					if (ImGui::CollapsingHeader("color")) {
						ImGui::ColorEdit4("Color", &sprite->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting")) {
						ImGui::RadioButton("None", &sprite->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert", &sprite->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert", &sprite->GetMaterialData()->enableLighting, 2);
					}
					ImGui::Separator();
					ImGui::Text("uvTransform(sprite)");
					if (ImGui::CollapsingHeader("SRT(uv)")) {
						ImGui::DragFloat2("uvScale", &sprite->GetUVTransform().scale.x, 0.01f); // uv球の拡縮を変更するUI
						ImGui::DragFloat("uvRotate", &sprite->GetUVTransform().rotate.z, 0.01f); // uv球の回転を変更するUI
						ImGui::DragFloat2("uvTranslate", &sprite->GetUVTransform().translate.x, 0.01f); // uv球の位置を変更するUI
					}
				}
				ImGui::Separator();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Model")) {
				ImGui::Checkbox("Draw(teapot)", &drawModel);
				if (drawModel) {
					if (ImGui::CollapsingHeader("SRT(1)")) {
						ImGui::DragFloat3("Scale(1)", &teapot->GetTransform().scale.x, 0.01f); // 球の拡縮を変更するUI
						ImGui::DragFloat3("Rotate(1)", &teapot->GetTransform().rotate.x, 0.01f); // 球の回転を変更するUI
						ImGui::DragFloat3("Translate(1)", &teapot->GetTransform().translate.x, 0.01f); // 球の位置を変更するUI
					}
					if (ImGui::CollapsingHeader("color(1)")) {
						ImGui::ColorEdit4("Color(1)", &teapot->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting(1)")) {
						ImGui::RadioButton("None(1)", &teapot->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert(1)", &teapot->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert(1)", &teapot->GetMaterialData()->enableLighting, 2);
					}
				}
				ImGui::Separator();
				ImGui::Checkbox("Draw(multiMaterial)", &drawModel2);
				if (drawModel2) {
					if (ImGui::CollapsingHeader("SRT(2)")) {
						ImGui::DragFloat3("Scale(2)", &multiMaterial->GetTransform().scale.x, 0.01f); // 球の拡縮を変更するUI
						ImGui::DragFloat3("Rotate(2)", &multiMaterial->GetTransform().rotate.x, 0.01f); // 球の回転を変更するUI
						ImGui::DragFloat3("Translate(2)", &multiMaterial->GetTransform().translate.x, 0.01f); // 球の位置を変更するUI
					}
					if (ImGui::CollapsingHeader("color(2)")) {
						ImGui::ColorEdit4("Color(2)", &multiMaterial->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting(2)")) {
						ImGui::RadioButton("None(2)", &multiMaterial->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert(2)", &multiMaterial->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert(2)", &multiMaterial->GetMaterialData()->enableLighting, 2);
					}
				}
				ImGui::Separator();
				ImGui::Checkbox("Draw(suzanne)", &drawModel3);
				if (drawModel3) {
					if (ImGui::CollapsingHeader("SRT(3)")) {
						ImGui::DragFloat3("Scale(3)", &suzanne->GetTransform().scale.x, 0.01f); // 球の拡縮を変更するUI
						ImGui::DragFloat3("Rotate(3)", &suzanne->GetTransform().rotate.x, 0.01f); // 球の回転を変更するUI
						ImGui::DragFloat3("Translate(3)", &suzanne->GetTransform().translate.x, 0.01f); // 球の位置を変更するUI
					}
					if (ImGui::CollapsingHeader("color(3)")) {
						ImGui::ColorEdit4("Color(3)", &suzanne->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting(3)")) {
						ImGui::RadioButton("None(3)", &suzanne->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert(3)", &suzanne->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert(3)", &suzanne->GetMaterialData()->enableLighting, 2);
					}
				}
				ImGui::Separator();
				ImGui::Checkbox("Draw(bunny)", &drawModel4);
				if (drawModel4) {
					if (ImGui::CollapsingHeader("SRT(4)")) {
						ImGui::DragFloat3("Scale(4)", &bunny->GetTransform().scale.x, 0.01f); // 球の拡縮を変更するUI
						ImGui::DragFloat3("Rotate(4)", &bunny->GetTransform().rotate.x, 0.01f); // 球の回転を変更するUI
						ImGui::DragFloat3("Translate(4)", &bunny->GetTransform().translate.x, 0.01f); // 球の位置を変更するUI
					}
					if (ImGui::CollapsingHeader("color(4)")) {
						ImGui::ColorEdit4("Color(4)", &bunny->GetMaterialData()->color.x, true); // 色の値を変更するUI
					}
					if (ImGui::CollapsingHeader("lighting(4)")) {
						ImGui::RadioButton("None(4)", &bunny->GetMaterialData()->enableLighting, 0);
						ImGui::RadioButton("Lambert(4)", &bunny->GetMaterialData()->enableLighting, 1);
						ImGui::RadioButton("HalfLambert(4)", &bunny->GetMaterialData()->enableLighting, 2);
					}
				}
				ImGui::Separator();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();

		ImGui::Begin("DebugCamera");
		if (useDebugCamera) {
			ImGui::Text("Debug Camera Running");
			ImGui::Text("Press [Tab] to exit the debug camera");
			ImGui::Text("Press [R] to reset the debug camera position");
			ImGui::Text("Press [W][A][S][D] to move");
			ImGui::Text("[Right-click and move] to move the viewpoint");
		} else {
			ImGui::Text("Debug Camera Disabled");
			ImGui::Text("Press [Tab] to launch the debug camera");
		}
		ImGui::End();

		ImGui::Begin("FPS");
		ImGui::Text("FPS: %.1f", 1.0f / dxCommon.GetDeltaTime());
		ImGui::End();
		// ImGuiのデモ用のUIを表示している
		// 開発用のUIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
		//ImGui::ShowDemoWindow();

		// ImGuiの内部コマンドを生成する
		ImGui::Render();

		// Inputの更新処理
		input->Update();

		//directionalLightの正規化
		directionalLightData->direction = Math::Normalize(directionalLightData->direction);

		if (input->Trigger(DIK_TAB)) {
			if (useDebugCamera) {
				useDebugCamera = false;
			} else {
				useDebugCamera = true;
			}
		}

		// 三角形の回転処理
		if (isRotate) {
			triangle->GetTransform().rotate.y += 0.03f;
		} else {
			triangle->GetTransform().rotate.y = 0.0f;
		}

		// 各種行列の処理
		// カメラ
		Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		Matrix4x4 viewMatrix2D = Math::MakeIdentity();
		Matrix4x4 projectionMatrix2D = Math::MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(WindowApp::kClientWidth), static_cast<float>(WindowApp::kClientHeight), 0.0f, 100.0f);
		Matrix4x4 viewMatrix3D;
		Matrix4x4 projectionMatrix3D;
		if (useDebugCamera) {
			if (!wasDebugCameraLastFrame) {
				debugCamera.skipNextMouseUpdate_ = true; // 初回だけフラグON
			}
			debugCamera.HideCursor();
			debugCamera.Update(input.get());
			viewMatrix3D = debugCamera.GetViewMatrix();
			projectionMatrix3D = debugCamera.GetProjectionMatrix();
		} else {
			debugCamera.ShowCursorBack();
			viewMatrix3D = Math::Inverse(cameraMatrix);
			projectionMatrix3D = Math::MakePerspectiveFovMatrix(0.45f, static_cast<float>(WindowApp::kClientWidth) / static_cast<float>(WindowApp::kClientHeight), 0.1f, 100.0f);
		}
		// フレーム最後に更新して次フレームに備える
		wasDebugCameraLastFrame = useDebugCamera;

		// 三角形
		triangle->Update(viewMatrix3D, projectionMatrix3D);

		// スプライト
		sprite->Update(viewMatrix2D, projectionMatrix2D);

		// 球
		sphere->Update(viewMatrix3D, projectionMatrix3D);

		// モデル
		teapot->Update(viewMatrix3D, projectionMatrix3D);

		// モデル2
		multiMaterial->Update(viewMatrix3D, projectionMatrix3D);

		// モデル3
		suzanne->Update(viewMatrix3D, projectionMatrix3D);

		// モデル4
		bunny->Update(viewMatrix3D, projectionMatrix3D);

		// 描画前処理
		dxCommon.BeginFrame();
		dxCommon.PreDraw(
			pso->GetPipelineState(),
			pso->GetRootSignature(),
			viewport,
			scissorRect
		);

		// 三角形の描画
		triangle->Draw(dxCommon.GetCommandList(), textureSrvHandleGPU3, directionalLightResource.Get(), drawTriangle);

		// Spriteの描画
		sprite->Draw(dxCommon.GetCommandList(), textureSrvHandleGPU, directionalLightResource.Get(), drawSprite);

		// Sphereの描画
		sphere->Draw(dxCommon.GetCommandList(), textureSrvHandleGPU2, directionalLightResource.Get(), drawSphere);

		// Modelの描画
		teapot->Draw(dxCommon.GetCommandList(),
			teapot->GetMaterialResource(),
			teapot->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel,
			drawModel);

		// Model2の描画
		multiMaterial->Draw(dxCommon.GetCommandList(),
			multiMaterial->GetMaterialResource(),
			multiMaterial->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel2,
			drawModel2);

		// Model3の描画
		suzanne->Draw(dxCommon.GetCommandList(),
			suzanne->GetMaterialResource(),
			suzanne->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel3,
			drawModel3);

		// Model4の描画
		bunny->Draw(dxCommon.GetCommandList(),
			bunny->GetMaterialResource(),
			bunny->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel4,
			drawModel4);

		// ImGui表示
		dxCommon.DrawImGui();

		// 描画後処理
		dxCommon.EndFrame();

		// 待機処理を行い、FPS固定
		dxCommon.FrameEnd(60);
	}

	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// XAudio2解放
	xAudio2.Reset();
	// 音声データ解放
	SoundUnload(&soundData1);

	// PSO の解放
	if (pso) {
		delete pso;
	}

	// 警告時に止まる
	//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true); // infoQueueがスコープ外でアクセスされるためコメントアウト

	// COMの終了処理
	CoUninitialize();
	return 0;
}