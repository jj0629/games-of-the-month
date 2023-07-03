#pragma once

#include <DirectXMath.h>

// --------------------------------------------------------
// A custom vertex definition
//
// You will eventually ADD TO this, and/or make more of these!
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The position of the vertex
	DirectX::XMFLOAT2 UV;			// Texture mapping
	DirectX::XMFLOAT3 Normal;		// Lighting
	DirectX::XMFLOAT3 Tangent;		// Normal mapping
};

struct ParticleVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Velocity;
	DirectX::XMFLOAT2 UV;
	DirectX::XMFLOAT4 Color;
	float Age;
};