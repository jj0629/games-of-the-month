#include "Lighting.hlsli"

cbuffer externalData : register(b0)
{
    matrix viewMatrix;
    matrix projMatrix;
    float2 screenSize;
    float refractionScale;
    float3 cameraPos;
};

struct VertexToPixel
{
    float4 screenPosition : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 worldPos : POSITION; // The world position of this vertex
};

Texture2D NormalMap : register(t0);
Texture2D RoughnessMap : register(t1);

TextureCube environmentMap : register(t2);

Texture2D screenPixels : register(t3);
Texture2D refractionSilhouette : register(t4);

SamplerState BasicSampler : register(s0);
SamplerState ClampSampler : register(s1);

// Fresnel term - Schlick approx.
float SimpleFresnel(float3 n, float3 v, float f0)
{
    // Pre-calculation
    float NdotV = saturate(dot(n, v));
    // Final value
    return f0 + (1 - f0) * pow(1 - NdotV, 5);
}

float4 main(VertexToPixel input) : SV_Target
{
    // Normalize input values
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    input.normal = NormalMapping(NormalMap, BasicSampler, input.uv, input.normal, input.tangent);
    
    float2 screenUV = input.screenPosition.xy / screenSize;
    
    float2 offsetUV = NormalMap.Sample(BasicSampler, input.uv).xy * 2 - 1;
    offsetUV.y *= -1;
    
    float2 refractedUV = screenUV + offsetUV * refractionScale;
    
    float silhouette = refractionSilhouette.Sample(ClampSampler, refractedUV).r;
    if (silhouette < 1)
    {
        refractedUV = screenUV;
    }
    
    float3 sceneColor = pow(screenPixels.Sample(ClampSampler, refractedUV).rgb, 2.2f);
    
    float3 viewToCam = normalize(cameraPos - input.worldPos);
    float3 viewRefl = normalize(reflect(-viewToCam, input.normal));
    float3 envSample = environmentMap.Sample(BasicSampler, viewRefl).rgb;
    
    float fresnel = SimpleFresnel(input.normal, viewToCam, F0_NON_METAL);
    
    return float4(pow(lerp(sceneColor, envSample, fresnel), 1.0f / 2.2f), 1);
}