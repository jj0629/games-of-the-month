#include "Particles.hlsli"

cbuffer externalData : register(b0)
{
    matrix world;
    matrix worldInverseTranspose;
    matrix view;
    matrix projection;
};

StructuredBuffer<Particle> ParticleData : register(t0);

ParticleVertexToPixel main(uint id : SV_VertexID)
{
    ParticleVertexToPixel output;
    
    uint particleID = id / 4;
    uint cornerID = id % 4;
    
    Particle p = ParticleData.Load(particleID);
    
    // Set the offsets for this corner. It's only one of four options, not hard to figure out.
    float2 offsets[4];
    offsets[0] = float2(-1.0f, +1.0f); // TL
    offsets[1] = float2(+1.0f, +1.0f); // TR
    offsets[2] = float2(+1.0f, -1.0f); // BR
    offsets[3] = float2(-1.0f, -1.0f); // BL
    
    // Implement the billboarding effect and offsets in one fell  swoop
    p.CurrentPos += float3(view._11, view._12, view._13) * offsets[cornerID].x; 
    p.CurrentPos += float3(view._21, view._22, view._23) * offsets[cornerID].y;
    
    // Finally update position by camera stuffs
    matrix viewProj = mul(projection, view);
    output.position = mul(viewProj, float4(p.CurrentPos, 1.0f));
    
    float2 uvs[4];
    uvs[0] = float2(0, 0); // TL
    uvs[1] = float2(1, 0); // TR
    uvs[2] = float2(1, 1); // BR
    uvs[3] = float2(0, 1); // BL
    output.uv = uvs[cornerID];
    
	return output;
}