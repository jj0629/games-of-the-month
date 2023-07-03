#pragma once

// Engine includes
#include "Lights.h"
#include "GameEntity.h"
#include "Input.h"
#include <DirectXMath.h>
#include "Emitter.h"
#include "Structs.h"

// ImGUI includes
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

enum RenderObjectType
{
	STANDARD,
	REFRACTIVE,
	SEETHROUGH,
	EMITTER,

	RENDER_OBJECT_TYPE_COUNT
};

class JustinGUI
{
public:
	JustinGUI(HWND hWnd, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, unsigned int width, unsigned int height);
	~JustinGUI();

	void Update(float deltaTime, float totalTime, unsigned int width, unsigned int height,
		std::vector<Light> lights, int* lightCount,
		std::vector<std::vector<std::shared_ptr<GameEntity>>> eGroups, std::vector<std::shared_ptr<Emitter>> emits, std::vector<int*> renderTypeNums,
		std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> srvs, std::shared_ptr<FocusParams> fp,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceMapSRV);
	void Draw();

private:
	std::vector<Light> lights;
	int* lightCount;
	std::vector<std::vector<std::shared_ptr<GameEntity>>> entityGroups;
	std::vector<std::shared_ptr<Emitter>> emitters;
	std::vector<int*> renderTypeCounts;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> renderTargetSRVs;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceMapSRV;
	std::shared_ptr<FocusParams> focusParams;
	unsigned int width;
	unsigned int height;

	void CreateBaseTree(unsigned int width, unsigned int height);
	void UpdateLights();
	void CreateSingleLight(Light* light, int lightNum);
	void UpdateGameObjects();
	void CreateSingleGameObject(std::shared_ptr<GameEntity> go, int goNum);
	void CreateSingleMaterial(std::shared_ptr<Material> mat);
	void UpdateEmitters();
	void CreateSingleEmitter(std::shared_ptr<Emitter> e, int eNum);
};

