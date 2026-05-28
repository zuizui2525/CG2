#ifdef _USEIMGUI
#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"

class ImguiManager {
public:
    void Initialize(HWND hwnd, ID3D12Device* device, int backBufferCount,
        DXGI_FORMAT rtvFormat,
        ID3D12DescriptorHeap* rtvHeap,
        ID3D12DescriptorHeap* srvHeap);

    void Begin();   // フレーム開始（NewFrame系）
    void End();     // フレーム終了（Render + Draw）
    void Shutdown();// 解放処理

private:
    bool initialized_ = false;
};
#endif
