struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer FocusParams : register(b0)
{
    float nearFocus;
    float farFocus;
    float2 focusCenter;
    float focusIntensity;
}

struct ObjectSplit_Output
{
    float4 foreground : SV_TARGET0;
    float4 background : SV_TARGET1;
};

Texture2D sceneColors : register(t0);
Texture2D sceneDepths : register(t1);

SamplerState basicSampler : register(s0);

ObjectSplit_Output main(VertexToPixel input)
{
    ObjectSplit_Output output;
    
    float depth = sceneDepths.Sample(basicSampler, input.uv);
    
    output.foreground = float4(0, 0, 0, 0);
    output.background = float4(0, 0, 0, 0);
    
    if (depth < nearFocus)
    {
        output.foreground = sceneColors.Sample(basicSampler, input.uv);
    }
    if (depth > farFocus)
    {
        output.background = sceneColors.Sample(basicSampler, input.uv);
    }
    
    return output;
}