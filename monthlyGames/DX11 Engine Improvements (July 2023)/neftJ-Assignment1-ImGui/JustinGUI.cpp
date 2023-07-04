#include "JustinGUI.h"

JustinGUI::JustinGUI(HWND hWnd, Microsoft::WRL::ComPtr<ID3D11Device> device, Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, unsigned int width, unsigned int height)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(device.Get(), context.Get());

	this->width = width;
	this->height = height;
}

JustinGUI::~JustinGUI()
{
}

void JustinGUI::Update(float deltaTime, float totalTime, unsigned int width, unsigned int height,
	std::vector<Light> lights, int* lightCount,
	std::vector<std::vector<std::shared_ptr<GameEntity>>> eGroups, std::vector<std::shared_ptr<Emitter>> emits,
	std::vector<int*> renderTypeNums,
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> srvs, std::shared_ptr<FocusParams> fp, std::shared_ptr<ChromaticAberrationParams> ca, 
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> irradianceMapSRV)
{
	// Update internal references to the scene
	this->lights = lights;
	this->lightCount = lightCount;
	this->entityGroups = eGroups;
	this->emitters = emits;
	this->renderTypeCounts = renderTypeNums;
	this->renderTargetSRVs = srvs;
	this->focusParams = fp;
	this->chromAbbParams = ca;
	this->irradianceMapSRV = irradianceMapSRV;
	this->width = width;
	this->height = height;

	// Update ImGui properties
	Input& input = Input::GetInstance();

	input.SetGuiKeyboardCapture(false);
	input.SetGuiMouseCapture(false);

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = deltaTime;
	io.DisplaySize.x = (float)width;
	io.DisplaySize.y = (float)height;
	io.KeyCtrl = input.KeyDown(VK_CONTROL);
	io.KeyShift = input.KeyDown(VK_SHIFT);
	io.KeyAlt = input.KeyDown(VK_MENU);
	io.MousePos.x = (float)input.GetMouseX();
	io.MousePos.y = (float)input.GetMouseY();
	io.MouseDown[0] = input.MouseLeftDown();
	io.MouseDown[1] = input.MouseRightDown();
	io.MouseDown[2] = input.MouseMiddleDown();
	io.MouseWheel = input.GetMouseWheel();
	input.GetKeyArray(io.KeysDown, 256);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	input.SetGuiKeyboardCapture(io.WantCaptureKeyboard);
	input.SetGuiMouseCapture(io.WantCaptureMouse);

	CreateBaseTree(width, height);
}

void JustinGUI::Draw()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void JustinGUI::CreateBaseTree(unsigned int width, unsigned int height)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Begin("Justin's Window");

	if (ImGui::CollapsingHeader("App Info"))
	{
		ImGui::Text(("FPS: " + std::to_string(io.Framerate)).c_str());
		ImGui::Text(("Width: " + std::to_string(width) + " Height: " + std::to_string(height)).c_str());
		ImGui::Text(("Aspect Ratio: " + std::to_string((float)width / (float)height)).c_str());
	}

	if (ImGui::CollapsingHeader("Controls"))
	{
		ImGui::Text("(WASD, X, Space) Move camera");
		ImGui::Text("(Left Click & Drag) Rotate camera");
		ImGui::Text("(Left Shift) Hold to speed up camera");
		ImGui::Text("(Left Ctrl) Hold to slow down camera");
		ImGui::Text("(TAB) Randomize lights");
	}

	if (ImGui::CollapsingHeader("Scene Entities"))
	{
		UpdateLights();
		UpdateGameObjects();
		UpdateEmitters();
	}

	if (ImGui::CollapsingHeader("MRT Images"))
	{
		ImVec2 imageSize = ImGui::GetItemRectSize();
		float imageHeight = imageSize.x * ((float)height / width);
		for (auto& tex : renderTargetSRVs)
		{
			ImTextureID imTex = tex.Get();
			ImGui::Image(imTex, ImVec2(imageSize.x, imageHeight));
		}
	}

	if (ImGui::CollapsingHeader("IrradianceMap"))
	{
		ImVec2 imageSize = ImGui::GetItemRectSize();
		float imageHeight = imageSize.x * ((float)height / width);
		ImTextureID imTex = irradianceMapSRV.Get();
		ImGui::Image(imTex, ImVec2(imageSize.x, imageHeight));
	}

	if (ImGui::CollapsingHeader("Depth of Field Parameters"))
	{
		ImGui::DragFloat("Near Focus", &focusParams->nearFocus, 0.001f, 0, 1);
		ImGui::DragFloat("Far Focus", &focusParams->farFocus, 0.001f, 0, 1);
		ImGui::DragFloat2("Focus Center", &focusParams->focusCenter.x, 0.001f, 0, 1);
		ImGui::DragFloat("Focus Intensity", &focusParams->focusIntensity, 0.1f, 0, 100);
	}

	if (ImGui::CollapsingHeader("Chromatic Aberration Parameters"))
	{
		ImGui::DragFloat2("Direction", &chromAbbParams->direction.x, 0.001f, -1, 1);
		ImGui::DragFloat("Color Split Difference", &chromAbbParams->colorSplitDiff, 0.001f, 0, 1);
	}

	ImGui::End();
}

/// <summary>
/// Updates the ImGui with all the lights 
/// </summary>
void JustinGUI::UpdateLights()
{
	if (ImGui::TreeNode("Lights"))
	{
		ImGui::SliderInt("Active Lights: ", lightCount, 0, lights.size());

		if (ImGui::TreeNode("Active Lights"))
		{
			// Display info on each light.
			for (int i = 0; i < *lightCount; i++)
			{
				CreateSingleLight(&lights[i], i);
			}
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}
}

/// <summary>
/// Creates a single light tree node so the user can edit each individual light in ImGui
/// </summary>
/// <param name="light">The current light being processed</param>
/// <param name="lightNum">The index number of the current light</param>
void JustinGUI::CreateSingleLight(Light* light, int lightNum)
{
	if (ImGui::TreeNode(("Light " + std::to_string(lightNum)).c_str()))
	{
		switch (light->Type)
		{
		case 0:
			ImGui::Text("Type: Directional");
			ImGui::DragFloat3("Direction: ", &light->Direction.x);
			ImGui::DragFloat("Intensity: ", &light->Intensity, 0, 10);
			break;
		case 1:
			ImGui::Text("Type: Point");
			ImGui::DragFloat3("Position: ", &light->Position.x);
			ImGui::DragFloat("Range: ", &light->Range, 0, 100);
			ImGui::DragFloat("Intensity: ", &light->Intensity, 0, 10);
			break;
		case 2:
			ImGui::Text("Type: Spot");
			ImGui::DragFloat3("Direction: ", &light->Direction.x);
			ImGui::DragFloat3("Position: ", &light->Position.x);
			ImGui::DragFloat("Range: ", &light->Range, 0, 100);
			ImGui::DragFloat("Intensity: ", &light->Intensity, 0, 10);
			ImGui::DragFloat("Falloff: ", &light->SpotFalloff, 0, 1);
			break;
		}
		ImGui::DragFloat3("Color:", &light->Color.x);

		ImGui::TreePop();
	}
}

void JustinGUI::UpdateGameObjects()
{
	if (ImGui::TreeNode("Game Objects"))
	{
		// First, sort out the game objects by type so we can properly render them.
		ImGui::Text("Active Game Objects");

		if (ImGui::TreeNode("Standard Game Objects"))
		{
			ImGui::SliderInt(" ", renderTypeCounts[RenderObjectType::STANDARD], 0, entityGroups[RenderObjectType::STANDARD].size());
			for (int i = 0; i < *renderTypeCounts[RenderObjectType::STANDARD]; i++)
			{
				std::shared_ptr<GameEntity> go = entityGroups[RenderObjectType::STANDARD][i];
				CreateSingleGameObject(go, i);
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Refractive Game Objects"))
		{
			ImGui::SliderInt(" ", renderTypeCounts[RenderObjectType::REFRACTIVE], 0, entityGroups[RenderObjectType::REFRACTIVE].size());
			for (int i = 0; i < *renderTypeCounts[RenderObjectType::REFRACTIVE]; i++)
			{
				std::shared_ptr<GameEntity> go = entityGroups[RenderObjectType::REFRACTIVE][i];
				CreateSingleGameObject(go, i);
			}

			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Transparent Game Objects"))
		{
			ImGui::SliderInt(" ", renderTypeCounts[RenderObjectType::SEETHROUGH], 0, entityGroups[RenderObjectType::SEETHROUGH].size());
			for (int i = 0; i < *renderTypeCounts[RenderObjectType::SEETHROUGH]; i++)
			{
				std::shared_ptr<GameEntity> go = entityGroups[RenderObjectType::SEETHROUGH][i];
				CreateSingleGameObject(go, i);
			}

			ImGui::TreePop();
		}

		ImGui::TreePop();
	}
}

void JustinGUI::CreateSingleGameObject(std::shared_ptr<GameEntity> go, int goNum)
{
	if (ImGui::TreeNode(("Game Object " + std::to_string(goNum)).c_str()))
	{
		DirectX::XMFLOAT3 goPos = go->GetTransform()->GetPosition();
		ImGui::Text("Position");
		if (ImGui::DragFloat3("", &goPos.x))
		{
			go->GetTransform()->SetPosition(goPos.x, goPos.y, goPos.z);
		}

		CreateSingleMaterial(go->GetMaterial());

		ImGui::TreePop();
	}
}

void JustinGUI::CreateSingleMaterial(std::shared_ptr<Material> mat)
{
	if (ImGui::TreeNode("Material"))
	{
		ImGui::Text("Color Tint");
		DirectX::XMFLOAT3 color = mat->GetColorTint();
		if (ImGui::ColorEdit3("", &color.x))
		{
			mat->SetColorTint(color);
		}

		for (const auto& m : mat->GetAllTextureSRVs())
		{
			ImVec2 imageSize = ImGui::GetItemRectSize();
			float imageHeight = imageSize.x * ((float)height / width);
			ImTextureID newTex = ImTextureID(m.second.Get());
			ImGui::Image(newTex, ImVec2(imageSize.x, imageHeight));
		}

		ImGui::TreePop();
	}
}

void JustinGUI::UpdateEmitters()
{
	if (ImGui::TreeNode("Particle Emitters"))
	{
		ImGui::Text("Emitters");
		ImGui::SliderInt(" ", renderTypeCounts[RenderObjectType::EMITTER], 0, emitters.size());

		for (int i = 0; i < *renderTypeCounts[RenderObjectType::EMITTER]; i++)
		{
			std::shared_ptr<Emitter> e = emitters[i];
			CreateSingleEmitter(e, i);
		}

		ImGui::TreePop();
	}
}

void JustinGUI::CreateSingleEmitter(std::shared_ptr<Emitter> e, int eNum)
{
	if (ImGui::TreeNode(("Emitter " + std::to_string(eNum)).c_str()))
	{
		DirectX::XMFLOAT3 ePos = e->GetTransform()->GetPosition();
		if (ImGui::DragFloat3("Emitter Position", &ePos.x))
		{
			e->GetTransform()->SetPosition(ePos.x, ePos.y, ePos.z);
		}

		DirectX::XMFLOAT3 eAccel = e->GetAcceleration();
		if (ImGui::DragFloat3("Acceleration", &eAccel.x))
		{
			e->SetAcceleration(eAccel);
		}

		DirectX::XMFLOAT3 eSpRg = e->GetSpawnRange();
		if (ImGui::DragFloat3("Spawn Range", &eSpRg.x))
		{
			e->SetSpawnRange(eSpRg);
		}

		DirectX::XMFLOAT3 eStartVel = e->GetStartVelocity();
		if (ImGui::DragFloat3("Start Velocity", &eStartVel.x))
		{
			e->SetStartVelocity(eStartVel);
		}

		DirectX::XMFLOAT3 eVelRg = e->GetVelocityRange();
		if (ImGui::DragFloat3("Velocity Range", &eVelRg.x))
		{
			e->SetVelocityRange(eVelRg);
		}

		CreateSingleMaterial(e->GetMaterial());

		ImGui::TreePop();
	}
}
