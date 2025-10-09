#pragma once
#include "Function.h"

class DxCommon {
public:
	void Initialize(HWND hwnd, int32_t width, int32_t height);
	void BeginFrame();
	void EndFrame();
	void PreDraw(ID3D12PipelineState* pipelineState, ID3D12RootSignature* rootSignature, const D3D12_VIEWPORT& viewport, const D3D12_RECT& scissorRect);
	void DrawImGui();
	void FrameStart();
	void FrameEnd(int targetFps);
public:
	// --- 基本系 ---
	ID3D12Device* GetDevice() const { return device_.Get(); }
	ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
	ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator_.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
	IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }
	// --- Descriptor Heaps ---
	ID3D12DescriptorHeap* GetRtvHeap() const { return rtvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetSrvHeap() const { return srvDescriptorHeap_.Get(); }
	ID3D12DescriptorHeap* GetDsvHeap() const { return dsvDescriptorHeap_.Get(); }
	// --- RTV / DSV Resource Handles ---
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle(UINT index) const { return rtvHandles_[index]; }
	ID3D12Resource* GetSwapChainResource(UINT index) const { return swapChainResources_[index].Get(); }
	ID3D12Resource* GetDepthStencilResource() const { return depthStencilResource_.Get(); }
	// --- その他 ---
	ID3D12Fence* GetFence() const { return fence_.Get(); }
	IDxcUtils* GetDxcUtils() const { return dxcUtils_.Get(); }
	IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler_.Get(); }
	IDxcIncludeHandler* GetIncludeHandler() const { return includeHandler_.Get(); }
	// --- DXGI / Adapter 情報 ---
	IDXGIFactory7* GetFactory() const { return dxgiFactory_.Get(); }
	IDXGIAdapter4* GetAdapter() const { return useAdapter_.Get(); }
	// --- SwapChain Info ---
	UINT GetBackBufferIndex() const { return backBufferIndex_; }
	UINT GetBackBufferCount() const { return backBufferCount_; }
	DXGI_FORMAT GetRtvFormat() const { return rtvFormat_; }
	// FPS固定
	float GetDeltaTime() const { return deltaTime_; }
private:
	void CreateAdapter();
	void CreateDevice();
	void CreateCommandObject();
	void CreateSwapChain(HWND hwnd, int32_t width, int32_t height);
	void CreateRenderTargets();
	void CreateDepthStencil(int32_t width, int32_t height);
	void CreateFence();
	void CreateDXC();
private:
	// CreateAdapter();
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
	Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter_;
	// CreateDevice();
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;
	// CreateCommandObject();
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
	// CreateSwapChain();
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;
	UINT backBufferIndex_;
	UINT backBufferCount_ = 2;
	// CreateRenderTargets();
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2] = { nullptr };
	DXGI_FORMAT rtvFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	// CreateDepthStencil();
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilResource_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
	// CreateFence();
	Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	// CreateDXC();
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;
	// FPS固定
	std::chrono::steady_clock::time_point frameStartTime_;
	float deltaTime_ = 0.0f;
};