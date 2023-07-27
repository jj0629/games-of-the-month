// Include guard
#ifndef _PARTICLES_HLSL
#define _PARTICLES_HLSL

struct Particle
{
    float EmitTime;
    float3 StartPos;
    float3 StartVelocity;
    float3 CurrentPos;
    float CurrentAge;
    float Padding;
};

struct ParticleVertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
};

#endif