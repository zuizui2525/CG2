#include "Fullscreen.hlsli"

struct RadialBlurParams {
    float32_t2 center;
    float32_t blurWidth;
};

ConstantBuffer<RadialBlurParams> gParams : register(b0);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    const int32_t kNumSamples = 10; // サンプリング数

    // 中心から現在のuvに対しての方向を計算
    float32_t2 direction = input.texcoord - gParams.center;
    float32_t3 outputColor = float32_t3(0.0f, 0.0f, 0.0f);

    for (int32_t sampleIndex = 0; sampleIndex < kNumSamples; ++sampleIndex) {
        // 現在のuvから、計算した方向にサンプリング点を進めながらサンプリング
        float32_t2 texcoord = input.texcoord + direction * gParams.blurWidth * float32_t(sampleIndex);
        outputColor.rgb += gTexture.Sample(gSampler, texcoord).rgb;
    }
    
    // 平均化する
    outputColor.rgb *= rcp(kNumSamples);

    PixelShaderOutput output;
    output.color.rgb = outputColor;
    output.color.a = 1.0f;
    return output;
}
