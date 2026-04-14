#pragma once
#include <d3d12.h>
#include <vector>
#include <string>

class InputLayoutBuilder {
public:
    InputLayoutBuilder();
    ~InputLayoutBuilder() = default;

    // 要素を追加
    void Add(const char* semanticName, DXGI_FORMAT format);

    // 最終的に LayoutDesc を作成して返す
    const D3D12_INPUT_LAYOUT_DESC& Build();

private:
    std::vector<std::string> semanticNameStorage_;
    std::vector<D3D12_INPUT_ELEMENT_DESC> elements_;
    uint32_t offset_; // 自動オフセット

    D3D12_INPUT_LAYOUT_DESC layoutDesc_;
};
