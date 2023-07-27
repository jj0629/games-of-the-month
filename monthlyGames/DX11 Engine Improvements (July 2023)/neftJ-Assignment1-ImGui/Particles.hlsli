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

struct Emitter
{
    uint LivingIndex;
    uint DeadIndex;
    uint AliveParticleCount;
    uint MaxParticles;
    float MaxAge;
    float3 Padding;
};

#endif