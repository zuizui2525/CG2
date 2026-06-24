#include "Fullscreen.hlsli"

Texture2D<float4> gTexture : register(t0);       // 元画像
Texture2D<float4> gNoiseTexture : register(t1);  // ノイズテクスチャ
SamplerState gSampler : register(s0);

cbuffer DissolveParams : register(b0) {
    float gThreshold;   // しきい値
    float gEdgeWidth;   // 境界線幅
    float2 pad;         // パディング
    float3 gEdgeColor;  // 境界色
    float pad2;         // パディング
};

float4 main(VertexShaderOutput input) : SV_TARGET {
    // 元画像のカラーをサンプリング
    float4 originalColor = gTexture.Sample(gSampler, input.texcoord);
    
    // ノイズテクスチャをサンプリング (R成分をノイズ値として使用)
    float noiseValue = gNoiseTexture.Sample(gSampler, input.texcoord).r;
    
    // ディゾルブしきい値が0.0ならそのまま元画像を出力
    if (gThreshold <= 0.0f) {
        return originalColor;
    }
    
    // しきい値判定：ノイズ値がしきい値以下ならディゾルブで消える（黒にする）
    if (noiseValue < gThreshold) {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    // 境界線（エッジ）の描画
    if (noiseValue < gThreshold + gEdgeWidth) {
        float t = (noiseValue - gThreshold) / gEdgeWidth;
        float4 edgeColor = float4(gEdgeColor, 1.0f);
        return lerp(edgeColor, originalColor, t);
    }
    
    return originalColor;
}
