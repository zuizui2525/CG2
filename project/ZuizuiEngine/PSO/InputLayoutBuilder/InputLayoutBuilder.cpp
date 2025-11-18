#include "InputLayoutBuilder.h"
#include <cassert>

InputLayoutBuilder::InputLayoutBuilder()
    : offset_(0) {
}

void InputLayoutBuilder::Add(const char* semanticName, DXGI_FORMAT format) {
    D3D12_INPUT_ELEMENT_DESC desc{};
    semanticNameStorage_.push_back(semanticName);

    desc.SemanticName = semanticNameStorage_.back().c_str();
    desc.SemanticIndex = 0;
    desc.Format = format;
    desc.InputSlot = 0;
    desc.AlignedByteOffset = offset_;
    desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    desc.InstanceDataStepRate = 0;

    // Formatのバイトサイズを推定（最低限対応）
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_FLOAT: offset_ += 16; break;
    case DXGI_FORMAT_R32G32B32_FLOAT: offset_ += 12; break;
    case DXGI_FORMAT_R32G32_FLOAT: offset_ += 8; break;
    default: assert(false); // 必要なら追加していく
    }

    elements_.push_back(desc);
}

const D3D12_INPUT_LAYOUT_DESC& InputLayoutBuilder::Build() {
    layoutDesc_.NumElements = (UINT)elements_.size();
    layoutDesc_.pInputElementDescs = elements_.data();
    return layoutDesc_;
}
