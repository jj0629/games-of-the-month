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
    bool isActive;
};

struct Emitter
{
    uint LivingIndex;
    uint DeadIndex;
    uint AliveParticleCount;
    uint MaxParticles;
    float MaxAge;
    float3 Padding;
};

struct ParticleVertexToPixel
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    bool isActive : STATUS;
};

#endif