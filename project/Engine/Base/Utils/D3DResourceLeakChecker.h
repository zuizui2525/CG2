#pragma once
#include <wrl.h>          // ComPtr
#include <dxgidebug.h>    // DXGI デバッグ
#include <dxgi1_3.h>      // DXGIGetDebugInterface1 のため
#include <d3d12.h>

#pragma comment(lib, "dxguid.lib") // 必要なライブラリをリンク

// D3D12 リソースリークチェッカー
struct D3DResourceLeakChecker {
    ~D3DResourceLeakChecker() {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
            debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        }
    }
};
