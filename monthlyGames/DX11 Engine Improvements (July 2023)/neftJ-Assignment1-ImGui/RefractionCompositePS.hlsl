struct VertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D sceneComposite : register(t0);
Texture2D refractionComposite : register(t1);
SamplerState basicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
    // Sample our textures
    float3 sceneColor = sceneComposite.Sample(basicSampler, input.uv).rgb;
    float3 refractionColor = refractionComposite.Sample(basicSampler, input.uv).rgb;
    
    if (any(refractionColor))
    {
        return float4(pow(refractionColor, 1.0f / 2.2f), 1);

    }
    else
    {
        return float4(pow(sceneColor, 1.0f / 2.2f), 1);
    }
}