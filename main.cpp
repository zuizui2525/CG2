#include "Function.h"
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
	if (!window.Initialize(L"CG2")) return -1;
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

	// DXGIファクトリーの生成
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory.GetAddressOf()));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	// どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter;
	// 良い順にアダプタを読む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(useAdapter.GetAddressOf())) != DXGI_ERROR_NOT_FOUND; ++i) {
		// アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));// 取得できないのは一大事
		// ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力。wstringの方なので注意
			Log(logStream, ConvertString(std::format(L"Use Adapater:{}\n", adapterDesc.Description)));
			break;
		}
		// ソフトウェアアダプタの場合は見なかったことにする
	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	Microsoft::WRL::ComPtr<ID3D12Device> device;
	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = {
		"12.2",
		"12.1",
		"12.0"
	};
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(
			useAdapter.Get(), // アダプタ
			featureLevels[i], // 機能レベル
			IID_PPV_ARGS(device.GetAddressOf())); // デバイスのポインタ
		// 指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr)) {
			// 生成できたのでログ出力を行ってループを抜ける
			Log(logStream, std::format("FeatureLevel　:　{}\n", featureLevelStrings[i]));
			break;
		}
	}
	// デバイスが生成できなかったので起動できない
	assert(device != nullptr);
	Log(logStream, "Complete create D3D12Device!!!\n"); // 初期化完了のログを出す

#ifdef _DEBUG
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue; // デバッグ用の情報キュー
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(infoQueue.GetAddressOf())))) {
		// ヤバいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// エラーと警告の抑制
		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバックレイヤーとDX12デバックレイヤーの相互作用バグによるエラーメッセージ
			// http://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};
		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds); // 抑制するメッセージの数
		filter.DenyList.pIDList = denyIds; // 抑制するメッセージのID
		filter.DenyList.NumSeverities = _countof(severities); // 抑制するレベルの数
		filter.DenyList.pSeverityList = severities; // 抑制するレベル
		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter); // フィルタを適用する

		// 解放

	}
#endif

	//uint32_t* //*p = 100;

	// コマンドキューを生成する
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(
		&commandQueueDesc, // コマンドキューの設定
		IID_PPV_ARGS(commandQueue.GetAddressOf())); // コマンドキューのポインタ
	// コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドアロケーターを生成する
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, // コマンドリストの種類
		IID_PPV_ARGS(commandAllocator.GetAddressOf())); // コマンドアロケータのポインタ
	// コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	hr = device->CreateCommandList(
		0, // コマンドリストのID
		D3D12_COMMAND_LIST_TYPE_DIRECT, // コマンドリストの種類
		commandAllocator.Get(), // コマンドアロケータ
		nullptr, // パイプラインステートオブジェクト
		IID_PPV_ARGS(commandList.GetAddressOf())); // コマンドリストのポインタ
	// コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// スワップチェインを生成する
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = WindowApp::kClientWidth; // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = WindowApp::kClientHeight; // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプリングの数。1はマルチサンプリングなし
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // バッファの使用方法。描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; // バッファの数。ダブルバッファリング
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // スワップチェインの効果。フリップディスカードは、古いバッファを捨てる。モニタにうつしたら、中身を廃棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(), // コマンドキュー
		window.GetHWND(), // ウィンドウハンドル
		&swapChainDesc, // スワップチェインの設定
		nullptr, // モニタのハンドル。nullptrはモニタを指定しない
		nullptr, // スワップチェインのオプション。nullptrはオプションなし
		reinterpret_cast<IDXGISwapChain1**>(swapChain.GetAddressOf())); // スワップチェインのポインタ
	// スワップチェインの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// RTV用のヒープでディスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	// SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// SwapChainからResourceを引っ張ってくる
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0])); // 0番目のバッファを取得
	// うまく取得出来なければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1])); // 1番目のバッファを取得
	// うまく取得出来なければ起動できない
	assert(SUCCEEDED(hr));

	// RTVの設定
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして書き込む
	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//RTVを２つ作るのでディスクリプタを２つ用意
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
	// まず１つ目を作る。１つ目は最初のところに作る。作る場所をこちらで指定してあげる必要がある
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(
		swapChainResources[0].Get(), // リソース
		&rtvDesc, // 設定
		rtvHandles[0]); // ディスクリプタのハンドル
	// ２つ目のディスクリプタハンドルを得る(自力で)
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 2つ目のRTVを作る
	device->CreateRenderTargetView(
		swapChainResources[1].Get(), // リソース
		&rtvDesc, // 設定
		rtvHandles[1]); // ディスクリプタのハンドル

	// 初期化0でFenceを作成する
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	uint64_t fenceValue = 0; // フェンスの値
	hr = device->CreateFence(
		fenceValue, // フェンスの値
		D3D12_FENCE_FLAG_NONE, // フェンスのフラグ。特に指定しない
		IID_PPV_ARGS(fence.GetAddressOf())); // フェンスのポインタ
	// フェンスの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	// イベントの作成がうまくいかなかったので起動できない
	assert(fenceEvent != nullptr);

	// dxcCompilerを初期化
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils.GetAddressOf()));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler.GetAddressOf()));
	assert(SUCCEEDED(hr));

	// 現時点ではincludeはしないが、includeに対応する為に設定をしておく
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	//PSOを作成する
	PSO* pso = new PSO(device.Get(), dxcUtils.Get(), dxcCompiler.Get(), includeHandler.Get(), logStream);
	// assert(SUCCEEDED(hr)); // PSOのコンストラクタはHRESULTを返さないので削除

	// XAudioエンジンのインスタンスを生成
	hr = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
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
	std::unique_ptr<TriangleObject> triangle = std::make_unique<TriangleObject>(device.Get());

	// スプライトの初期化
	std::unique_ptr<SpriteObject> sprite = std::make_unique<SpriteObject>(device.Get(), 640, 360);

	// 球の初期化
	std::unique_ptr<SphereObject> sphere = std::make_unique<SphereObject>(device.Get(), 16, 1.0f);

	// モデル生成（例: teapot.obj を読み込む）
	std::unique_ptr<ModelObject> teapot = std::make_unique<ModelObject>(device.Get(), "resources", "teapot.obj", Vector3{ 1.0f, 0.0f, 0.0f });

	// モデル生成（例: multiMaterial.obj を読み込む）
	std::unique_ptr<ModelObject> multiMaterial = std::make_unique<ModelObject>(device.Get(), "resources", "multiMaterial.obj", Vector3{ 0.0f, 0.0f, 0.0f });

	// モデル生成（例: suzanne.obj を読み込む）
	std::unique_ptr<ModelObject> suzanne = std::make_unique<ModelObject>(device.Get(), "resources", "suzanne.obj", Vector3{ 0.0f, 0.0f, 0.0f });

	// モデル生成（例: bunny.obj を読み込む）
	std::unique_ptr<ModelObject> bunny = std::make_unique<ModelObject>(device.Get(), "resources", "bunny.obj", Vector3{ 0.0f, 0.0f, 0.0f });

	// DirectionalLight
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource = CreateBufferResource(device.Get(), sizeof(DirectionalLight));
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
	ImGui_ImplDX12_Init(device.Get(),
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap.Get(),
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeRTV, 0);

	// 1枚目のTextureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(device.Get(), metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource.Get(), mipImages, device.Get(), commandList.Get());
	// 2枚目のTextureを読んで転送する(2)
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterball.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device.Get(), metadata2);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2.Get(), mipImages2, device.Get(), commandList.Get());
	// 3枚目のTextureを読んで転送する(3)
	DirectX::ScratchImage mipImages3 = LoadTexture("resources/white.png");
	const DirectX::TexMetadata& metadata3 = mipImages3.GetMetadata(); // mipImagesからmipImages3に変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource3 = CreateTextureResource(device.Get(), metadata3);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource3 = UploadTextureData(textureResource3.Get(), mipImages3, device.Get(), commandList.Get());
	// ModelのTextureを読んで転送する(Model)
	DirectX::ScratchImage mipImagesModel = LoadTexture(teapot->GetModelData().material.textureFilePath);
	const DirectX::TexMetadata& metadataModel = mipImagesModel.GetMetadata(); // mipImagesからmipImagesModelに変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel = CreateTextureResource(device.Get(), metadataModel);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel = UploadTextureData(textureResourceModel.Get(), mipImagesModel, device.Get(), commandList.Get());
	// ModelのTextureを読んで転送する(Model2)
	DirectX::ScratchImage mipImagesModel2 = LoadTexture(multiMaterial->GetModelData().material.textureFilePath);
	const DirectX::TexMetadata& metadataModel2 = mipImagesModel2.GetMetadata(); // mipImagesからmipImagesModelに変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel2 = CreateTextureResource(device.Get(), metadataModel2);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel2 = UploadTextureData(textureResourceModel2.Get(), mipImagesModel2, device.Get(), commandList.Get());
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
	textureResourceModel3 = CreateTextureResource(device.Get(), metadataModel3);
	intermediateResourceModel3 = UploadTextureData(
		textureResourceModel3.Get(), mipImagesModel3, device.Get(), commandList.Get()
	);

	// ModelのTextureを読んで転送する(Model4)
	DirectX::ScratchImage mipImagesModel4 = LoadTexture(bunny->GetModelData().material.textureFilePath);
	const DirectX::TexMetadata& metadataModel4 = mipImagesModel4.GetMetadata(); // mipImagesからmipImagesModelに変更
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResourceModel4 = CreateTextureResource(device.Get(), metadataModel4);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResourceModel4 = UploadTextureData(textureResourceModel4.Get(), mipImagesModel4, device.Get(), commandList.Get());

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
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 1);
	// SRVを作成する
	device->CreateShaderResourceView(
		textureResource.Get(), // テクスチャリソース
		&srvDesc, // SRVの設定
		textureSrvHandleCPU); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(2)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 2);
	// SRVを作成する(2)
	device->CreateShaderResourceView(
		textureResource2.Get(), // テクスチャリソース
		&srvDesc2, // SRVの設定
		textureSrvHandleCPU2); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(3)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU3 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 3);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU3 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 3);
	// SRVを作成する(3)
	device->CreateShaderResourceView(
		textureResource3.Get(), // テクスチャリソース
		&srvDesc3, // SRVの設定
		textureSrvHandleCPU3); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 4);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 4);
	// SRVを作成する(Model)
	device->CreateShaderResourceView(
		textureResourceModel.Get(), // テクスチャリソース
		&srvDescModel, // SRVの設定
		textureSrvHandleCPUModel); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model2)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel2 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 5);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel2 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 5);
	// SRVを作成する(Model2)
	device->CreateShaderResourceView(
		textureResourceModel2.Get(), // テクスチャリソース
		&srvDescModel2, // SRVの設定
		textureSrvHandleCPUModel2); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model3)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel3 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 6);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel3 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 6);
	// SRVを作成する(Model3)
	device->CreateShaderResourceView(
		textureResourceModel3.Get(), // テクスチャリソース
		&srvDescModel3, // SRVの設定
		textureSrvHandleCPUModel3); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model4)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel4 = GetCPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 7);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel4 = GetGPUDescriptorHandle(srvDescriptorHeap.Get(), descriptorSizeSRV, 7);
	// SRVを作成する(Model4)
	device->CreateShaderResourceView(
		textureResourceModel4.Get(), // テクスチャリソース
		&srvDescModel4, // SRVの設定
		textureSrvHandleCPUModel4); // SRVのディスクリプタハンドル

	// DepthStencil用のリソースを作成する
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource = CreateDepthStencilTextureResource(device.Get(), WindowApp::kClientWidth, WindowApp::kClientHeight);
	// DepthStencil用のDSVを作成する
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	// DSVHeapの先頭にDSVを作成する
	device->CreateDepthStencilView(
		depthStencilResource.Get(), // テクスチャリソース
		&dsvDesc, // DSVの設定
		dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); // DSVのディスクリプタハンドル

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

		// これから書き込むバックバッファのインデックスを取得
		UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
		// TransitionBarrierを設定する
		D3D12_RESOURCE_BARRIER barrier{};
		// 今回のバリアはTransition
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		// Noneにしておく
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		// バリアを張る対象のリソース。現在のバックバッファに対して行う
		barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
		// 遷移前(現在)のResourceState。
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		// 遷移後のResourceState。描画を行うためにレンダーターゲットとして使用する。
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		// TransitionBarrierを張る
		commandList->ResourceBarrier(1, &barrier);
		// 描画先のRTVを設定する
		// 描画先のRTVとDSVを設定する
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		commandList->OMSetRenderTargets(1, &rtvHandles[backBufferIndex], false, &dsvHandle);
		// 指定した深度で画面全体をクリアする
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		// 指定した色で画面全体をクリアにする
		float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f }; // 青っぽい色。RGBAの順
		commandList->ClearRenderTargetView(rtvHandles[backBufferIndex], clearColor, 0, nullptr);
		//描画用のDescriptorHeapの設定
		ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
		commandList->SetDescriptorHeaps(1, descriptorHeaps);
		commandList->RSSetViewports(1, &viewport); // Viewportを設定
		commandList->RSSetScissorRects(1, &scissorRect); // Scissorを設定
		// RootSignatureを設定。PSOに設定しているけど別途設定が必要
		commandList->SetGraphicsRootSignature(pso->GetRootSignature());
		commandList->SetPipelineState(pso->GetPipelineState()); // PSOを設定
		//形状を設定。PSOに設定しているものとはまた別。同じものを設定するトポロジ考えておけばいい。
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// 三角形の描画
		triangle->Draw(commandList.Get(), textureSrvHandleGPU3, directionalLightResource.Get(), drawTriangle);

		// Spriteの描画
		sprite->Draw(commandList.Get(), textureSrvHandleGPU, directionalLightResource.Get(), drawSprite);

		// Sphereの描画
		sphere->Draw(commandList.Get(), textureSrvHandleGPU2, directionalLightResource.Get(), drawSphere);

		// Modelの描画
		teapot->Draw(commandList.Get(),
			teapot->GetMaterialResource(),
			teapot->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel,
			drawModel);

		// Model2の描画
		multiMaterial->Draw(commandList.Get(),
			multiMaterial->GetMaterialResource(),
			multiMaterial->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel2,
			drawModel2);

		// Model3の描画
		suzanne->Draw(commandList.Get(),
			suzanne->GetMaterialResource(),
			suzanne->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel3,
			drawModel3);

		// Model4の描画
		bunny->Draw(commandList.Get(),
			bunny->GetMaterialResource(),
			bunny->GetWVPResource(),
			directionalLightResource.Get(),
			textureSrvHandleGPUModel4,
			drawModel4);


		// 実際のcommandListのImGuiの描画コマンドを積む
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
		// 画面に描く処理は全て終わり、画面に移すので、状態を遷移
		// 今回はRenderTargetからPresentにする
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		// TransitionBarrierを張る
		commandList->ResourceBarrier(1, &barrier);

		// コマンドリストの内容を確定させる。全てのコマンドを積んでからCloseすること
		hr = commandList->Close();
		// コマンドリストの確定がうまくいかなかったので起動できない
		assert(SUCCEEDED(hr));

		// GPUにコマンドリストの実行を行わせる
		ID3D12CommandList* commandLists[] = { commandList.Get() }; // コマンドリストの配列
		commandQueue->ExecuteCommandLists(1, commandLists); // コマンドリストを実行する
		// GPUとOSに画面の交換を行うよう通知する
		swapChain->Present(1, 0); // 1はV-Syncのための待機。0は何もしない
		// Fenceの値を更新
		fenceValue++;
		// GPUがここまでたどり着いた時に、Fenceの値を指定した値に代入するようにSignalを送る
		commandQueue->Signal(fence.Get(), fenceValue);
		// Fenceの値が指定した値にたどり着いているか確認する
		// GetCompletedValueの初期値はFence作成時に渡した初期値
		if (fence->GetCompletedValue() < fenceValue) {
			// 指定した値にたどり着いていないので、イベントが発生するまで待機する
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			// イベントが発生するまで待機する
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		// 次のフレーム用のコマンドリストを準備
		hr = commandList->Reset(commandAllocator.Get(), nullptr); // コマンドリストをリセットする
		// コマンドリストのリセットがうまくいかなかったので起動できない
		assert(SUCCEEDED(hr));
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