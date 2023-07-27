#include "Particles.hlsli"

cbuffer data : register(b0)
{
    float currentTime;
    float3 startPos;
    float3 startVel;
    float3 randPos;
    float3 randVel;
    uint startIndex;
};

RWStructuredBuffer<Particle> ParticleData : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint particleId = startIndex + DTid.x;
    Particle p = ParticleData.Load(particleId);

    p.EmitTime = currentTime;
	
	// Decide particle spawning information
    p.StartPos = startPos;
    p.StartPos.x += randPos.x;
    p.StartPos.y += randPos.y;
    p.StartPos.z += randPos.z;

    p.StartVelocity = startVel;
    p.StartVelocity.x += randVel.x;
    p.StartVelocity.y += randVel.y;
    p.StartVelocity.z += randVel.z;
    
    ParticleData[particleId] = p;
}