#include "Fullscreen.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct UnderwaterParams {
    float32_t time;
    float32_t distortionStrength;
    float32_t distortionFrequency;
    float32_t blurStrength;
    float32_t blurWeight;
    float32_t waterColorIntensity;
    float32_t2 pad; // 16byte alignment
    float32_t3 waterColor;
    float32_t pad2; // 16byte alignment
};
ConstantBuffer<UnderwaterParams> gUnderwater : register(b0);

struct PixelShaderOutput {
    float32_t4 color : SV_TARGET0;
};

static const float32_t PI = 3.14159265f;

// 1-dimensional Gaussian function (referenced from original GaussianBlur)
float32_t gauss(float32_t d, float32_t sigma) {
    float32_t exponent = -(d * d) * rcp(2.0f * sigma * sigma);
    float32_t denominator = sqrt(2.0f * PI) * sigma;
    return exp(exponent) * rcp(denominator);
}

PixelShaderOutput main(VertexShaderOutput input) {
    PixelShaderOutput output;
    
    float32_t2 uv = input.texcoord;
    
    // 1. Water Ripple Distortion (Yurayura)
    float32_t distortionX = sin(uv.y * gUnderwater.distortionFrequency + gUnderwater.time) * gUnderwater.distortionStrength;
    float32_t distortionY = cos(uv.x * gUnderwater.distortionFrequency + gUnderwater.time) * gUnderwater.distortionStrength;
    float32_t2 distortedUV = uv + float32_t2(distortionX, distortionY);
    
    // Clamp to prevent sampling outside of texture borders
    distortedUV = saturate(distortedUV);
    
    // 2. Pseudo Depth Blur (Gaussian Blurring for Far Objects)
    // Assume Y=0.45 is the far horizon. Close to Y=0.45 is "far", top and bottom are "near".
    float32_t distFromHorizon = abs(distortedUV.y - 0.45f);
    float32_t pseudoDepth = saturate(1.0f - distFromHorizon * 2.0f);
    
    uint32_t width, height;
    gTexture.GetDimensions(width, height);
    float32_t2 uvStepSize = float32_t2(rcp(float32_t(width)), rcp(float32_t(height)));
    
    // Calculate Gaussian weights dynamically (Fast 5-tap Gaussian Blur)
    float32_t offsetDist = pseudoDepth * gUnderwater.blurStrength;
    float32_t sigma = max(gUnderwater.blurStrength * 0.5f, 0.1f);
    
    float32_t wCenter = gauss(0.0f, sigma);
    float32_t wOffset = gauss(offsetDist, sigma);
    
    float32_t weights[5] = { wCenter, wOffset, wOffset, wOffset, wOffset };
    float32_t2 offsets[5] = {
        float32_t2(0.0f, 0.0f),
        float32_t2(-1.0f, 0.0f), float32_t2(1.0f, 0.0f),
        float32_t2(0.0f, -1.0f), float32_t2(0.0f, 1.0f)
    };
    
    float32_t4 sumColor = float32_t4(0.0f, 0.0f, 0.0f, 0.0f);
    float32_t sumWeight = 0.0f;
    
    for (int32_t i = 0; i < 5; i++) {
        float32_t2 sampleUV = distortedUV + offsets[i] * offsetDist * uvStepSize;
        sampleUV = saturate(sampleUV);
        
        sumColor += gTexture.Sample(gSampler, sampleUV) * weights[i];
        sumWeight += weights[i];
    }
    
    float32_t4 blurColor = sumColor * rcp(sumWeight);
    
    float32_t4 baseColor = gTexture.Sample(gSampler, distortedUV);
    baseColor = lerp(baseColor, blurColor, pseudoDepth * gUnderwater.blurWeight);
    
    // 3. Optional Water Color Tint (Default to inactive: Intensity = 0.0)
    if (gUnderwater.waterColorIntensity > 0.0f) {
        float32_t3 darkWater = gUnderwater.waterColor * 0.05f;
        float32_t3 lightWater = gUnderwater.waterColor * 1.2f;
        float32_t3 fogColor = lerp(lightWater, darkWater, distortedUV.y);
        
        // Deeper fog density at the bottom
        float32_t fogDensity = lerp(0.25f, 0.55f, distortedUV.y) * gUnderwater.waterColorIntensity;
        baseColor.rgb = lerp(baseColor.rgb, fogColor, fogDensity);
    }
    
    output.color = baseColor;
    return output;
}
