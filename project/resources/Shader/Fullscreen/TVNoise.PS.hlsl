#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);       // 元画像
SamplerState gSampler : register(s0);

cbuffer NoiseParams : register(b0) {
    float gTime;            // 時間
    float gNoiseStrength;   // ノイズの強度 (0.0: 無し 〜 1.0: 砂嵐のみ)
    float2 pad;             // 16バイトアライメント
};

// 疑似乱数生成関数 (ピクセル座標と時間をシードにする)
float random(float2 co) {
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(VertexShaderOutput input) : SV_TARGET {
    // 元画像のカラーをサンプリング
    float4 originalColor = gTexture.Sample(gSampler, input.texcoord);

    // 時間変化をシード値に乗算してノイズを毎フレーム不規則に動かす
    // 時間が0の時に乱数が固定化するのを防ぐため、(gTime + 1.0f) を掛け合わせます
    float noise = random(input.texcoord * (gTime + 1.0f));

    // 砂嵐（グレースケールノイズ）のカラー
    float4 noiseColor = float4(noise, noise, noise, 1.0f);

    // 強度に応じて元画像とブレンド
    return lerp(originalColor, noiseColor, gNoiseStrength);
}
