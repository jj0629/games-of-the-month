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
	std::shared_ptr<Material> mat) :
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

	transform = Transform();

	// Create buffer and srv
	D3D11_BUFFER_DESC desc = {};
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.Usage = D3D11_USAGE_DYNAMIC;
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

	// Create indices
	unsigned int* indices = new unsigned int[maxParticles * 6];
	int indexCount = 0;
	for (int i = 0; i < maxParticles * 4; i+=4)
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

Emitter::~Emitter()
{
	delete[] particles;
}

void Emitter::Update(float currentTime, float deltaTime)
{
	// First, loop through all live particles and check for age.

	if (aliveParticleCount > 0)
	{
		// The case where we don't need to loop through the array
		if (livingIndex < deadIndex)
		{
			for (int i = livingIndex; i < deadIndex; i++)
			{
				if (currentTime - particles[i].EmitTime > particleMaxAge)
				{
					livingIndex++;
					livingIndex = livingIndex % maxParticles;
					aliveParticleCount--;
				}
			}
		}
		else
		{
			// First, go from 0 to the deadIndex
			for (int i = 0; i < deadIndex; i++)
			{
				if (currentTime - particles[i].EmitTime > particleMaxAge)
				{
					livingIndex++;
					livingIndex = livingIndex % maxParticles;
					aliveParticleCount--;
				}
			}

			// Next, go from the livingIndex to the end of the array
			for (int i = livingIndex; i < maxParticles; i++)
			{
				if (currentTime - particles[i].EmitTime > particleMaxAge)
				{
					livingIndex++;
					livingIndex = livingIndex % maxParticles;
					aliveParticleCount--;
				}
			}
		}
	}
	
	// Next, see if we need to emit a new particle
	timeSinceLastEmission += deltaTime;
	while (timeSinceLastEmission > secondsPerEmission)
	{
		EmitParticle(currentTime);
		timeSinceLastEmission -= secondsPerEmission;
	}

	CopyBufferToGPU();
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
	vs->SetFloat("currentTime", currentTime);
	vs->SetFloat3("acceleration", DirectX::XMFLOAT3(0, -3.0f, 0));

	// Finally make the draw call
	context->DrawIndexed(aliveParticleCount * 6, 0, 0);
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

void Emitter::CopyBufferToGPU()
{
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	context->Map(particleDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	
	if (livingIndex < deadIndex)
	{
		// Copy all particles from livingIndex to deadIndex
		memcpy(mapped.pData, particles + livingIndex, sizeof(Particle) * aliveParticleCount);
	}
	else
	{
		// First copy all the particles from 0 to the deadIndex
		memcpy(mapped.pData, particles, sizeof(Particle) * aliveParticleCount);

		// Next, copy all the particles from livingIndex to the end of the array
		memcpy((void*)((Particle*)mapped.pData + deadIndex), particles + livingIndex, sizeof(Particle) * (maxParticles - livingIndex));
	}

	context->Unmap(particleDataBuffer.Get(), 0);
}

void Emitter::EmitParticle(float currentTime)
{
	if (aliveParticleCount >= maxParticles)
	{
		return;
	}

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
	deadIndex++;
	deadIndex %= maxParticles;
	aliveParticleCount++;
}
