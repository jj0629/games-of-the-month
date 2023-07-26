#include "Emitter.h"
#include <string>

Emitter::Emitter(Microsoft::WRL::ComPtr<ID3D11Device> device,
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
	std::shared_ptr<SimpleComputeShader> particleEmitCS) :
	device(device),
	context(context),
	particleAcceleration(accel),
	particlePositionRange(spawnRange),
	startVelocity(startVel),
	velocityRange(velRange)
{
	// Set fields
	this->maxParticles = mp;
	this->emissionsPerSecond = eps;
	this->secondsPerEmission = 1.0f / this->emissionsPerSecond;
	this->particleMaxAge = pma;
	this->timeSinceLastEmission = 0;
	this->aliveParticleCount = 0;
	this->livingIndex = 0;
	this->deadIndex = 0;
	this->particles = new Particle[maxParticles];
	this->material = mat;
	this->updateParticleCS = particleUpdateCS;
	this->emitParticleCS = particleEmitCS;

	transform = Transform();

	CreateBuffers();
	CopyBufferToGPU();
}

Emitter::~Emitter()
{
	delete[] particles;
}

void Emitter::Update(float currentTime, float deltaTime)
{
	// See if we need to emit a new particle
	timeSinceLastEmission += deltaTime;
	while (timeSinceLastEmission > secondsPerEmission)
	{
		EmitParticle(currentTime);
		timeSinceLastEmission -= secondsPerEmission;
	}

	// After copying, run the update compute shader to update particle positions and ages
	updateParticleCS->SetShader();
	
	updateParticleCS->SetUnorderedAccessView("ParticleData", particleDataUAV);
	updateParticleCS->SetUnorderedAccessView("EmitterData", emitterDataUAV);
	updateParticleCS->SetFloat("currentTime", currentTime);
	updateParticleCS->SetFloat3("acceleration", DirectX::XMFLOAT3(0, -3.0f, 0));
	updateParticleCS->CopyBufferData(0);
	updateParticleCS->DispatchByThreads(aliveParticleCount, 1, 1);

	updateParticleCS->SetUnorderedAccessView("ParticleData", nullptr);
	updateParticleCS->SetUnorderedAccessView("EmitterData", nullptr);
}

void Emitter::Draw(std::shared_ptr<Camera> camera, float currentTime)
{
	// We're going to run the vertex shader 4 times for every particle (aliveParticleCount * 4)

	// Setup null vertex buffer
	UINT stride = 0;
	UINT offset = 0;
	ID3D11Buffer* nullBuff = 0;
	context->IASetVertexBuffers(0, 1, &nullBuff, &stride, &offset);
	context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Setup shader data
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();
	material->PrepareMaterial(&transform, camera);
	
	vs->SetShaderResourceView("ParticleData", particleDataSRV);
	vs->SetShaderResourceView("EmitterData", emitterDataSRV);

	// Finally make the draw call
	context->DrawIndexed(maxParticles * 6, 0, 0);
	vs->SetShaderResourceView("ParticleData", nullptr);
	vs->SetShaderResourceView("EmitterData", nullptr);
}

Transform* Emitter::GetTransform()
{
	return &transform;
}

std::shared_ptr<Material> Emitter::GetMaterial()
{
	return material;
}

DirectX::XMFLOAT3 Emitter::GetAcceleration()
{
	return this->particleAcceleration;
}

DirectX::XMFLOAT3 Emitter::GetSpawnRange()
{
	return this->particlePositionRange;
}

DirectX::XMFLOAT3 Emitter::GetStartVelocity()
{
	return this->startVelocity;
}

DirectX::XMFLOAT3 Emitter::GetVelocityRange()
{
	return this->velocityRange;
}

void Emitter::SetMaterial(std::shared_ptr<Material> mat)
{
	if (mat.get() == nullptr) return;

	material = mat;
}

void Emitter::SetAcceleration(DirectX::XMFLOAT3 accel)
{
	this->particleAcceleration = accel;
}

void Emitter::SetSpawnRange(DirectX::XMFLOAT3 spawnRg)
{
	this->particlePositionRange = spawnRg;
}

void Emitter::SetStartVelocity(DirectX::XMFLOAT3 startVel)
{
	this->startVelocity = startVel;
}

void Emitter::SetVelocityRange(DirectX::XMFLOAT3 velRange)
{
	this->velocityRange = velRange;
}

void Emitter::CreateBuffers()
{
	// PARTICLE DATA UAV/SRV
	// Create buffer and srv
	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(Particle);
	desc.ByteWidth = sizeof(Particle) * maxParticles;
	device->CreateBuffer(&desc, 0, particleDataBuffer.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = maxParticles;
	device->CreateShaderResourceView(particleDataBuffer.Get(), &srvDesc, particleDataSRV.GetAddressOf());

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = maxParticles;
	device->CreateUnorderedAccessView(particleDataBuffer.Get(), &uavDesc, particleDataUAV.GetAddressOf());

	// EMITTERDATA UAV/SRV
	desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	desc.StructureByteStride = sizeof(EmitterData);
	desc.ByteWidth = sizeof(EmitterData);
	device->CreateBuffer(&desc, 0, emitterDataBuffer.GetAddressOf());

	srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = 1;
	device->CreateShaderResourceView(emitterDataBuffer.Get(), &srvDesc, emitterDataSRV.GetAddressOf());

	uavDesc = {};
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = 1;
	device->CreateUnorderedAccessView(emitterDataBuffer.Get(), &uavDesc, emitterDataUAV.GetAddressOf());

	// INDEX BUFFER
	// Create indices
	unsigned int* indices = new unsigned int[maxParticles * 6];
	int indexCount = 0;
	for (int i = 0; i < maxParticles * 4; i += 4)
	{
		indices[indexCount++] = i;
		indices[indexCount++] = i + 1;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i;
		indices[indexCount++] = i + 2;
		indices[indexCount++] = i + 3;
	}
	D3D11_SUBRESOURCE_DATA iData = {};
	iData.pSysMem = indices;

	// Create index buffer
	D3D11_BUFFER_DESC iDesc = {};
	iDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	iDesc.CPUAccessFlags = 0;
	iDesc.Usage = D3D11_USAGE_DEFAULT;
	iDesc.ByteWidth = sizeof(unsigned int) * maxParticles * 6;
	device->CreateBuffer(&iDesc, &iData, indexBuffer.GetAddressOf());
	delete[] indices;
}

void Emitter::CopyBufferToGPU()
{
	// Create the entire buffer of max particles on the GPU.
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_READ_WRITE, 0, &mapped);
	memcpy(mapped.pData, particles, sizeof(Particle) * maxParticles);
	context->Unmap(particleDataBuffer.Get(), 0);
	
	// Create the emitter data buffer on the GPU
	EmitterData eData = {};
	eData.DeadIndex = this->deadIndex;
	eData.LivingIndex = this->livingIndex;
	eData.MaxAge = this->particleMaxAge;
	eData.MaxParticles = this->maxParticles;
	eData.AliveParticleCount = this->aliveParticleCount;
	mapped = {};
	context->Map(emitterDataBuffer.Get(), 0, D3D11_MAP_READ_WRITE, 0, &mapped);
	memcpy(mapped.pData, &eData, sizeof(EmitterData));
	context->Unmap(emitterDataBuffer.Get(), 0);
}

void Emitter::EmitParticle(float currentTime)
{
	DirectX::XMFLOAT3 randPos = DirectX::XMFLOAT3();
	randPos.x = particlePositionRange.x * RandomRange(-1, 1);
	randPos.y = particlePositionRange.y * RandomRange(-1, 1);
	randPos.z = particlePositionRange.z * RandomRange(-1, 1);

	DirectX::XMFLOAT3 randVel = DirectX::XMFLOAT3();
	randVel.x = velocityRange.x * RandomRange(-1, 1);
	randVel.y = velocityRange.y * RandomRange(-1, 1);
	randVel.z = velocityRange.z * RandomRange(-1, 1);

	// Run a compute shader to emit a particle
	emitParticleCS->SetShader();
	emitParticleCS->SetFloat("currentTime", currentTime);
	emitParticleCS->SetFloat3("startPos", this->transform.GetPosition());
	emitParticleCS->SetFloat3("startVel", this->startVelocity);
	emitParticleCS->SetFloat3("randPos", randPos);
	emitParticleCS->SetFloat3("randVel", randVel);
	emitParticleCS->CopyBufferData(0);
	emitParticleCS->SetUnorderedAccessView("ParticleData", particleDataUAV);
	emitParticleCS->SetUnorderedAccessView("EmitterData", emitterDataUAV);
	emitParticleCS->DispatchByThreads(1, 1, 1);

	emitParticleCS->SetUnorderedAccessView("ParticleData", nullptr);
	emitParticleCS->SetUnorderedAccessView("EmitterData", nullptr);

	/*
	particles[deadIndex] = {};
	particles[deadIndex].EmitTime = currentTime;
	
	// Decide particle spawning information
	particles[deadIndex].StartPos = this->transform.GetPosition();
	particles[deadIndex].StartPos.x += particlePositionRange.x * RandomRange(-1, 1);
	particles[deadIndex].StartPos.y += particlePositionRange.y * RandomRange(-1, 1);
	particles[deadIndex].StartPos.z += particlePositionRange.z * RandomRange(-1, 1);

	particles[deadIndex].StartVelocity = startVelocity;
	particles[deadIndex].StartVelocity.x += velocityRange.x * RandomRange(-1, 1);
	particles[deadIndex].StartVelocity.y += velocityRange.y * RandomRange(-1, 1);
	particles[deadIndex].StartVelocity.z += velocityRange.z * RandomRange(-1, 1);

	// Increment emitter counters
	
	*/
}
