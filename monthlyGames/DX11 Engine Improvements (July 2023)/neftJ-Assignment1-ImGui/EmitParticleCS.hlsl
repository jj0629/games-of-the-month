struct Particle
{
    float EmitTime;
    float3 StartPos;
    float3 StartVelocity;
    float3 CurrentPos;
    float CurrentAge;
    float Padding;
};

cbuffer data : register(b0)
{
    float currentTime;
    float3 acceleration;
    float3 startPos;
    float3 startVel;
    float startIndex;
};

RWStructuredBuffer<Particle> ParticleData : register(u0);

[numthreads(1, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    
}