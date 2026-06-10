#ifdef _USEIMGUI
#include "Engine/Debug/ReplaySystem.h"
#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/DxUtils.h"
#include <cassert>
#include <chrono>
#include <algorithm>
#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")

ReplaySystem* ReplaySystem::instance_ = nullptr;


namespace {
    // リソースバリア遷移の簡易ヘルパー
    void TransitionBarrier(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter) {
        if (!resource || stateBefore == stateAfter) {
            return;
        }
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        commandList->ResourceBarrier(1, &barrier);
    }
}

ReplaySystem* ReplaySystem::GetInstance() {
    if (!instance_) {
        instance_ = new ReplaySystem();
    }
    return instance_;
}

void ReplaySystem::Initialize(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, int32_t width, int32_t height) {
    device_ = device;
    srvHeap_ = srvHeap;
    currentWidth_ = width;
    currentHeight_ = height;
    isPaused_ = false;
    needsCopy_ = false;
    seekProgress_ = 1.0f;
    frameCounter_ = 0;
    records_.clear();
    lastPausedGpuHandle_ = {};

    // 縮小描画用の一時RTVヒープの生成
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;
    HRESULT hr = device_->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeapShrink_));
    assert(SUCCEEDED(hr));

    // 縮小描画用シェーダーのコンパイルとPSOの生成
    const char shrinkShaderCode[] = R"(
    struct VSOutput {
        float4 pos : SV_Position;
        float2 uv : TEXCOORD;
    };

    VSOutput VSMain(uint vertexID : SV_VertexID) {
        VSOutput output;
        output.uv = float2((vertexID << 1) & 2, vertexID & 2);
        output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
        return output;
    }

    Texture2D<float4> gTexture : register(t0);
    SamplerState gSampler : register(s0);

    float4 PSMain(VSOutput input) : SV_Target {
        return gTexture.Sample(gSampler, input.uv);
    }
    )";

    // ルートシグネチャの作成
    D3D12_DESCRIPTOR_RANGE descRange{};
    descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descRange.NumDescriptors = 1;
    descRange.BaseShaderRegister = 0;
    descRange.RegisterSpace = 0;
    descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParam{};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 1;
    rootParam.DescriptorTable.pDescriptorRanges = &descRange;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = 1;
    rootSigDesc.pParameters = &rootParam;
    rootSigDesc.NumStaticSamplers = 1;
    rootSigDesc.pStaticSamplers = &samplerDesc;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if (SUCCEEDED(hr)) {
        device_->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&shrinkRootSignature_));
    }

    // シェーダーのビルド
    Microsoft::WRL::ComPtr<ID3DBlob> vsBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> psBlob;
    hr = D3DCompile(shrinkShaderCode, sizeof(shrinkShaderCode), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vsBlob, &errorBlob);
    assert(SUCCEEDED(hr));
    hr = D3DCompile(shrinkShaderCode, sizeof(shrinkShaderCode), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &psBlob, &errorBlob);
    assert(SUCCEEDED(hr));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = shrinkRootSignature_.Get();
    psoDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    psoDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    psoDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
    psoDesc.PS.BytecodeLength = psBlob->GetBufferSize();
    psoDesc.SampleMask = UINT_MAX;

    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
        FALSE, FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
    }

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.InputLayout = { nullptr, 0 };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;

    hr = device_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&shrinkPipelineState_));
    assert(SUCCEEDED(hr));

    RecreatePool();
}

void ReplaySystem::Finalize() {
    records_.clear();
    garbageCollectTextures_.clear();
    
    delete instance_;
    instance_ = nullptr;
}

void ReplaySystem::ClearGarbage() {
    garbageCollectTextures_.clear();
}


void ReplaySystem::OnResize(int32_t width, int32_t height) {
    if (currentWidth_ == width && currentHeight_ == height) {
        return;
    }
    currentWidth_ = width;
    currentHeight_ = height;

    // 録画用テクスチャは640x360固定サイズになったため、リサイズ時のテクスチャ再生成・履歴クリアは不要です。
    // これによりリサイズ（ドッキング）操作時のクラッシュバグ要因が根本的に排除され、極めて堅牢になります。
}


void ReplaySystem::RecreatePool() {
    totalCreatedCount_ = 0;
}

void ReplaySystem::RecordFrame(ID3D12GraphicsCommandList* commandList, ID3D12Resource* sourceTexture, D3D12_GPU_DESCRIPTOR_HANDLE sourceSrv, float fps, float memory) {
    if (isPaused_ || !device_ || !sourceTexture) return;

    frameCounter_++;
    if (frameCounter_ < kRecordInterval) {
        return;
    }
    frameCounter_ = 0;

    // 起動からの経過時間を取得
    float timestamp = 0.0f;
    auto now = std::chrono::steady_clock::now();
    // LogクラスからstartTime_を取得できないため、一時的にシステム時間等の基準で記録するか、
    // あるいはGetTickCount64()などをベースにする。ここでは再現性の高いミリ秒カウントを使用。
    static auto startAppTime = std::chrono::steady_clock::now();
    timestamp = std::chrono::duration<float>(now - startAppTime).count();

    FrameRecord newRecord;
    
    // リングバッファが最大数に達している場合、最古のテクスチャを再利用してアロケーションを回避
    if (records_.size() >= static_cast<size_t>(kMaxFrames)) {
        newRecord = std::move(records_[0]);
        records_.erase(records_.begin());

        // 再利用するテクスチャのサイズが一致しているか確認（もしサイズ変更などで不一致なら再生成）
        if (newRecord.texture) {
            D3D12_RESOURCE_DESC desc = newRecord.texture->GetDesc();
            if (desc.Width != static_cast<UINT64>(kShrinkWidth) || desc.Height != static_cast<UINT>(kShrinkHeight)) {
                newRecord.texture.Reset();
            }
        }
    }

    // テクスチャが無い場合は新規作成
    if (!newRecord.texture) {
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resDesc{};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        resDesc.Width = kShrinkWidth;
        resDesc.Height = kShrinkHeight;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.SampleDesc.Count = 1;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // レンダーターゲットを許可

        HRESULT hr = device_->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, // 初期状態をサンプリング可能に
            nullptr,
            IID_PPV_ARGS(&newRecord.texture)
        );
        if (FAILED(hr)) {
            Log::Write(std::format("ReplaySystem: Record texture creation failed with HRESULT: 0x{:08X}", static_cast<uint32_t>(hr)));
            return;
        }

        // 新規作成されたテクスチャに対する個別 SRV の生成
        UINT descriptorSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        UINT srvIndex = kReservedSrvIndexReplayStart + totalCreatedCount_;
        totalCreatedCount_++;

        newRecord.srvCpuHandle = DxUtils::GetCPUDescriptorHandle(srvHeap_, descriptorSize, srvIndex);
        newRecord.srvGpuHandle = DxUtils::GetGPUDescriptorHandle(srvHeap_, descriptorSize, srvIndex);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        device_->CreateShaderResourceView(newRecord.texture.Get(), &srvDesc, newRecord.srvCpuHandle);
    }

    newRecord.fps = fps;
    newRecord.memory = memory;
    newRecord.timestamp = timestamp;

    // GPU上での縮小描画の実行
    // 1. 描画先のテクスチャを RENDER_TARGET 状態に遷移
    TransitionBarrier(commandList, newRecord.texture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 2. テンポラリRTVヒープにRTVを生成してバインド
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeapShrink_->GetCPUDescriptorHandleForHeapStart();
    device_->CreateRenderTargetView(newRecord.texture.Get(), &rtvDesc, rtvHandle);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // 3. ビューポート・シザー矩形の設定 (640x360固定)
    D3D12_VIEWPORT vp{};
    vp.Width = static_cast<float>(kShrinkWidth);
    vp.Height = static_cast<float>(kShrinkHeight);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    commandList->RSSetViewports(1, &vp);

    D3D12_RECT sr{};
    sr.left = 0;
    sr.top = 0;
    sr.right = kShrinkWidth;
    sr.bottom = kShrinkHeight;
    commandList->RSSetScissorRects(1, &sr);

    // 4. パイプライン・ルートシグネチャ・テクスチャのバインド
    commandList->SetPipelineState(shrinkPipelineState_.Get());
    commandList->SetGraphicsRootSignature(shrinkRootSignature_.Get());
    commandList->SetGraphicsRootDescriptorTable(0, sourceSrv);

    // 5. 描画実行 (インプットレイアウト無しのフルスクリーン描画)
    commandList->DrawInstanced(3, 1, 0, 0);

    // 6. コピー完了後、履歴テクスチャを常時状態である PIXEL_SHADER_RESOURCE へ戻す
    TransitionBarrier(commandList, newRecord.texture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // 新しいレコードを履歴末尾に追加
    records_.push_back(std::move(newRecord));

    // 通常稼働中は常に最新を指すようにシーク位置を1.0に保つ
    seekProgress_ = 1.0f;
    needsCopy_ = true;
}


void ReplaySystem::SetPause(bool pause) {
    if (isPaused_ == pause) return;

    if (!pause) {
        // ポーズ解除の直前に、現在表示しているリプレイフレームのSRVハンドルをキャッシュして静止表示用とする
        lastPausedGpuHandle_ = GetReplaySrvGpuHandle();
    }

    isPaused_ = pause;
    isReplayPlaying_ = false; // 再生状態に入った場合はリプレイ再生を止める

    if (isPaused_) {
        seekProgress_ = 1.0f;
        needsCopy_ = true;
    } else {
        seekProgress_ = 1.0f;
    }
}

void ReplaySystem::SetSeekPos(float progress) {
    seekProgress_ = std::clamp(progress, 0.0f, 1.0f);
    isReplayPlaying_ = false; // 手動シークした場合は自動再生を一時停止
    if (!isPaused_) {
        SetPause(true); // 自動で一時停止状態にする
    }
    needsCopy_ = true;
}

D3D12_GPU_DESCRIPTOR_HANDLE ReplaySystem::GetReplaySrvGpuHandle() const {
    if (records_.empty()) return {};

    // 通常動作中（一時停止していない）は、最後に一時停止した時点のフレーム（キャッシュ）を返す
    if (!isPaused_ && lastPausedGpuHandle_.ptr != 0) {
        return lastPausedGpuHandle_;
    }

    int32_t numRecords = static_cast<int32_t>(records_.size());
    int32_t targetIdx = static_cast<int32_t>(seekProgress_ * (numRecords - 1));
    targetIdx = std::clamp(targetIdx, 0, numRecords - 1);
    return records_[targetIdx].srvGpuHandle;
}


float ReplaySystem::GetReplayFps(int32_t targetIdx) const {
    if (records_.empty()) return 0.0f;
    int32_t idx = std::clamp(targetIdx, 0, static_cast<int32_t>(records_.size() - 1));
    return records_[idx].fps;
}

float ReplaySystem::GetReplayMemory(int32_t targetIdx) const {
    if (records_.empty()) return 0.0f;
    int32_t idx = std::clamp(targetIdx, 0, static_cast<int32_t>(records_.size() - 1));
    return records_[idx].memory;
}

float ReplaySystem::GetReplayTimeOffset(int32_t targetIdx) const {
    if (records_.empty()) return 0.0f;
    int32_t idx = std::clamp(targetIdx, 0, static_cast<int32_t>(records_.size() - 1));
    float currentTimestamp = records_.back().timestamp;
    return currentTimestamp - records_[idx].timestamp;
}

float ReplaySystem::GetReplayMaxTimestamp() const {
    if (!isPaused_ || records_.empty()) {
        return -1.0f; // 無制限
    }
    int32_t numRecords = static_cast<int32_t>(records_.size());
    int32_t targetIdx = static_cast<int32_t>(seekProgress_ * (numRecords - 1));
    targetIdx = std::clamp(targetIdx, 0, numRecords - 1);
    return records_[targetIdx].timestamp;
}

void ReplaySystem::GetReplayHistory(int32_t targetIdx, float* outFpsHistory, float* outMemHistory, int32_t historySize) {
    int32_t numRecords = static_cast<int32_t>(records_.size());
    if (numRecords == 0) {
        std::fill_n(outFpsHistory, historySize, 0.0f);
        std::fill_n(outMemHistory, historySize, 0.0f);
        return;
    }

    int32_t baseIdx = std::clamp(targetIdx, 0, numRecords - 1);

    for (int32_t i = 0; i < historySize; ++i) {
        // historySize のぶんだけ過去のインデックスを計算する
        // 例: historySize = 120, i = 119 が最新（baseIdx）, i = 0 が最も古い（baseIdx - 119）
        int32_t idx = baseIdx - (historySize - 1 - i);
        if (idx < 0) {
            idx = 0; // 過去の記録が足りない場合は最古の値で埋める
        }
        outFpsHistory[i] = records_[idx].fps;
        outMemHistory[i] = records_[idx].memory;
    }
}

void ReplaySystem::UpdateReplayPlay(float deltaTime) {
    if (!isReplayPlaying_ || records_.empty() || records_.size() <= 1) {
        return;
    }

    // 30fps想定でシークを進める（1秒間に30フレーム分）
    float speed = (30.0f / static_cast<float>(records_.size() - 1)) * deltaTime;
    seekProgress_ += speed;

    if (seekProgress_ >= 1.0f) {
        seekProgress_ = 1.0f;
        isReplayPlaying_ = false; // 自動停止
    }
    needsCopy_ = true;
}
#endif
