#include "Object3d.hlsli"

// --- リソース定義 ---
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// --- 構造体定義 ---
struct Camera
{
    float32_t3 worldPosition;
    float32_t padding;
};

struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
    float32_t shininess;
};

struct DirectionalLight
{
    float32_t4 color;
    float32_t3 direction;
    float intensity;
};

struct PointLight
{
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float radius;
    float decay;
    float32_t2 padding;
};

struct SpotLight
{
    float32_t4 color;
    float32_t3 position;
    float intensity;
    float32_t3 direction;
    float distance;
    float decay;
    float cosAngle;
    float cosFalloffStart;
    float padding;
};

struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};

// --- 定数バッファ ---
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b1);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
ConstantBuffer<SpotLight> gSpotLight : register(b4);

// --- ヘルパー関数 ---
// ライティングの共通計算（Lambert or Half-Lambert）
float CalculateCos(float NdotL, int32_t mode)
{
    if (mode == 1)
        return saturate(NdotL); // Lambert
    if (mode == 2)
        return pow(NdotL * 0.5f + 0.5f, 2.0f); // Half-Lambert
    return 1.0f;
}

// --- メイン関数 ---
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 1. UV変形とサンプリング
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float32_t3 materialColor = gMaterial.color.rgb * textureColor.rgb;

    // 2. ライティング計算
    if (gMaterial.enableLighting == 0)
    {
        // ライティング無効時
        output.color.rgb = materialColor;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        // ライティング有効時：共通パラメータの準備
        float32_t3 normal = normalize(input.normal);
        float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
        
        float32_t3 totalDiffuse = float32_t3(0.0f, 0.0f, 0.0f);
        float32_t3 totalSpecular = float32_t3(0.0f, 0.0f, 0.0f);

        // --- 2.1. 平行光源 (Directional Light) ---
        {
            float32_t3 L = normalize(-gDirectionalLight.direction);
            float NdotL = dot(normal, L);
            float cos = CalculateCos(NdotL, gMaterial.enableLighting);

            totalDiffuse += materialColor * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;

            float32_t3 halfVector = normalize(L + toEye);
            float spec = pow(saturate(dot(normal, halfVector)), gMaterial.shininess);
            totalSpecular += gDirectionalLight.color.rgb * gDirectionalLight.intensity * spec;
        }

        // --- 2.2. 点光源 (Point Light) ---
        {
            float32_t3 lightVec = input.worldPosition - gPointLight.position;
            float dist = length(lightVec);
            float32_t3 L = normalize(-lightVec);
            float attenuation = pow(saturate(-dist / gPointLight.radius + 1.0f), gPointLight.decay);

            float NdotL = dot(normal, L);
            float cos = CalculateCos(NdotL, gMaterial.enableLighting);

            totalDiffuse += (materialColor * gPointLight.color.rgb * cos * gPointLight.intensity) * attenuation;

            float32_t3 halfVector = normalize(L + toEye);
            float spec = pow(saturate(dot(normal, halfVector)), gMaterial.shininess);
            totalSpecular += (gPointLight.color.rgb * gPointLight.intensity * spec) * attenuation;
        }

        // --- 2.3. スポットライト (Spot Light) ---
        {
            float32_t3 lightVec = input.worldPosition - gSpotLight.position;
            float dist = length(lightVec);
            float32_t3 L = normalize(-lightVec);

            // 減衰計算（距離・角度）
            float distAtten = pow(saturate(-dist / gSpotLight.distance + 1.0f), gSpotLight.decay);
            float cosDiffuse = dot(L, -gSpotLight.direction);
            float angleAtten = saturate((cosDiffuse - gSpotLight.cosAngle) / (gSpotLight.cosFalloffStart - gSpotLight.cosAngle));
            float totalAtten = distAtten * angleAtten;

            float NdotL = dot(normal, L);
            float cos = CalculateCos(NdotL, gMaterial.enableLighting);

            totalDiffuse += (materialColor * gSpotLight.color.rgb * cos * gSpotLight.intensity) * totalAtten;

            float32_t3 halfVector = normalize(L + toEye);
            float spec = pow(saturate(dot(normal, halfVector)), gMaterial.shininess);
            totalSpecular += (gSpotLight.color.rgb * gSpotLight.intensity * spec) * totalAtten;
        }

        // 最終色の合成
        output.color.rgb = totalDiffuse + totalSpecular;
        output.color.a = gMaterial.color.a * textureColor.a;
    }

    // 3. アルファテスト
    if (output.color.a <= 0.2f)
    {
        discard;
    }

    return output;
}
