#include "Skybox.hlsli"

TextureCube<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // 方向ベクトルを使ってキューブマップからフェッチ
    // サンプラーはエンジンの標準のものを使用
    float32_t4 textureColor = gTexture.Sample(gSampler, input.texcoord);
    output.color = textureColor;
    
    return output;
}
