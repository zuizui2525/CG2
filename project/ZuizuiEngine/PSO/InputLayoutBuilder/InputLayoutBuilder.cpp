#include "InputLayoutBuilder.h"
#include <cassert>

InputLayoutBuilder::InputLayoutBuilder()
    : offset_(0) {
}

void InputLayoutBuilder::Add(const char* semanticName, DXGI_FORMAT format) {
    D3D12_INPUT_ELEMENT_DESC desc{};

    // 文字列自体は保存しておく
    semanticNameStorage_.push_back(semanticName);

    // ★重要: ここでは SemanticName には一旦 nullptr や一時的な値を入れておく
    // (ポインタの解決は Build で行うため)
    desc.SemanticName = nullptr;

    desc.SemanticIndex = 0;
    desc.Format = format;
    desc.InputSlot = 0;
    desc.AlignedByteOffset = offset_;
    desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    desc.InstanceDataStepRate = 0;

    // Formatのバイトサイズ推定
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT: offset_ += 16; break;
    case DXGI_FORMAT_R32G32B32_FLOAT: offset_ += 12; break;
    case DXGI_FORMAT_R32G32_FLOAT: offset_ += 8; break;
    default: assert(false);
    }

    elements_.push_back(desc);
}

const D3D12_INPUT_LAYOUT_DESC& InputLayoutBuilder::Build() {
    // ★重要: ここでポインタを紐付け直す
    // elements_ と semanticNameStorage_ の要素数は必ず一致しているはず
    for (size_t i = 0; i < elements_.size(); ++i) {
        elements_[i].SemanticName = semanticNameStorage_[i].c_str();
    }

    layoutDesc_.pInputElementDescs = elements_.data();
    layoutDesc_.NumElements = (UINT)elements_.size();

    return layoutDesc_;
}
