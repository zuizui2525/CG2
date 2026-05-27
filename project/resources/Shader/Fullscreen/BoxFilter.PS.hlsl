// 定数バッファ (b0)
cbuffer BoxFilterParams : register(b0) {
    int32_t gKernelRadius; // k (1 = 3x3, 2 = 5x5)
    float32_t3 gPad;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput {
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VSOutput input) {
    PSOutput output;
    
    // 1. テクスチャ解像度から1テクセルあたりのUV移動量(uvStepSize)を算出
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(float32_t(width)), rcp(float32_t(height)));
    
    float32_t3 sumColor = float32_t3(0.0f, 0.0f, 0.0f);
    int32_t sampleCount = 0;
    
    // 2. 動的なカーネル半径 gKernelRadius (k) を用いたループ処理
    // k=1 のとき x, y は -1 から 1 (3x3 = 9ピクセル)
    // k=2 のとき x, y は -2 から 2 (5x5 = 25ピクセル)
    for (int32_t x = -gKernelRadius; x <= gKernelRadius; ++x) {
        for (int32_t y = -gKernelRadius; y <= gKernelRadius; ++y) {
            float32_t2 offsetUV = input.texcoord + float32_t2(x, y) * uvStepSize;
            
            // サンプラーは CLAMP モードに設定されているため、端のUV境界外サンプリングは自動的に端のピクセル色になります
            sumColor += gTexture.Sample(gSampler, offsetUV).rgb;
            sampleCount++;
        }
    }
    
    // 3. すべての色の平均値を算出
    output.color.rgb = sumColor / float32_t(sampleCount);
    output.color.a = 1.0f;
    
    return output;
}
