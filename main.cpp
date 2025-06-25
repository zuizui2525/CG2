#include "Function.h"

// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;
// ウィンドウサイズを表す構造体にクライアント領域を入れる
RECT wrc = { 0, 0, kClientWidth, kClientHeight };

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	// 誰も補足しなかった場合に(Unhandled)、補足する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

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

	WNDCLASS wc = {};
	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名(何でも良い)
	wc.lpszClassName = L"CG2WindowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	// ウィンドウクラスを登録する
	RegisterClass(&wc);

	// クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName, // ウィンドウクラス名
		L"CG2", // ウィンドウ名
		WS_OVERLAPPEDWINDOW, // ウィンドウスタイル
		CW_USEDEFAULT, // 表示X座標(Windowsに任せる)
		CW_USEDEFAULT, // 表示Y座標(WindowsOSに任せる)
		wrc.right - wrc.left, // ウィンドウ横幅
		wrc.bottom - wrc.top, // ウィンドウ縦横
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		wc.hInstance, // インスタンスハンドル
		nullptr); // オプション

	// エラー放置ダメ絶対
#ifdef _DEBUG
	ID3D12Debug1* debugController = nullptr; // デバッグ用のコントローラ
	// デバッグレイヤーのインターフェースを取得する
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効にする
		debugController->EnableDebugLayer();
		// コンプライアンスのチェックを行う
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	// ウィンドウを表示する
	ShowWindow(hwnd, SW_SHOW);

	// DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;
	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、
	// どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));

	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	IDXGIAdapter4* useAdapter = nullptr;
	// 良い順にアダプタを読む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
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
		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;
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
			useAdapter, // アダプタ
			featureLevels[i], // 機能レベル
			IID_PPV_ARGS(&device)); // デバイスのポインタ
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
	ID3D12InfoQueue* infoQueue = nullptr; // デバッグ用の情報キュー
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
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
		infoQueue->Release();
	}
#endif

	//uint32_t* p = nullptr;
	//*p = 100;

	// コマンドキューを生成する
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	hr = device->CreateCommandQueue(
		&commandQueueDesc, // コマンドキューの設定
		IID_PPV_ARGS(&commandQueue)); // コマンドキューのポインタ
	// コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドアロケーターを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	hr = device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT, // コマンドリストの種類
		IID_PPV_ARGS(&commandAllocator)); // コマンドアロケータのポインタ
	// コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// コマンドリストを生成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	hr = device->CreateCommandList(
		0, // コマンドリストのID
		D3D12_COMMAND_LIST_TYPE_DIRECT, // コマンドリストの種類
		commandAllocator, // コマンドアロケータ
		nullptr, // パイプラインステートオブジェクト
		IID_PPV_ARGS(&commandList)); // コマンドリストのポインタ
	// コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// スワップチェインを生成する
	IDXGISwapChain4* swapChain = nullptr;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = kClientWidth; // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight; // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプリングの数。1はマルチサンプリングなし
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // バッファの使用方法。描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2; // バッファの数。ダブルバッファリング
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // スワップチェインの効果。フリップディスカードは、古いバッファを捨てる。モニタにうつしたら、中身を廃棄
	// コマンドキュー、ウィンドウハンドル、設定を渡して生成する
	hr = dxgiFactory->CreateSwapChainForHwnd(
		commandQueue, // コマンドキュー
		hwnd, // ウィンドウハンドル
		&swapChainDesc, // スワップチェインの設定
		nullptr, // モニタのハンドル。nullptrはモニタを指定しない
		nullptr, // スワップチェインのオプション。nullptrはオプションなし
		reinterpret_cast<IDXGISwapChain1**>(&swapChain)); // スワップチェインのポインタ
	// スワップチェインの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// RTV用のヒープでディスクリプタの数は２。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);

	// SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	// SwapChainからResourceを引っ張ってくる
	ID3D12Resource* swapChainResources[2] = { nullptr };
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
		swapChainResources[0], // リソース
		&rtvDesc, // 設定
		rtvHandles[0]); // ディスクリプタのハンドル
	// ２つ目のディスクリプタハンドルを得る(自力で)
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 2つ目のRTVを作る
	device->CreateRenderTargetView(
		swapChainResources[1], // リソース
		&rtvDesc, // 設定
		rtvHandles[1]); // ディスクリプタのハンドル

	// 初期化0でFenceを作成する
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0; // フェンスの値
	hr = device->CreateFence(
		fenceValue, // フェンスの値
		D3D12_FENCE_FLAG_NONE, // フェンスのフラグ。特に指定しない
		IID_PPV_ARGS(&fence)); // フェンスのポインタ
	// フェンスの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	// FenceのSignalを待つためのイベントを作成する
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	// イベントの作成がうまくいかなかったので起動できない
	assert(fenceEvent != nullptr);

	// dxcCompilerを初期化
	IDxcUtils* dxcUtils = nullptr;
	IDxcCompiler3* dxcCompiler = nullptr;
	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
	assert(SUCCEEDED(hr));

	// 現時点ではincludeはしないが、includeに対応する為に設定をしておく
	IDxcIncludeHandler* includeHandler = nullptr;
	hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
	assert(SUCCEEDED(hr));

	//PSOを作成する
	PSO* pso = new PSO(device, dxcUtils, dxcCompiler, includeHandler, logStream);
	assert(SUCCEEDED(hr));



	// マテリアル用のリソースを作る。今回はcolor１つ分のサイズを用意する
	ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Material));
	ID3D12Resource* materialResource2 = CreateBufferResource(device, sizeof(Material));
	// マテリアルにデータを書き込む
	Material* materialData = nullptr;
	Material* materialData2 = nullptr;
	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialResource2->Map(0, nullptr, reinterpret_cast<void**>(&materialData2));
	// 今回は赤を書き込んでみる
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvtransform = Math::MakeIdentity();
	materialData2->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData2->enableLighting = false;
	materialData2->uvtransform = Math::MakeIdentity();
	// WVP用のリソースを作る。１つ分のサイズを用意する。
	ID3D12Resource* wvpResource = CreateBufferResource(device, sizeof(TransformationMatrix));
	ID3D12Resource* wvpResource2 = CreateBufferResource(device, sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpData = nullptr;
	TransformationMatrix* wvpData2 = nullptr;
	// 書き込むためのアドレスを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&wvpData));
	wvpResource2->Map(0, nullptr, reinterpret_cast<void**>(&wvpData2));
	// 単位行列を書き込んでおく
	wvpData->WVP = Math::MakeIdentity();
	wvpData->world = Math::MakeIdentity();
	wvpData2->WVP = Math::MakeIdentity();
	wvpData2->world = Math::MakeIdentity();
	// 実際に頂点リソースを作る
	ID3D12Resource* vertexResource = CreateBufferResource(device, sizeof(VertexData) * 3);
	ID3D12Resource* vertexResource2 = CreateBufferResource(device, sizeof(VertexData) * 3);
	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView2{};
	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView2.BufferLocation = vertexResource2->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferView.SizeInBytes = sizeof(VertexData) * 3;
	vertexBufferView2.SizeInBytes = sizeof(VertexData) * 3;
	// 1頂点あたりのサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	vertexBufferView2.StrideInBytes = sizeof(VertexData);
	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	VertexData* vertexData2 = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	vertexResource2->Map(0, nullptr, reinterpret_cast<void**>(&vertexData2));
	// 一つ目の三角形
	vertexData[0].position = { -0.5f,-0.5f,0.0f,1.0f }; // 左下
	vertexData[0].texcoord = { 0.0f,1.0f };
	vertexData[1].position = { 0.0f,0.5f,0.0f,1.0f }; // 上
	vertexData[1].texcoord = { 0.5f,0.0f };
	vertexData[2].position = { 0.5f,-0.5f,0.0f,1.0f }; // 右下
	vertexData[2].texcoord = { 1.0f,1.0f };
	// 二つ目の三角形
	vertexData2[0].position = { -0.5f,-0.5f,0.5f,1.0f }; // 左下
	vertexData2[0].texcoord = { 0.0f,1.0f };
	vertexData2[1].position = { 0.0f,0.0f,0.0f,1.0f }; // 上
	vertexData2[1].texcoord = { 0.5f,0.0f };
	vertexData2[2].position = { 0.5f,-0.5f,-0.5f,1.0f }; // 右下
	vertexData2[2].texcoord = { 1.0f,1.0f };
	// ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = kClientWidth;
	viewport.Height = kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	// シザー矩形
	D3D12_RECT scissorRect{};
	// 基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = kClientHeight;
	// Transform変数を作る。
	Transform transform = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform transform2 = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	Transform cameraTransform = { { 1.0f,1.0f,1.0f }, { 0.0f,0.0f,0.0f }, { 0.0f,0.0f,-7.0f } };
	// uvTransform
	Transform uvTransform{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};
	Transform uvTransform2{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};



	// マテリアル用のリソースを作る。今回はcolor１つ分のサイズを用意する
	ID3D12Resource* materialResourceSprite = CreateBufferResource(device, sizeof(Material));
	// マテリアルにデータを書き込む
	Material* materialDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	// 今回は白を書き込んでみる
	materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialDataSprite->enableLighting = false;
	materialDataSprite->uvtransform = Math::MakeIdentity();
	// WVP用のリソースを作る。 １つ分のサイズを用意する。
	ID3D12Resource* wvpResourceSprite = CreateBufferResource(device, sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	wvpResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSprite));
	// 単位行列を書き込んでおく
	wvpDataSprite->WVP = Math::MakeIdentity();
	wvpDataSprite->world = Math::MakeIdentity();
	// Sprite用の頂点リソースを作る
	ID3D12Resource* vertexResourceSprite = CreateBufferResource(device, sizeof(VertexData) * 4);
	// Sprite用の頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	// リソースの先頭のアドレスから使う
	vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点4つ分のサイズ
	vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 4;
	// 1頂点あたりのサイズ
	vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);
	// Sprite用の頂点リソースにデータを書き込む
	VertexData* vertexDataSprite = nullptr;
	// 書き込むためのアドレスを取得
	vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));
	// 四頂点
	vertexDataSprite[0].position = { 0.0f,360.0f,0.0f,1.0f }; // 左下
	vertexDataSprite[0].texcoord = { 0.0f,1.0f };
	vertexDataSprite[0].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[1].position = { 0.0f,0.0f,0.0f,1.0f }; // 左上
	vertexDataSprite[1].texcoord = { 0.0f,0.0f };
	vertexDataSprite[1].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[2].position = { 640.0f,360.0f,0.0f,1.0f }; // 右下
	vertexDataSprite[2].texcoord = { 1.0f,1.0f };
	vertexDataSprite[2].normal = { 0.0f,0.0f,-1.0f };
	vertexDataSprite[3].position = { 640.0f,0.0f,0.0f,1.0f }; // 右上
	vertexDataSprite[3].texcoord = { 1.0f,0.0f };
	vertexDataSprite[3].normal = { 0.0f,0.0f,-1.0f };
	//インデックス用のGPUリソースを作成
	ID3D12Resource* indexResourceSprite = CreateBufferResource(device, sizeof(uint32_t) * 6);
	// インデックスバッファビューの作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	// リソースの先頭のアドレスから使う
	indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス6つ分のサイズ
	indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuin32_tとする
	indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	// インデックスリソースにデータを書き込む
	uint32_t* indexDataSprite = nullptr;
	indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	indexDataSprite[0] = 0;
	indexDataSprite[1] = 1;
	indexDataSprite[2] = 2;
	indexDataSprite[3] = 1;
	indexDataSprite[4] = 3;
	indexDataSprite[5] = 2;
	// CPUで動かす用のTransformを作る
	Transform transformSprite = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	// uvTransformSprite
	Transform uvTransformSprite{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};



	// 球の初期化
	Sphere sphere = {};
	sphere.center = { 0.0f, 0.0f, 0.0f };
	sphere.radius = 1.0f;
	const uint32_t kSubdivision = 16;
	const uint32_t kVertexCount = (kSubdivision + 1) * (kSubdivision + 1);
	const uint32_t kIndexCount = kSubdivision * kSubdivision * 6;
	const float kLonEvery = static_cast<float>(M_PI * 2.0f / kSubdivision);
	const float kLatEvery = static_cast<float>(M_PI / kSubdivision);
	// マテリアル用のリソースを作る。今回はcolor１つ分のサイズを用意する
	ID3D12Resource* materialResourceSphere = CreateBufferResource(device, sizeof(Material));
	// マテリアルにデータを書き込む
	Material* materialDataSphere = nullptr;
	// 書き込むためのアドレスを取得
	materialResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSphere));
	// 今回は白を書き込んでみる
	materialDataSphere->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialDataSphere->enableLighting = true;
	materialDataSphere->uvtransform = Math::MakeIdentity();
	// WVP用のリソースを作る。１つ分のサイズを用意する。
	ID3D12Resource* wvpResourceSphere = CreateBufferResource(device, sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpDataSphere = nullptr;
	// 書き込むためのアドレスを取得
	wvpResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataSphere));
	// 単位行列を書き込んでおく
	wvpDataSphere->WVP = Math::MakeIdentity();
	wvpDataSphere->world = Math::MakeIdentity();
	// Sphere用の頂点リソースを作る
	ID3D12Resource* vertexResourceSphere = CreateBufferResource(device, sizeof(VertexData) * kVertexCount);
	// Sphere用の頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSphere{};
	// リソースの先頭のアドレスから使う
	vertexBufferViewSphere.BufferLocation = vertexResourceSphere->GetGPUVirtualAddress();
	// 使用するリソースのサイズは頂点6つ分のサイズ
	vertexBufferViewSphere.SizeInBytes = sizeof(VertexData) * kVertexCount;
	// 1頂点あたりのサイズ
	vertexBufferViewSphere.StrideInBytes = sizeof(VertexData);
	// Sphere用の頂点リソースにデータを書き込む
	VertexData* vertexDataSphere = nullptr;
	// 書き込むためのアドレスを取得
	vertexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSphere));
	// CPU用インデックス配列（スタック確保）
	uint32_t indexDataSphere[kIndexCount];
	// 頂点生成
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
		float lat = static_cast<float>(-M_PI / 2.0f + kLatEvery * latIndex);
		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
			float lon = kLonEvery * lonIndex;

			float x = cosf(lat) * cosf(lon) * sphere.radius + sphere.center.x;
			float y = sinf(lat) * sphere.radius + sphere.center.y;
			float z = cosf(lat) * sinf(lon) * sphere.radius + sphere.center.z;

			uint32_t index = latIndex * (kSubdivision + 1) + lonIndex;
			vertexDataSphere[index].position = { x, y, z, 1.0f };
			vertexDataSphere[index].texcoord = { (float)lonIndex / kSubdivision, 1.0f - (float)latIndex / kSubdivision };
			vertexDataSphere[index].normal = { vertexDataSphere[index].position.x, vertexDataSphere[index].position.y, vertexDataSphere[index].position.z};
		}
	}
	// インデックス生成（三角形2つで四角形1枚）
	uint32_t idx = 0;
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t current = latIndex * (kSubdivision + 1) + lonIndex;
			uint32_t next = current + kSubdivision + 1;

			// 三角形1
			indexDataSphere[idx++] = current;
			indexDataSphere[idx++] = next;
			indexDataSphere[idx++] = current + 1;

			// 三角形2
			indexDataSphere[idx++] = current + 1;
			indexDataSphere[idx++] = next;
			indexDataSphere[idx++] = next + 1;
		}
	}
	// インデックス用のGPUリソースを作成（バッファのサイズは uint32_t の個数×サイズ）
	ID3D12Resource* indexResourceSphere = CreateBufferResource(device, sizeof(uint32_t) * kIndexCount);
	// インデックスバッファビューの作成
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSphere{};
	indexBufferViewSphere.BufferLocation = indexResourceSphere->GetGPUVirtualAddress();
	indexBufferViewSphere.SizeInBytes = sizeof(uint32_t) * kIndexCount;
	indexBufferViewSphere.Format = DXGI_FORMAT_R32_UINT; // 32bitインデックスを使う場合
	// 書き込み用のポインタを取得
	uint32_t* indexDataSphereGPU = nullptr;
	indexResourceSphere->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSphereGPU));
	// CPU側で作成したインデックスデータをGPUリソースにコピー
	memcpy(indexDataSphereGPU, indexDataSphere, sizeof(uint32_t) * kIndexCount);
	// CPUで動かす用のTransformを作る
	Transform transformSphere = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	// uvTransformSphere
	Transform uvTransformSphere{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};



	// マテリアル用のリソースを作る。今回はcolor１つ分のサイズを用意する
	ID3D12Resource* materialResourceModel = CreateBufferResource(device, sizeof(Material));
	// マテリアルにデータを書き込む
	Material* materialDataModel = nullptr;
	// 書き込むためのアドレスを取得
	materialResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&materialDataModel));
	// 今回は白を書き込んでみる
	materialDataModel->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialDataModel->enableLighting = true;
	materialDataModel->uvtransform = Math::MakeIdentity();
	// WVP用のリソースを作る。１つ分のサイズを用意する。
	ID3D12Resource* wvpResourceModel = CreateBufferResource(device, sizeof(TransformationMatrix));
	// データを書き込む
	TransformationMatrix* wvpDataModel = nullptr;
	// 書き込むためのアドレスを取得
	wvpResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&wvpDataModel));
	// 単位行列を書き込んでおく
	wvpDataModel->WVP = Math::MakeIdentity();
	wvpDataModel->world = Math::MakeIdentity();
	// モデル読み込み
	ModelData modelData = LoadObjFile("resources", "plane.obj");
	// 頂点リソースを作る
	ID3D12Resource* vertexResourceModel = CreateBufferResource(device, sizeof(VertexData) * modelData.vertices.size());
	// 頂点バッファビューを作成する
	D3D12_VERTEX_BUFFER_VIEW vertexBufferViewModel{};
	vertexBufferViewModel.BufferLocation = vertexResourceModel->GetGPUVirtualAddress(); // リソースの先頭のアドレスから使う
	vertexBufferViewModel.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 使用するリソースのサイズは頂点のサイズ
	vertexBufferViewModel.StrideInBytes = sizeof(VertexData); // 1頂点あたりのサイズ
	// 頂点リソースにデータを書き込む
	VertexData* vertexDataModel = nullptr;
	vertexResourceModel->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel)); // 書き込むためのアドレスを取得
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size()); // 頂点データをリソースにコピー
	// CPUで動かす用のTransformを作る
	Transform transformModel = { {1.0f,1.0f,1.0f},{0.0f,0.0f,0.0f},{0.0f,0.0f,0.0f} };
	// uvTransformSprite
	Transform uvTransformModel{
		{1.0f,1.0f,1.0f},
		{0.0f,0.0f,0.0f},
		{0.0f,0.0f,0.0f},
	};



	// DirectionalLight
	ID3D12Resource* directionalLightResource = CreateBufferResource(device, sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f,1.0f,1.0f,1.0f };
	directionalLightData->direction = { 0.0f,-1.0f,0.0f };
	directionalLightData->intensity = 1.0f;



	// ImGuiの初期化。詳細はさして重要ではないので解説は省略する。
	// こういうもん
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX12_Init(device,
		swapChainDesc.BufferCount,
		rtvDesc.Format,
		srvDescriptorHeap,
		srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// DescriptorSizeを取得しておく
	const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeRTV, 0);

	// 1枚目のTextureを読んで転送する
	DirectX::ScratchImage mipImages = LoadTexture("resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
	ID3D12Resource* intermediateResource = UploadTextureData(textureResource, mipImages, device, commandList);
	// 2枚目のTextureを読んで転送する(2)
	DirectX::ScratchImage mipImages2 = LoadTexture("resources/monsterball.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	ID3D12Resource* textureResource2 = CreateTextureResource(device, metadata2);
	ID3D12Resource* intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);
	// 3枚目のTextureを読んで転送する(3)
	DirectX::ScratchImage mipImages3 = LoadTexture("resources/white.png");
	const DirectX::TexMetadata& metadata3 = mipImages.GetMetadata();
	ID3D12Resource* textureResource3 = CreateTextureResource(device, metadata3);
	ID3D12Resource* intermediateResource3 = UploadTextureData(textureResource3, mipImages3, device, commandList);
	// ModelのTextureを読んで転送する(Model)
	DirectX::ScratchImage mipImagesModel = LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadataModel = mipImages.GetMetadata();
	ID3D12Resource* textureResourceModel = CreateTextureResource(device, metadataModel);
	ID3D12Resource* intermediateResourceModel = UploadTextureData(textureResourceModel, mipImagesModel, device, commandList);


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

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 1);
	// SRVを作成する
	device->CreateShaderResourceView(
		textureResource, // テクスチャリソース
		&srvDesc, // SRVの設定
		textureSrvHandleCPU); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(2)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	// SRVを作成する(2)
	device->CreateShaderResourceView(
		textureResource2, // テクスチャリソース
		&srvDesc2, // SRVの設定
		textureSrvHandleCPU2); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(3)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU3 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 3);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU3 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 3);
	// SRVを作成する(3)
	device->CreateShaderResourceView(
		textureResource3, // テクスチャリソース
		&srvDesc3, // SRVの設定
		textureSrvHandleCPU3); // SRVのディスクリプタハンドル
	// SRVを作成するDescriptorHeapの場所を決める(Model)
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPUModel = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 4);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPUModel = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 4);
	// SRVを作成する(Model)
	device->CreateShaderResourceView(
		textureResourceModel, // テクスチャリソース
		&srvDescModel, // SRVの設定
		textureSrvHandleCPUModel); // SRVのディスクリプタハンドル

	// DepthStencil用のリソースを作成する
	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(device, kClientWidth, kClientHeight);
	// DepthStencil用のDSVを作成する
	ID3D12DescriptorHeap* dsvDescriptorHeap = CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	// DSVHeapの先頭にDSVを作成する
	device->CreateDepthStencilView(
		depthStencilResource, // テクスチャリソース
		&dsvDesc, // DSVの設定
		dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); // DSVのディスクリプタハンドル

	bool useUvChecker = true; // UVCheckerを使うかどうかのフラグ
	bool useMonsterBall = false; // モンスターボールを使うかどうかのフラグ
	bool useModelTexture = false; // Modelのtextureを使うかどうかのフラグ
	bool isRotate = true; // 回転するかどうかのフラグ
	bool isRotate2 = true; // 回転するかどうかのフラグ

	MSG msg{};
	// ウィンドウの×ボタンが押されるまでループ
	while (msg.message != WM_QUIT) {
		// Windowにメッセージが来てたら最優先で処理させる
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			// ゲームの処理
			// フレームが始まる
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::Begin("infomation");
			ImGui::Text("Camera");
			ImGui::DragFloat3("scale(Camera)", &cameraTransform.scale.x, 0.01f); // カメラの拡縮を変更するUI
			ImGui::DragFloat3("Translate(Camera)", &cameraTransform.translate.x, 0.01f); // カメラの位置を変更するUI
			ImGui::DragFloat3("Rotate(Camera)", &cameraTransform.rotate.x, 0.01f); // カメラの回転を変更するUI
			ImGui::Separator();
			ImGui::Text("directionalLight");
			ImGui::ColorEdit4("LightColor", &directionalLightData->color.x, true); // 色の値を変更するUI
			ImGui::DragFloat3("LightDirection", &directionalLightData->direction.x, 0.01f); // 角度を変更するUI
			ImGui::DragFloat("Intensity", &directionalLightData->intensity, 0.01f); // 光度を変更するUI
			ImGui::Separator();
			ImGui::Text("Sphere");
			ImGui::DragFloat3("Scale(sphere)", &transformSphere.scale.x, 0.01f); // 球の拡縮を変更するUI
			ImGui::DragFloat3("Translate(sphere)", &transformSphere.translate.x, 0.01f); // 球の位置を変更するUI
			ImGui::DragFloat3("Rotate(sphere)", &transformSphere.rotate.x, 0.01f); // 球の回転を変更するUI
			ImGui::Separator();
			ImGui::Text("uvTransform(sprite)");
			ImGui::DragFloat2("uvScale(sprite)", &uvTransformSprite.scale.x, 0.01f); // uv球の拡縮を変更するUI
			ImGui::DragFloat2("uvTranslate(sprite)", &uvTransformSprite.translate.x, 0.01f); // uv球の位置を変更するUI
			ImGui::DragFloat("uvRotate(sprite)", &uvTransformSprite.rotate.z, 0.01f); // uv球の回転を変更するUI
			ImGui::Separator();
			ImGui::Text("Model");
			ImGui::DragFloat3("Scale(Model)", &transformModel.scale.x, 0.01f); // 球の拡縮を変更するUI
			ImGui::DragFloat3("Translate(Model)", &transformModel.translate.x, 0.01f); // 球の位置を変更するUI
			ImGui::DragFloat3("Rotate(Model)", &transformModel.rotate.x, 0.01f); // 球の回転を変更するUI
			ImGui::Separator();
			/*ImGui::Text("Triangle"); // Title
			ImGui::DragFloat3("scale(Triangle)", &transform.scale.x, 0.01f); // Triangleの拡縮を変更するUI
			ImGui::DragFloat3("Translate(Triangle)", &transform.translate.x, 0.01f); // Triangleの位置を変更するUI
			ImGui::ColorEdit4("Color(Triangle)", &materialData->x, true); // 色の値を変更するUI
			ImGui::Checkbox("isRotate(Triangle)", &isRotate); // 回転するかどうかのUI
			ImGui::Separator();
			ImGui::Text("Triangle2"); // Title
			ImGui::DragFloat3("scale(Triangle2)", &transform2.scale.x, 0.01f); // Triangle2の拡縮を変更するUI
			ImGui::DragFloat3("Translate(Triangle2)", &transform2.translate.x, 0.01f); // Triangle2の位置を変更するUI
			ImGui::ColorEdit4("Color(Triangle2)", &materialData2->x, true); // 色の値を変更するUI
			ImGui::Checkbox("isRotate(Triangle2)", &isRotate2); // 回転するかどうかのUI
			ImGui::Separator();
			ImGui::Text("Sprite"); // Title
			ImGui::DragFloat3("Translate(Sprite)", &transformSprite.translate.x, 1.0f); // Spriteの位置を変更するUI
			ImGui::ColorEdit4("Color(Sprite)", &materialDataSprite->x, true); // 色の値を変更するUI
			*/
			ImGui::Text("TextureChange"); // Title
			ImGui::Checkbox("useUvChecker", &useUvChecker); // UvCheckerを使うかどうかのUI
			ImGui::Checkbox("useMonsterBall", &useMonsterBall); // モンスターボールを使うかどうかのUI
			ImGui::Checkbox("useModelTexture", &useModelTexture); // Modelのtextureを使うかどうかのUI
			ImGui::End();
			// ImGuiのデモ用のUIを表示している
			// 開発用のUIの処理。実際に開発用のUIを出す場合はここをゲーム固有の処理に置き換える
			//ImGui::ShowDemoWindow();
			// ImGuiの内部コマンドを生成する
			ImGui::Render();

			//directionalLightの正規化
			directionalLightData->direction = Math::Normalize(directionalLightData->direction);

			// 三角形の回転処理
			if (isRotate) {
				transform.rotate.y += 0.03f;
			} else {
				transform.rotate.y = 0.0f;
			}
			if (isRotate2) {
				transform2.rotate.y += 0.03f;
			} else {
				transform2.rotate.y = 0.0f;
			}
			// 各種行列の処理
			// カメラ
			Matrix4x4 cameraMatrix = Math::MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);

			// 三角形
			Matrix4x4 worldMatrix = Math::MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
			Matrix4x4 worldMatrix2 = Math::MakeAffineMatrix(transform2.scale, transform2.rotate, transform2.translate);
			Matrix4x4 viewMatrix = Math::Inverse(cameraMatrix);
			Matrix4x4 projectionMatrix = Math::MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrix = Math::Multiply(Math::Multiply(worldMatrix, viewMatrix), projectionMatrix);
			Matrix4x4 worldViewProjectionMatrix2 = Math::Multiply(Math::Multiply(worldMatrix2, viewMatrix), projectionMatrix);
			wvpData->WVP = worldViewProjectionMatrix;
			wvpData->world = worldMatrix;
			wvpData2->WVP = worldViewProjectionMatrix2;
			wvpData2->world = worldMatrix2;

			// スプライト
			Matrix4x4 worldMatrixSprite = Math::MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
			Matrix4x4 viewMatrixSprite = Math::Inverse(cameraMatrix);
			Matrix4x4 projectionMatrixSprite = Math::MakeOrthographicMatrix(0.0f, 0.0f, static_cast<float>(kClientWidth), static_cast<float>(kClientHeight), 0.0f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSprite = Math::Multiply(Math::Multiply(worldMatrixSprite, viewMatrixSprite), projectionMatrixSprite);
			wvpDataSprite->WVP= worldViewProjectionMatrixSprite;
			wvpDataSprite->world = worldMatrixSprite;

			// 球
			Matrix4x4 worldMatrixSphere = Math::MakeAffineMatrix(transformSphere.scale, transformSphere.rotate, transformSphere.translate);
			Matrix4x4 viewMatrixSphere = Math::Inverse(cameraMatrix);
			Matrix4x4 projectionMatrixSphere = Math::MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixSphere = Math::Multiply(Math::Multiply(worldMatrixSphere, viewMatrixSphere), projectionMatrixSphere);
			wvpDataSphere->WVP = worldViewProjectionMatrixSphere;
			wvpDataSphere->world = worldMatrixSphere;

			// モデル
			Matrix4x4 worldMatrixModel = Math::MakeAffineMatrix(transformModel.scale, transformModel.rotate, transformModel.translate);
			Matrix4x4 viewMatrixModel = Math::Inverse(cameraMatrix);
			Matrix4x4 projectionMatrixModel = Math::MakePerspectiveFovMatrix(0.45f, static_cast<float>(kClientWidth) / static_cast<float>(kClientHeight), 0.1f, 100.0f);
			Matrix4x4 worldViewProjectionMatrixModel = Math::Multiply(Math::Multiply(worldMatrixModel, viewMatrixModel), projectionMatrixModel);
			wvpDataModel->WVP = worldViewProjectionMatrixModel;
			wvpDataModel->world = worldMatrixModel;

			// uv(sprite)
			Matrix4x4 uvTransformSpriteMatrix = Math::MakeScaleMatrix(uvTransformSprite.scale);
			uvTransformSpriteMatrix = Math::Multiply(uvTransformSpriteMatrix, Math::MakeRotateZMatrix(uvTransformSprite.rotate.z));
			uvTransformSpriteMatrix = Math::Multiply(uvTransformSpriteMatrix, Math::MakeTranslateMatrix(uvTransformSprite.translate));
			materialDataSprite->uvtransform = uvTransformSpriteMatrix;

			// これから書き込むバックバッファのインデックスを取得
			UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();
			// TransitionBarrierを設定する
			D3D12_RESOURCE_BARRIER barrier{};
			// 今回のバリアはTransition
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			// Noneにしておく
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			// バリアを張る対象のリソース。現在のバックバッファに対して行う
			barrier.Transition.pResource = swapChainResources[backBufferIndex];
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
			ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
			commandList->SetDescriptorHeaps(1, descriptorHeaps);
			commandList->RSSetViewports(1, &viewport); // Viewportを設定
			commandList->RSSetScissorRects(1, &scissorRect); // Scissorを設定
			// RootSignatureを設定。PSOに設定しているけど別途設定が必要
			commandList->SetGraphicsRootSignature(pso->GetRootSignature());
			commandList->SetPipelineState(pso->GetPipelineState()); // PSOを設定
			//形状を設定。PSOに設定しているものとはまた別。同じものを設定するトポロジ考えておけばいい。
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// SRVのDescriptorTableの先頭を設定。2はrootParameter[2]である。
			if (useUvChecker) {
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);
			} else if (useMonsterBall) {
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU2);
			} else if (useModelTexture) {
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPUModel);
			} else {
				commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU3);
			}

			// 1つ目の三角形の描画
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView); //VBVを設定
			// マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
			// wvp用のCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
			// directionalLight
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			// 描画！(DrawCall/ドローコール)。6頂点で1つのインスタンス。インスタンスについては今後
			//commandList->DrawInstanced(3, 1, 0, 0);

			// 2つ目の三角形の描画
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView2); //VBVを設定
			// マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResource2->GetGPUVirtualAddress());
			// wvp用のCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1, wvpResource2->GetGPUVirtualAddress());
			// directionalLight
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			// 描画！(DrawCall/ドローコール)。6頂点で1つのインスタンス。インスタンスについては今後
			//commandList->DrawInstanced(3, 1, 0, 0);

			// Spriteの描画
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite); //VBVを設定
			// マテリアルCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
			// TransformationMatrixCBufferの場所を設定
			commandList->SetGraphicsRootConstantBufferView(1, wvpResourceSprite->GetGPUVirtualAddress());
			// directionalLight
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			// インデックスバッファ設定
			commandList->IASetIndexBuffer(&indexBufferViewSprite);
			// 描画！(DrawCall/ドローコール)。6頂点で1つのインスタンス。インスタンスについては今後
			//commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

			// Sphereの描画
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSphere);
			// マテリアルカラーなどの定数バッファ（スロット0にマテリアルリソースを設定）
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceSphere->GetGPUVirtualAddress());
			// WVP行列などの定数バッファ（スロット1に変換行列リソースを設定）
			commandList->SetGraphicsRootConstantBufferView(1, wvpResourceSphere->GetGPUVirtualAddress());
			// directionalLight
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			// インデックスバッファ設定
			commandList->IASetIndexBuffer(&indexBufferViewSphere);
			// 描画コマンド
			//commandList->DrawIndexedInstanced(kIndexCount, 1, 0, 0, 0);

			// Modelの描画
			commandList->IASetVertexBuffers(0, 1, &vertexBufferViewModel);
			// マテリアルカラーなどの定数バッファ（スロット0にマテリアルリソースを設定）
			commandList->SetGraphicsRootConstantBufferView(0, materialResourceModel->GetGPUVirtualAddress());
			// WVP行列などの定数バッファ（スロット1に変換行列リソースを設定）
			commandList->SetGraphicsRootConstantBufferView(1, wvpResourceModel->GetGPUVirtualAddress());
			// directionalLight
			commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
			// 描画コマンド
			commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);



			// 実際のcommandListのImGuiの描画コマンドを積む
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
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
			ID3D12CommandList* commandLists[] = { commandList }; // コマンドリストの配列
			commandQueue->ExecuteCommandLists(1, commandLists); // コマンドリストを実行する
			// GPUとOSに画面の交換を行うよう通知する
			swapChain->Present(1, 0); // 1はV-Syncのための待機。0は何もしない
			// Fenceの値を更新
			fenceValue++;
			// GPUがここまでたどり着いた時に、Fenceの値を指定した値に代入するようにSignalを送る
			commandQueue->Signal(fence, fenceValue);
			// Fenceの値が指定した値にたどり着いているか確認する
			// GetCompletedValueの初期値はFence作成時に渡した初期値
			if (fence->GetCompletedValue() < fenceValue) {
				// 指定した値にたどり着いていないので、イベントが発生するまで待機する
				fence->SetEventOnCompletion(fenceValue, fenceEvent);
				// イベントが発生するまで待機する
				WaitForSingleObject(fenceEvent, INFINITE);
			}

			// 次のフレーム用のコマンドリストを準備
			hr = commandList->Reset(commandAllocator, nullptr); // コマンドリストをリセットする
			// コマンドリストのリセットがうまくいかなかったので起動できない
			assert(SUCCEEDED(hr));
		}
	}

	// ImGuiの終了処理
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// マッピング解除が必要なリソースはUnmapを行う（例：materialResource, wvpResource）
	if (materialResource) {
		materialResource->Unmap(0, nullptr);
		materialResource->Release();
		materialResource = nullptr;
	}
	if (materialResource2) {
		materialResource2->Unmap(0, nullptr);
		materialResource2->Release();
		materialResource2 = nullptr;
	}
	if (materialResourceSprite) {
		materialResourceSprite->Unmap(0, nullptr);
		materialResourceSprite->Release();
		materialResourceSprite = nullptr;
	}
	if (materialResourceSphere) {
		materialResourceSphere->Unmap(0, nullptr);
		materialResourceSphere->Release();
		materialResourceSphere = nullptr;
	}
	if (materialResourceModel) {
		materialResourceModel->Unmap(0, nullptr);
		materialResourceModel->Release();
		materialResourceModel = nullptr;
	}
	if (wvpResource) {
		wvpResource->Unmap(0, nullptr);
		wvpResource->Release();
		wvpResource = nullptr;
	}
	if (wvpResource2) {
		wvpResource2->Unmap(0, nullptr);
		wvpResource2->Release();
		wvpResource2 = nullptr;
	}
	if (wvpResourceSprite) {
		wvpResourceSprite->Unmap(0, nullptr);
		wvpResourceSprite->Release();
		wvpResourceSprite = nullptr;
	}
	if (wvpResourceSphere) {
		wvpResourceSphere->Unmap(0, nullptr);
		wvpResourceSphere->Release();
		wvpResourceSphere = nullptr;
	}
	if (wvpResourceModel) {
		wvpResourceModel->Unmap(0, nullptr);
		wvpResourceModel->Release();
		wvpResourceModel = nullptr;
	}
	if (indexResourceSprite) {
		indexResourceSprite->Unmap(0, nullptr);
		indexResourceSprite->Release();
		indexResourceSprite = nullptr;
	}
	if (indexResourceSphere) {
		indexResourceSphere->Unmap(0, nullptr);
		indexResourceSphere->Release();
		indexResourceSphere = nullptr;
	}

	// そのほかのリソース
	if (vertexResource) {
		vertexResource->Release();
		vertexResource = nullptr;
	}
	if (vertexResource2) {
		vertexResource2->Release();
		vertexResource2 = nullptr;
	}
	if (vertexResourceSprite) {
		vertexResourceSprite->Release();
		vertexResourceSprite = nullptr;
	}
	if (vertexResourceSphere) {
		vertexResourceSphere->Release();
		vertexResourceSphere = nullptr;
	}
	if (vertexResourceModel) {
		vertexResourceModel->Release();
		vertexResourceModel = nullptr;
	}
	if (textureResource) {
		textureResource->Release();
		textureResource = nullptr;
	}
	if (textureResource2) {
		textureResource2->Release();
		textureResource2 = nullptr;
	}
	if (textureResource3) {
		textureResource3->Release();
		textureResource3 = nullptr;
	}
	if (textureResourceModel) {
		textureResourceModel->Release();
		textureResourceModel = nullptr;
	}
	if (intermediateResource) {
		intermediateResource->Release();
		intermediateResource = nullptr;
	}
	if (intermediateResource2) {
		intermediateResource2->Release();
		intermediateResource2 = nullptr;
	}
	if (intermediateResource3) {
		intermediateResource3->Release();
		intermediateResource3 = nullptr;
	}
	if (intermediateResourceModel) {
		intermediateResourceModel->Release();
		intermediateResourceModel = nullptr;
	}
	if (depthStencilResource) {
		depthStencilResource->Release();
		depthStencilResource = nullptr;
	}
	if (dsvDescriptorHeap) {
		dsvDescriptorHeap->Release();
		dsvDescriptorHeap = nullptr;
	}
	if (directionalLightResource) {
		directionalLightResource->Release();
		directionalLightResource = nullptr;
	}

	// DXGI / D3D12関連リソースも同様に
	if (fenceEvent) {
		CloseHandle(fenceEvent);
		fenceEvent = nullptr;
	}
	if (fence) {
		fence->Release();
		fence = nullptr;
	}
	if (rtvDescriptorHeap) {
		rtvDescriptorHeap->Release();
		rtvDescriptorHeap = nullptr;
	}
	if (srvDescriptorHeap) {
		srvDescriptorHeap->Release();
		srvDescriptorHeap = nullptr;
	}
	if (swapChainResources[0]) {
		swapChainResources[0]->Release();
		swapChainResources[0] = nullptr;
	}
	if (swapChainResources[1]) {
		swapChainResources[1]->Release();
		swapChainResources[1] = nullptr;
	}
	if (swapChain) {
		swapChain->Release();
		swapChain = nullptr;
	}
	if (commandList) {
		commandList->Release();
		commandList = nullptr;
	}
	if (commandAllocator) {
		commandAllocator->Release();
		commandAllocator = nullptr;
	}
	if (commandQueue) {
		commandQueue->Release();
		commandQueue = nullptr;
	}
	if (device) {
		device->Release();
		device = nullptr;
	}
	if (useAdapter) {
		useAdapter->Release();
		useAdapter = nullptr;
	}
	if (dxgiFactory) {
		dxgiFactory->Release();
		dxgiFactory = nullptr;
	}
#ifdef _DEBUG
	if (debugController) {
		debugController->Release();
		debugController = nullptr;
	}
#endif

	// ウィンドウのクローズは最後に行うのが無難
	if (hwnd) {
		CloseWindow(hwnd);
		hwnd = nullptr;
	}

	// PSO の解放
	if (pso) {
		delete pso;
		pso = nullptr;
	}

	//  出力ウィンドウへの文字出力
	OutputDebugStringA("Hello, DirectX!\n");

	// リソースリークチェック
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		debug->Release();
	}

	// 警告時に止まる
	//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

	// COMの終了処理
	CoUninitialize();
	return 0;
}