#include "Particles.hlsli"

cbuffer data : register(b0)
{
    float currentTime;
    float3 acceleration;
};

RWStructuredBuffer<Particle> ParticleData : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleId = DTid.x;
    
    Particle p = ParticleData.Load(particleId);
   
    // Calculate age
    p.CurrentAge = currentTime - p.EmitTime;
    
    // Move particle
    //float3 pos = p.StartPos + (age * p.Direction);
    p.CurrentPos = acceleration * p.CurrentAge * p.CurrentAge / 2.0f + p.StartVelocity * p.CurrentAge + p.StartPos;
    ParticleData[DTid.x] = p;
}