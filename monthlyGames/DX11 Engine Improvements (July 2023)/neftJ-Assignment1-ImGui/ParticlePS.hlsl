#include "Particles.hlsli"

cbuffer externalData : register(b0)
{
    float3 colorTint;
    float3 cameraPosition;
    float2 uvScale;
    float2 uvOffset;
};

Texture2D Texture : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(ParticleVertexToPixel input) : SV_Target
{
    if (input.isActive == false)
    {
        discard;
    }
    return Texture.Sample(BasicSampler, input.uv);
}