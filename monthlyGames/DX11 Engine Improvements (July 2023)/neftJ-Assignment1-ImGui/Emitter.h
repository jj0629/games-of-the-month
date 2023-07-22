#pragma once
#include <DirectXMath.h>
#include <vector>
#include <d3d11.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "Transform.h"
#include "Material.h"
#include "Camera.h"
#include <memory>

#define RandomRange(min, max) ((float)rand() / RAND_MAX * (max - min) + min);

struct Particle
{
	float EmitTime;
	DirectX::XMFLOAT3 StartPos;
	DirectX::XMFLOAT3 StartVelocity;
	DirectX::XMFLOAT3 CurrentPos;
	float CurrentAge;
	float Padding;
};

struct EmitterData
{
	int LivingIndex;
	int DeadIndex;
	int AliveParticleCount;
	int MaxParticles;
	float MaxAge;
	DirectX::XMFLOAT3 Padding;
};

class Emitter
{
public:
	Emitter(Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		unsigned int mp,
		float eps,
		float pma,
		DirectX::XMFLOAT3 accel,
		DirectX::XMFLOAT3 spawnRange,
		DirectX::XMFLOAT3 startVel,
		DirectX::XMFLOAT3 velRange,
		std::shared_ptr<Material> mat,
		std::shared_ptr<SimpleComputeShader> particleUpdateCS,
		std::shared_ptr<SimpleComputeShader> particleEmitCS);
	~Emitter();

	void Update(float currentTime, float deltaTime);
	void Draw(std::shared_ptr<Camera> camera, float currentTime);

	// Getters
	Transform* GetTransform();
	std::shared_ptr<Material> GetMaterial();
	DirectX::XMFLOAT3 GetAcceleration();
	DirectX::XMFLOAT3 GetSpawnRange();
	DirectX::XMFLOAT3 GetStartVelocity();
	DirectX::XMFLOAT3 GetVelocityRange();

	// Setters
	void SetMaterial(std::shared_ptr<Material> mat);
	void SetAcceleration(DirectX::XMFLOAT3 accel);
	void SetSpawnRange(DirectX::XMFLOAT3 spawnRg);
	void SetStartVelocity(DirectX::XMFLOAT3 startVel);
	void SetVelocityRange(DirectX::XMFLOAT3 velRange);

private:
	// System ComPtrs
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	
	// Particle information
	unsigned int maxParticles;
	Particle* particles;
	unsigned int livingIndex;
	unsigned int deadIndex;
	unsigned int aliveParticleCount;

	// Emission information
	float emissionsPerSecond;
	float secondsPerEmission;
	float timeSinceLastEmission;

	// Emitter information
	float particleMaxAge;
	Transform transform;
	std::shared_ptr<Material> material;
	std::shared_ptr<SimpleComputeShader> updateParticleCS;
	std::shared_ptr<SimpleComputeShader> emitParticleCS;
	DirectX::XMFLOAT3 particleAcceleration;
	DirectX::XMFLOAT3 particlePositionRange;
	DirectX::XMFLOAT3 startVelocity;
	DirectX::XMFLOAT3 velocityRange;

	// Buffer Information
	Microsoft::WRL::ComPtr<ID3D11Buffer> particleDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particleDataSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> particleDataUAV;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> emitterDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> emitterDataSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> emitterDataUAV;

	void CreateBuffers();
	void CopyBufferToGPU();
	void EmitParticle(float currentTime);
};

