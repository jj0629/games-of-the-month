#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "GameEntity.h"
#include "Camera.h"
#include "SimpleShader.h"
#include "SpriteFont.h"
#include "SpriteBatch.h"
#include "Lights.h"
#include "Sky.h"
#include <d3d11.h>
#include "JustinGUI.h"
#include "Renderer.h"
#include "Emitter.h"

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Our scene
	std::vector<std::shared_ptr<GameEntity>> entities;
	int entityCount;
	std::vector<std::shared_ptr<Emitter>> emitters;
	int emitterCount;
	std::shared_ptr<Camera> camera;

	// Lights
	std::vector<Light> lights;
	int lightCount;

	// These will be loaded along with other assets and
	// saved to these variables for ease of access
	std::shared_ptr<Mesh> lightMesh;
	std::shared_ptr<SimpleVertexShader> lightVS;
	std::shared_ptr<SimplePixelShader> lightPS;

	// Text & ui
	std::shared_ptr<DirectX::SpriteFont> arial;
	std::shared_ptr<DirectX::SpriteBatch> spriteBatch;
	std::shared_ptr<JustinGUI> justinGUI;

	// Texture related resources
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> iblSamplerOptions;

	// Skybox
	std::shared_ptr<Sky> sky;

	// Rednerer object
	std::shared_ptr<Renderer> renderer;

	// Other shaders we may need for various stuffs
	std::unordered_map<std::string, std::shared_ptr<SimpleVertexShader>>* vertexShaders;
	std::unordered_map<std::string, std::shared_ptr<SimplePixelShader>>* pixelShaders;

	// General helpers for setup and drawing
	void CreateIBLMaterials(std::shared_ptr<Mesh> sphere, std::shared_ptr<SimplePixelShader> ps, std::shared_ptr<SimpleVertexShader> vs);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CreateSolidColorTexture(int width, int height, DirectX::XMFLOAT4 color);
	void CreateEmitters();
	void GenerateLights();

	// Initialization helper method
	void LoadAssetsAndCreateEntities();
};

