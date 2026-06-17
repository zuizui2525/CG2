// 定数バッファ (b0)
struct DepthOutlineParams {
    float32_t4x4 projectionInverse;
    float32_t3 edgeColor;
    float32_t edgeWidth;
    float32_t threshold;
    float32_t scale; // 検出感度の倍率
    float32_t pad[2]; // 16バイトアライメント用
};
ConstantBuffer<DepthOutlineParams> gParams : register(b0);

Texture2D<float32_t4> gTexture : register(t0);
Texture2D<float32_t> gDepthTexture : register(t1);

SamplerState gSampler : register(s0);
SamplerState gSamplerPoint : register(s1);

struct VSOutput {
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
};

struct PSOutput {
    float32_t4 color : SV_TARGET0;
};

static const float32_t kPrewittHorizontalKernel[3][3] = {
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};

static const float32_t kPrewittVerticalKernel[3][3] = {
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    {  0.0f,         0.0f,         0.0f },
    {  1.0f / 6.0f,  1.0f / 6.0f,  1.0f / 6.0f },
};

static const int32_t2 kIndex3x3[3][3] = {
    { { -1, -1 }, { 0, -1 }, { 1, -1 } },
    { { -1,  0 }, { 0,  0 }, { 1,  0 } },
    { { -1,  1 }, { 0,  1 }, { 1,  1 } }
};

// NDC深度からビュー空間深度(viewZ)を復元する関数
float32_t GetViewZ(float32_t2 texcoord) {
    float32_t ndcDepth = gDepthTexture.Sample(gSamplerPoint, texcoord).r;
    float32_t4 viewSpace = mul(float32_t4(0.0f, 0.0f, ndcDepth, 1.0f), gParams.projectionInverse);
    return viewSpace.z * rcp(viewSpace.w);
}

PSOutput main(VSOutput input) {
    PSOutput output;

    // 1. テクスチャの解像度から1テクセルあたりの移動量を算出
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(float32_t(width)), rcp(float32_t(height)));

    // gParams.edgeWidth倍した移動量
    float32_t2 stepSize = uvStepSize * gParams.edgeWidth;

    float32_t2 difference = float32_t2(0.0f, 0.0f);

    // 3x3 Prewittフィルターで畳み込み
    for (int32_t x = 0; x < 3; ++x) {
        for (int32_t y = 0; y < 3; ++y) {
            float32_t2 texcoord = input.texcoord + float32_t2(kIndex3x3[x][y]) * stepSize;
            float32_t viewZ = GetViewZ(texcoord);
            
            difference.x += viewZ * kPrewittHorizontalKernel[x][y];
            difference.y += viewZ * kPrewittVerticalKernel[x][y];
        }
    }

    // 差分の大きさを計算
    float32_t weight = length(difference);
    // スケールを適用して感度を調整
    weight = weight * gParams.scale;
    // 閾値判定
    if (weight < gParams.threshold) {
        weight = 0.0f;
    }
    weight = saturate(weight);

    // 元画像の色をサンプリング
    float32_t3 originalColor = gTexture.Sample(gSampler, input.texcoord).rgb;

    // エッジ部分を黒（または指定色）にブレンドして出力
    output.color.rgb = lerp(originalColor, gParams.edgeColor, weight);
    output.color.a = 1.0f;

    return output;
}
