struct VertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

cbuffer externalData : register(b0)
{
    float3 colorTint;
    float3 cameraPosition;
    float2 uvScale;
    float2 uvOffset;
};

Texture2D Texture : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_Target
{
    return Texture.Sample(BasicSampler, input.uv);
}