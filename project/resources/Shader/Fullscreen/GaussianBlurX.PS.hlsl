// 定数バッファ (b0)
cbuffer GaussianParams : register(b0) {
    int32_t gKernelRadius; // k (1: 3x3, 2: 5x5, 3: 7x7, 4: 9x9 等)
    float32_t gSigma;      // 標準偏差σ
    float32_t2 gPad;
};

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

static const float32_t PI = 3.14159265f;

// 1次元ガウス関数
float gauss(float d, float sigma) {
    float exponent = -(d * d) * rcp(2.0f * sigma * sigma);
    float denominator = sqrt(2.0f * PI) * sigma;
    return exp(exponent) * rcp(denominator);
}

struct VSOutput {
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

PSOutput main(VSOutput input) {
    PSOutput output;
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(float32_t(width)), rcp(float32_t(height)));
    
    float32_t3 sumColor = float32_t3(0.0f, 0.0f, 0.0f);
    float32_t sumWeight = 0.0f;
    
    // 左右方向 (X) の畳み込み
    for (int32_t d = -gKernelRadius; d <= gKernelRadius; ++d) {
        float32_t2 offsetUV = input.texcoord + float32_t2(float32_t(d), 0.0f) * uvStepSize;
        float32_t w = gauss(float32_t(d), gSigma);
        
        sumColor += gTexture.Sample(gSampler, offsetUV).rgb * w;
        sumWeight += w;
    }
    
    // 重みの合計で割り、正規化する
    output.color.rgb = sumColor * rcp(sumWeight);
    output.color.a = 1.0f;
    
    return output;
}
