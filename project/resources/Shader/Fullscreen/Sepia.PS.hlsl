#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // BT.709規格に基づいた輝度ウェイト定数
    static const float32_t3 kLuminanceWeights = float32_t3(0.2125f, 0.7154f, 0.0721f);
    
    // 輝度の算出
    float32_t grayValue = dot(output.color.rgb, kLuminanceWeights);
    
    // セピア色（RGB: 107, 74, 43）を最大輝度基準で正規化した乗算比率
    static const float32_t3 kSepiaColorScale = float32_t3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);
    
    output.color.rgb = grayValue * kSepiaColorScale;
    
    return output;
}
