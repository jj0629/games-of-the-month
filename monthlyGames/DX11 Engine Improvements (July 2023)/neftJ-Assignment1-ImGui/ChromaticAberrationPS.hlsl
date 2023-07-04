struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer chromAbbData : register(b0)
{
    float2 direction;
    float colorSplitDiff;
}

Texture2D sceneColors : register(t0);
SamplerState basicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    float4 rValue = sceneColors.Sample(basicSampler, input.uv + (direction * float2(colorSplitDiff, colorSplitDiff)));
    float4 gValue = sceneColors.Sample(basicSampler, input.uv + direction);
    float4 bValue = sceneColors.Sample(basicSampler, input.uv - (direction * float2(colorSplitDiff, colorSplitDiff)));
    
    return float4(rValue.r, gValue.g, bValue.ba);

}