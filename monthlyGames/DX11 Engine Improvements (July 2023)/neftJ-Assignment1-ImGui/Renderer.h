#pragma once

#include "Lights.h"
#include "Sky.h"
#include "GameEntity.h"
#include "JustinGUI.h"
#include "Emitter.h"
#include "Structs.h"

#include <d3d11.h>
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>
using namespace DirectX;

enum RenderTargetType
{
	SCENE_COLORS,
	SCENE_AMBIENT,
	SCENE_NORMALS,
	SCENE_DEPTHS,
	SCENE_COMPOSITE,
	REFRACTION_SILHOUETTE,
	REFRACTION_COMPOSITE,
	FOREGROUND_OBJECTS,
	BACKGROUND_OBJECTS,
	FINAL_COMPOSITE,

	// This is last so we know how many types of MRTs we have.
	RENDER_TARGET_TYPE_COUNT
};

struct VSPerFrameData
{
	DirectX::XMFLOAT4X4 ViewMatrix;
	DirectX::XMFLOAT4X4 ProjectionMatrix;
};

struct PSPerFrameData
{
	Light Lights[MAX_LIGHTS];
	int LightCount;
	DirectX::XMFLOAT3 CameraPosition;
	int TotalSpecIBLMipLevels;
	DirectX::XMFLOAT3 AmbientNonPBR;
};

class Renderer
{
public:
	Renderer(
		Microsoft::WRL::ComPtr<ID3D11Device> device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV,
		unsigned int width,
		unsigned int height,
		std::shared_ptr<Sky> sky,
		const std::vector<std::shared_ptr<GameEntity>>& entities,
		std::vector<std::shared_ptr<Emitter>> emits,
		std::vector<Light>& lights,
		unsigned int lightCount,
		std::shared_ptr<JustinGUI> justinGUI,
		std::shared_ptr<Mesh> lightMesh,
		std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>>* vShaders,
		std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>>* pShaders,
		float refractionScale
	);
	~Renderer();

	void PreResize();
	void PostResize(
		unsigned int windowWidth,
		unsigned int windowHeight,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV
		);
	void Render(std::shared_ptr<Camera> camera, float deltaTime, float totalTime);
	
	std::vector<int*> GetRenderTypeCounts();
	std::vector<std::vector<std::shared_ptr<GameEntity>>> GetEntityGroups();
	std::vector<std::shared_ptr<Emitter>> GetEmitters();
	unsigned int* GetLightCount();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRenderTargetSRV(int i);
	unsigned int GetRenderTargetCount();
	std::shared_ptr<FocusParams> GetFocusParams();
	
private:
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV;
	unsigned int width;
	unsigned int height;
	std::shared_ptr<Sky> sky;
	const std::vector<std::shared_ptr<GameEntity>>& entities;
	std::vector<Light>& lights;
	unsigned int lightCount;
	std::shared_ptr<JustinGUI> justinGUI;

	std::vector<int*> renderTypeCounts;

	// Entity types
	std::vector<std::vector<std::shared_ptr<GameEntity>>> entityGroups;
	std::vector<std::shared_ptr<GameEntity>> refractiveEntities;
	std::vector<std::shared_ptr<GameEntity>> normalEntities;
	std::vector<std::shared_ptr<GameEntity>> transparentEntities;
	std::vector<std::shared_ptr<Emitter>> emitters;

	std::shared_ptr<Mesh> lightMesh;

	// Render Targets
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetRTVs[RenderTargetType::RENDER_TARGET_TYPE_COUNT];
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> renderTargetSRVs[RenderTargetType::RENDER_TARGET_TYPE_COUNT];

	// Refraction
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> refractionSilhouetteDepthState;
	float refractionScale;

	// Particles
	Microsoft::WRL::ComPtr<ID3D11BlendState> particleBlendAdditive;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> particleDepthState;

	// Depth of Field stuffs
	std::shared_ptr<FocusParams> focusParams;

	// Shader Dictionaries
	std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>>* vertexShaders;
	std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>>* pixelShaders;

	// Per frame datas
	Microsoft::WRL::ComPtr<ID3D11Buffer> psPerFrameCB;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vsPerFrameCB;
	PSPerFrameData psPerFrameData;
	VSPerFrameData vsPerFrameData;

	void DrawPointLights(std::shared_ptr<Camera> camera);
	void DrawEmitters(std::shared_ptr<Camera> camera, float currentTime);
	void CreateRenderTarget(unsigned int width, unsigned int height,
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv,
		DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM);
	void ResortEntityVectors();
};

