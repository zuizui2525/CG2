#ifdef _USEIMGUI
#include "ImguiManager.h"

void ImguiManager::Initialize(HWND hwnd, ID3D12Device* device, int backBufferCount,
    DXGI_FORMAT rtvFormat, ID3D12DescriptorHeap* rtvHeap, ID3D12DescriptorHeap* srvHeap) {
    if (initialized_) return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(device,
        backBufferCount,
        rtvFormat,
        rtvHeap,
        srvHeap->GetCPUDescriptorHandleForHeapStart(),
        srvHeap->GetGPUDescriptorHandleForHeapStart());

    initialized_ = true;
}

void ImguiManager::Begin() {
    if (!initialized_) return;
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void ImguiManager::End() {
    if (!initialized_) return;
    ImGui::Render();
}

void ImguiManager::Shutdown() {
    if (!initialized_) return;
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}
#endif
