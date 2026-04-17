#include "Engine/Base/Utils/DxUtils.h"
#include <cassert>

namespace DxUtils {

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
        // 頂点リソース用のヒープの設定
        D3D12_HEAP_PROPERTIES uploadHeapProperties{};
        uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // Uploadheapを使う

        // 頂点リソースの設定
        D3D12_RESOURCE_DESC vertexResourceDesc{};
        vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        vertexResourceDesc.Width = sizeInBytes; // リソースのサイズ
        vertexResourceDesc.Height = 1;
        vertexResourceDesc.DepthOrArraySize = 1;
        vertexResourceDesc.MipLevels = 1;
        vertexResourceDesc.SampleDesc.Count = 1;
        vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        // 実際に頂点リソースを作る
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        HRESULT hr = device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &vertexResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource));

        assert(SUCCEEDED(hr));
        return resource;
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        ID3D12Device* device,
        D3D12_DESCRIPTOR_HEAP_TYPE heapType,
        UINT numDescriptors,
        bool shaderVisible) {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
        descriptorHeapDesc.Type = heapType;
        descriptorHeapDesc.NumDescriptors = numDescriptors;
        descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
        assert(SUCCEEDED(hr));
        return descriptorHeap;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
        // 生成するResourceの設定
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Width = width;
        resourceDesc.Height = height;
        resourceDesc.MipLevels = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        // 利用するheapの設定
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

        // 深度値のクリア設定
        D3D12_CLEAR_VALUE depthClearValue{};
        depthClearValue.DepthStencil.Depth = 1.0f;
        depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

        // Resourceの生成
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        HRESULT hr = device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthClearValue,
            IID_PPV_ARGS(&resource));

        assert(SUCCEEDED(hr));
        return resource;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
        D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        handleCPU.ptr += static_cast<size_t>(descriptorSize) * index;
        return handleCPU;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
        D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        handleGPU.ptr += static_cast<size_t>(descriptorSize) * index;
        return handleGPU;
    }

} // namespace DxUtils
