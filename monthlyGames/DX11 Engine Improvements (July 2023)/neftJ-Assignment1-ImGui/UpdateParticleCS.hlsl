#include "Particles.hlsli"

cbuffer data : register(b0)
{
    float currentTime;
    float3 acceleration;
    uint startIndex;
    float3 padding;
};

RWStructuredBuffer<Particle> ParticleData : register(u0);
RWStructuredBuffer<Emitter> EmitterData : register(u1);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    Emitter eData = EmitterData.Load(0);
    uint particleId = (startIndex + DTid.x) % eData.MaxParticles;
    
    Particle p = ParticleData.Load(particleId);
   
    // Calculate age
    p.CurrentAge = currentTime - p.EmitTime;
    
    // Move particle
    //float3 pos = p.StartPos + (age * p.Direction);
    p.CurrentPos = acceleration * p.CurrentAge * p.CurrentAge / 2.0f + p.StartVelocity * p.CurrentAge + p.StartPos;
    if (p.CurrentAge >= eData.MaxAge)
    {
        eData.LivingIndex++;
        eData.LivingIndex = eData.LivingIndex % eData.MaxParticles;
        eData.AliveParticleCount--;
    }
    EmitterData[0] = eData;
    ParticleData[particleId] = p;
}