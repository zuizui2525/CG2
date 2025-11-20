#pragma once
#include <d3d12.h>

enum BlendMode {
    kBlendModeNone,       // ブレンドなし
    kBlendModeNormal,     // 通常αブレンド
    kBlendModeAdd,        // 加算
    kBlendModeSubtract,   // 減算
    kBlendModeMultiply,   // 乗算
    kBlendModeScreen,     // スクリーン
    kCountOfBlendMode,    // カウント用
};

class BlendStateBuilder {
public:
    BlendStateBuilder();
    ~BlendStateBuilder() = default;

    void SetBlendMode(BlendMode mode);
    const D3D12_BLEND_DESC& Build();

private:
    D3D12_BLEND_DESC blendDesc_{};
};
