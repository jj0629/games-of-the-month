struct VertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D pixels : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_Target
{
    return pixels.Sample(BasicSampler, input.uv);

}