#include "Object3d.hlsli"

Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

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
    float32_t intensity;
    float32_t radius;
    float32_t decay;
    float32_t2 padding;
};
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<Camera> gCamera : register(b1);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);
ConstantBuffer<PointLight> gPointLight : register(b3);
struct PixelShaderOutput
{
    float32_t4 color : SV_TARGET0;
};
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    float4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    float cos = 1.0f;
    float32_t3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
    float32_t3 normal = normalize(input.normal);
    
    if (gMaterial.enableLighting == 1) // ランバート
    {
        // --- 1. 平行光源の計算 ---
        float cosDir = saturate(dot(normal, -gDirectionalLight.direction));
        float32_t3 diffuseDir = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cosDir * gDirectionalLight.intensity;
        
        float32_t3 halfVectorDir = normalize(-gDirectionalLight.direction + toEye);
        float specularPowDir = pow(saturate(dot(normal, halfVectorDir)), gMaterial.shininess);
        float32_t3 specularDir = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPowDir;

        // --- 2. 点光源の計算 (ここを追加) ---
        float32_t3 pointLightDirection = input.worldPosition - gPointLight.position;
        float dist = length(pointLightDirection);
        float32_t3 l = normalize(-pointLightDirection); // 入射光方向
        
        float factor = pow(saturate(-dist / gPointLight.radius + 1.0), gPointLight.decay); // 減衰
        
        float cosPoint = saturate(dot(normal, l)); // 点光源のランバート
        float32_t3 diffusePoint = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosPoint * gPointLight.intensity;
        
        float32_t3 halfVectorPoint = normalize(l + toEye);
        float specularPowPoint = pow(saturate(dot(normal, halfVectorPoint)), gMaterial.shininess);
        float32_t3 specularPoint = gPointLight.color.rgb * gPointLight.intensity * specularPowPoint;

        // --- 3. 合体 ---
        output.color.rgb = (diffuseDir + specularDir) + (diffusePoint + specularPoint) * factor;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else if (gMaterial.enableLighting == 2) // ハーフランバート
    {
        // --- 1. 平行光源の計算 ---
        float NdotLDir = dot(normal, -gDirectionalLight.direction);
        float cosDir = pow(NdotLDir * 0.5f + 0.5f, 2.0f);
        float32_t3 diffuseDir = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cosDir * gDirectionalLight.intensity;
        
        float32_t3 reflectLightDir = reflect(gDirectionalLight.direction, normal);
        float specularPowDir = pow(saturate(dot(reflectLightDir, toEye)), gMaterial.shininess);
        float32_t3 specularDir = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPowDir;

        // --- 2. 点光源の計算 (ここを追加) ---
        float32_t3 pointLightDirection = input.worldPosition - gPointLight.position;
        float dist = length(pointLightDirection);
        float32_t3 l = normalize(-pointLightDirection);
        
        float factor = pow(saturate(-dist / gPointLight.radius + 1.0), gPointLight.decay); // 減衰
        
        float NdotLPoint = dot(normal, l);
        float cosPoint = pow(NdotLPoint * 0.5f + 0.5f, 2.0f); // 点光源のハーフランバート
        float32_t3 diffusePoint = gMaterial.color.rgb * textureColor.rgb * gPointLight.color.rgb * cosPoint * gPointLight.intensity;
        
        float32_t3 halfVectorPoint = normalize(l + toEye);
        float specularPowPoint = pow(saturate(dot(normal, halfVectorPoint)), gMaterial.shininess);
        float32_t3 specularPoint = gPointLight.color.rgb * gPointLight.intensity * specularPowPoint;

        // --- 3. 合体 ---
        output.color.rgb = (diffuseDir + specularDir) + (diffusePoint + specularPoint) * factor;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    else// Lightingしない場合、前回までと同じ演算
    {
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
        output.color.a = gMaterial.color.a * textureColor.a;
    }
    
    if (textureColor.a == 0.0 || output.color.a == 0.0f || output.color.a <= 0.2f)
    {
        discard;
    }
    return output;
}
