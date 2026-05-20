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
    output.color.rgb = float32_t3(grayValue, grayValue, grayValue);
    
    return output;
}
