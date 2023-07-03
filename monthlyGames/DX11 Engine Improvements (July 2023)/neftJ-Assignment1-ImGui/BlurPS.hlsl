struct VertexToPixel
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer externalData : register(b0)
{
    uint texWidth;
    uint texHeight;
    uint2 padding;
}

Texture2D texToBlur : register(t0);
Texture2D blurMask : register(t1);
SamplerState basicSampler : register(s0);

float4 main(VertexToPixel input) : SV_Target
{
    float2 uvOffset = (1.0f / texWidth, 1.0f / texHeight);
    float2 offsets[8];
    offsets[0] = float2(input.uv.x - uvOffset.x, input.uv.y);
    offsets[1] = float2(input.uv.x - uvOffset.x, input.uv.y + uvOffset.y);
    offsets[2] = float2(input.uv.x, input.uv.y + uvOffset.y);
    offsets[3] = float2(input.uv.x + uvOffset.x, input.uv.y + uvOffset.y);
    offsets[4] = float2(input.uv.x + uvOffset.x, input.uv.y);
    offsets[5] = float2(input.uv.x + uvOffset.x, input.uv.y - uvOffset.y);
    offsets[6] = float2(input.uv.x, input.uv.y - uvOffset.y);
    offsets[7] = float2(input.uv.x - uvOffset.x, input.uv.y - uvOffset.y);
    
    float4 color = 0;
    
    float4 mask = blurMask.Sample(basicSampler, input.uv);
    
    if (all(mask))
    {
        for (int i = 0; i < 8; i++)
        {
            color += texToBlur.Sample(basicSampler, offsets[i]);
        }
        color = color / 8;
    }
    else
    {
        color = texToBlur.Sample(basicSampler, input.uv);
    }

    return color;
}