#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct PostProcessParams {
    int32_t enableGrayscale;
    int32_t enableSepia;
    int32_t enableVignette;
    float32_t vignetteScale;
    float32_t vignetteExponent;
};
ConstantBuffer<PostProcessParams> gParams : register(b0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // BT.709規格に基づいた輝度ウェイト定数
    static const float32_t3 kLuminanceWeights = float32_t3(0.2125f, 0.7154f, 0.0721f);

    // 1. Grayscale
    if (gParams.enableGrayscale != 0) {
        float32_t grayValue = dot(output.color.rgb, kLuminanceWeights);
        output.color.rgb = float32_t3(grayValue, grayValue, grayValue);
    }

    // 2. Sepia
    if (gParams.enableSepia != 0) {
        float32_t grayValue = dot(output.color.rgb, kLuminanceWeights);
        static const float32_t3 kSepiaColorScale = float32_t3(1.0f, 74.0f / 107.0f, 43.0f / 107.0f);
        output.color.rgb = grayValue * kSepiaColorScale;
    }

    // 3. Vignette
    if (gParams.enableVignette != 0) {
        float32_t2 correct = input.texcoord * (1.0f - input.texcoord.yx);
        float32_t vignette = correct.x * correct.y * gParams.vignetteScale;
        vignette = saturate(pow(vignette, gParams.vignetteExponent));
        output.color.rgb *= vignette;
    }

    return output;
}
