#include "Particles.hlsli"

cbuffer data : register(b0)
{
    float currentTime;
    float3 startPos;
    float3 startVel;
    float3 randPos;
    float3 randVel;
    float padding;
};

RWStructuredBuffer<Particle> ParticleData : register(u0);
RWStructuredBuffer<Emitter> EmitterData : register(u1);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    Emitter eData = EmitterData.Load(0);
    if (eData.AliveParticleCount >= eData.MaxParticles)
        return;
    
    uint particleId = eData.DeadIndex + DTid.x;
    Particle p = ParticleData.Load(particleId);
    p.CurrentAge = 0;

    p.EmitTime = currentTime;
    p.isActive = true;
	
	// Decide particle spawning information
    p.StartPos = startPos;
    p.StartPos.x += randPos.x;
    p.StartPos.y += randPos.y;
    p.StartPos.z += randPos.z;
    p.CurrentPos = p.StartPos;

    p.StartVelocity = startVel;
    p.StartVelocity.x += randVel.x;
    p.StartVelocity.y += randVel.y;
    p.StartVelocity.z += randVel.z;
    
    ParticleData[particleId] = p;
    
    eData.DeadIndex++;
    eData.DeadIndex %= eData.MaxParticles;
    eData.AliveParticleCount++;
    
    EmitterData[0] = eData;
}