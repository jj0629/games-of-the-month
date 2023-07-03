#pragma once

#include <memory>

#include "Mesh.h"
#include "SimpleShader.h"
#include "Camera.h"

#include <wrl/client.h> // Used for ComPtr

class Sky
{
public:

	// Constructor that loads a DDS cube map file
	Sky(
		const wchar_t* cubemapDDSFile, 
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 	
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> irradiancePS,
		std::shared_ptr<SimplePixelShader> convolvedSpecularPS,
		std::shared_ptr<SimplePixelShader> brdfTablePS
	);

	// Constructor that loads 6 textures and makes a cube map
	Sky(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back,
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<SimpleVertexShader> skyVS,
		std::shared_ptr<SimplePixelShader> skyPS,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions,
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		std::shared_ptr<SimpleVertexShader> fullscreenVS,
		std::shared_ptr<SimplePixelShader> irradiancePS,
		std::shared_ptr<SimplePixelShader> convolvedSpecularPS,
		std::shared_ptr<SimplePixelShader> brdfTablePS
	);

	~Sky();

	void Draw(std::shared_ptr<Camera> camera);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSkySRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetIrradianceMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetConvolvedSpecularMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBRDFLookupTable();
	int GetConvolvedSpecularMipLevels();

private:

	void InitRenderStates();

	void IBLCreateIrradianceMap(std::shared_ptr<SimpleVertexShader> fullscreenVS, std::shared_ptr<SimplePixelShader> irradiancePS);
	void IBLCreateConvolvedSpecularMap(std::shared_ptr<SimpleVertexShader> fullscreenVS, std::shared_ptr<SimplePixelShader> convolvedSpecularPS);
	void IBLCreateBRDFLookupTable(std::shared_ptr<SimpleVertexShader> fullscreenVS, std::shared_ptr<SimplePixelShader> brdfTablePS);

	// Helper for creating a cubemap from 6 individual textures
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateCubemap(
		const wchar_t* right,
		const wchar_t* left,
		const wchar_t* up,
		const wchar_t* down,
		const wchar_t* front,
		const wchar_t* back);

	// Skybox related resources
	std::shared_ptr<SimpleVertexShader> skyVS;
	std::shared_ptr<SimplePixelShader> skyPS;
	
	std::shared_ptr<Mesh> skyMesh;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyRasterState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyDepthState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skySRV;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11Device> device;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceCubeMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> convolvedSpecularMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> brdfLookupTable;

	int mipLevels;
	const int mipSkipLevels = 3;
	const int cubeMapSize = 512;
	const int textureSize = 512;
};

