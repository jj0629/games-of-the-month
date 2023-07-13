#include "Lighting.hlsli"

struct Particle
{
    float EmitTime;
    float3 StartPos;
    float3 StartVelocity;
    float Padding;
};

cbuffer externalData : register(b0)
{
    matrix world;
    matrix worldInverseTranspose;
    matrix view;
    matrix projection;
    float currentTime;
    float3 acceleration;
};

RWStructuredBuffer<Particle> ParticleData : register(u0);

[numthreads(2, 2, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    // Find our offsets by using the thread numbers. Since the offset numbers will only be -1 or 1, we can use the DTid's x and y values
    // to calculate what the final offsets should be.
    float2 offsets = float2(-1.0f + (DTid.x * 2.0f), -1.0f + (DTid.y * 2.0f));
    float2 uvs = float2(DTid.xy);

}