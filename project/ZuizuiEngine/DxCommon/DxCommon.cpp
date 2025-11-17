#include "DxCommon.h"
#include <iostream>
#include <thread>

void DxCommon::Initialize(HWND hwnd, int32_t width, int32_t height) {
	InitializeViewport(width, height);
	InitializeScissorRect(width, height);
	EnableDebugLayer();
	CreateAdapter();
	CreateDevice();
	CreateCommandObject();
	CreateSwapChain(hwnd, width, height);
	CreateRenderTargets();
	CreateDepthStencil(width, height);
	CreateFence();
	CreateDXC();
}

void DxCommon::BeginFrame() {
	backBufferIndex_ = swapChain_->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex_].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex_], false, &dsvHandle);
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
	commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex_], clearColor, 0, nullptr);
}

void DxCommon::EndFrame() {
	UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier);

	HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	swapChain_->Present(1, 0);

	static uint64_t fenceValue = 0;
	fenceValue++;
	commandQueue_->Signal(fence_.Get(), fenceValue);
	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (fenceEvent == nullptr) {
		assert(false && "Failed to create fence event.");
		return;
	}
	if (fence_->GetCompletedValue() < fenceValue) {
		fence_->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	CloseHandle(fenceEvent);

	hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
	assert(SUCCEEDED(hr));
}


void DxCommon::PreDraw(ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature) {
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, heaps);
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_); commandList_->SetGraphicsRootSignature(rootSignature);
	commandList_->SetPipelineState(pipelineState);
	commandList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DxCommon::DrawImGui() {
#ifdef _DEBUG
	ID3D12DescriptorHeap* heaps[] = { srvDescriptorHeap_.Get() };
	commandList_->SetDescriptorHeaps(1, heaps);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHandles_[swapChain_->GetCurrentBackBufferIndex()];
	commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList_.Get());
#endif
}

void DxCommon::FrameStart() {
	frameStartTime_ = std::chrono::steady_clock::now();
}

// FPS固定＋経過時間更新
void DxCommon::FrameEnd(int targetFps) {
	using namespace std::chrono;

	if (targetFps <= 0) { targetFps = 60; }

	// 目標フレーム時間（例: 16.666ms）
	const microseconds targetFrameTime(1000000 / targetFps);

	// フレーム終了時刻
	auto endTime = steady_clock::now();

	// 経過時間を計算
	auto elapsed = duration_cast<microseconds>(endTime - frameStartTime_);

	// deltaTime_を秒単位で更新（実測）
	deltaTime_ = static_cast<float>(elapsed.count()) / 1'000'000.0f;

	// もし目標より短ければスリープして調整
	if (elapsed < targetFrameTime) {
		std::this_thread::sleep_for(targetFrameTime - elapsed);
		deltaTime_ = static_cast<float>(targetFrameTime.count()) / 1'000'000.0f;
	}
}

void DxCommon::InitializeViewport(int32_t width, int32_t height) {
	viewport_.Width = static_cast<float>(width);
	viewport_.Height = static_cast<float>(height);
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;
}

void DxCommon::InitializeScissorRect(int32_t width, int32_t height) {
	scissorRect_.left = 0;
	scissorRect_.right = width;
	scissorRect_.top = 0;
	scissorRect_.bottom = height;
}

void DxCommon::EnableDebugLayer() {
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
}

void DxCommon::CreateAdapter() {
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(dxgiFactory_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(useAdapter_.GetAddressOf())) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter_->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			std::wcout << L"Use Adapter: " << adapterDesc.Description << std::endl;
			break;
		}
	}
	assert(useAdapter_ != nullptr);
}

void DxCommon::CreateDevice() {
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

	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		HRESULT hr = D3D12CreateDevice(
			useAdapter_.Get(),
			featureLevels[i],
			IID_PPV_ARGS(device_.GetAddressOf()));
		if (SUCCEEDED(hr)) {
			std::wcout << L"FeatureLevel : " << featureLevelStrings[i] << L"\n";
			break;
		}
	}
	assert(device_ != nullptr);
	std::wcout << L"Complete create D3D12Device!!!\n";

#ifdef _DEBUG
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(infoQueue_.GetAddressOf())))) {
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue_->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		D3D12_MESSAGE_ID denyIds[] = {
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		infoQueue_->PushStorageFilter(&filter);
	}
#endif
}

void DxCommon::CreateCommandObject() {
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	HRESULT hr = device_->CreateCommandQueue(
		&commandQueueDesc,
		IID_PPV_ARGS(commandQueue_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = device_->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(commandAllocator_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator_.Get(),
		nullptr,
		IID_PPV_ARGS(commandList_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void DxCommon::CreateSwapChain(HWND hwnd, int32_t width, int32_t height) {
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = backBufferCount_;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(),
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf()));
	assert(SUCCEEDED(hr));
}

void DxCommon::CreateRenderTargets() {
	rtvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	srvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	HRESULT hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
	assert(SUCCEEDED(hr));

	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	rtvDesc.Format = rtvFormat_;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	rtvHandles_[0] = rtvStartHandle;
	device_->CreateRenderTargetView(
		swapChainResources_[0].Get(),
		&rtvDesc,
		rtvHandles_[0]);

	rtvHandles_[1].ptr = rtvHandles_[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device_->CreateRenderTargetView(
		swapChainResources_[1].Get(),
		&rtvDesc,
		rtvHandles_[1]);
}

void DxCommon::CreateDepthStencil(int32_t width, int32_t height) {
	depthStencilResource_ = CreateDepthStencilTextureResource(device_.Get(), width, height);
	dsvDescriptorHeap_ = CreateDescriptorHeap(device_.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device_->CreateDepthStencilView(
		depthStencilResource_.Get(),
		&dsvDesc,
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DxCommon::CreateFence() {
	uint64_t fenceValue = 0;
	HRESULT hr = device_->CreateFence(
		fenceValue,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(fence_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DxCommon::CreateDXC() {
	HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(dxcUtils_.GetAddressOf()));
	assert(SUCCEEDED(hr));
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(dxcCompiler_.GetAddressOf()));
	assert(SUCCEEDED(hr));

	hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
	assert(SUCCEEDED(hr));
}
