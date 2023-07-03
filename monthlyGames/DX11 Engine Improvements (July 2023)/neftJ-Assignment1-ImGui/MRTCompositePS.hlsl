struct VertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D sceneColors : register(t0);
Texture2D ambient : register(t1);
SamplerState basicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    // Sample our textures
    float3 sceneColor = sceneColors.Sample(basicSampler, input.uv).rgb;
    float3 ambientColor = ambient.Sample(basicSampler, input.uv).rgb;
    
    return float4(pow(ambientColor + sceneColor, 1.0f / 2.2f), 1);
}