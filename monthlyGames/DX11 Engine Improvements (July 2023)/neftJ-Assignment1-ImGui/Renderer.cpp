#include "Renderer.h"

Renderer::Renderer(
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
	float refractionScale) :
	entities(entities),
	emitters(emits),
	lights(lights),
	depthBufferDSV(depthBufferDSV)
{
	this->device = device;
	this->context = context;
	this->swapChain = swapChain;
	this->backBufferRTV = backBufferRTV;
	this->width = width;
	this->height = height;
	this->sky = sky;
	this->lightCount = lightCount;
	this->justinGUI = justinGUI;
	this->lightMesh = lightMesh;
	this->vertexShaders = vShaders;
	this->pixelShaders = pShaders;
	this->refractionScale = refractionScale;

	// Set up rendering pointers that can be shared with the gui for selecting how many of each object type to render.
	for (int i = 0; i < RenderObjectType::RENDER_OBJECT_TYPE_COUNT; i++)
	{
		renderTypeCounts.push_back(new int(0));
	}

	// Set focus params
	focusParams = std::make_shared<FocusParams>();
	focusParams.get()->farFocus = 0.7f;
	focusParams.get()->nearFocus = 0.1f;
	focusParams.get()->focusCenter = DirectX::XMFLOAT2(0, 0);
	focusParams.get()->focusIntensity = 10;

	// Set chromatic aberration params
	chromAbbParams = std::make_shared<ChromaticAberrationParams>();
	chromAbbParams.get()->direction = DirectX::XMFLOAT2(0.003f, 0.002f);
	chromAbbParams.get()->offset = DirectX::XMFLOAT2(0.002, 0.007f);
	chromAbbParams.get()->colorSplitDiff = 0.006f;

	ResortEntityVectors();

	entityGroups.push_back(normalEntities);
	entityGroups.push_back(refractiveEntities);
	entityGroups.push_back(transparentEntities);

	PostResize(width, height, backBufferRTV, depthBufferDSV);

	D3D11_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable = true;
	depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&depthDesc, refractionSilhouetteDepthState.GetAddressOf());

	// Create particle blend state and depth stencil
	D3D11_BLEND_DESC partBlend = {};
	partBlend.AlphaToCoverageEnable = false;
	partBlend.IndependentBlendEnable = false;
	partBlend.RenderTarget[0].BlendEnable = true;
	partBlend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	partBlend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	partBlend.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	partBlend.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	partBlend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	partBlend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	partBlend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&partBlend, particleBlendAdditive.GetAddressOf());

	D3D11_DEPTH_STENCIL_DESC particleDepthDesc = {};
	particleDepthDesc.DepthEnable = true; // READ from depth buffer
	particleDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No depth WRITING
	particleDepthDesc.DepthFunc = D3D11_COMPARISON_LESS; // Standard depth comparison
	device->CreateDepthStencilState(&particleDepthDesc, particleDepthState.GetAddressOf());
}

	Renderer::~Renderer()
	{
		for (auto& count : renderTypeCounts)
		{
			delete count;
		}
	}

void Renderer::PreResize()
{
	this->backBufferRTV.Get()->Release();
	this->depthBufferDSV.Get()->Release();
}

void Renderer::PostResize(
	unsigned int windowWidth, 
	unsigned int windowHeight, 
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV, 
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV)
{
	this->width = windowWidth;
	this->height = windowHeight;
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;

	for (auto& rt : renderTargetRTVs) rt.Reset();
	for (auto& rt : renderTargetSRVs) rt.Reset();

	// Remake our mrts if the window resizes so they're the correct size
	for (int i = 0; i < RenderTargetType::RENDER_TARGET_TYPE_COUNT; i++)
	{
		if (i == RenderTargetType::SCENE_DEPTHS)
		{
			CreateRenderTarget(width, height, renderTargetRTVs[i], renderTargetSRVs[i], DXGI_FORMAT_R32_FLOAT);
		}
		else if (i == RenderTargetType::REFRACTION_SILHOUETTE)
		{
			CreateRenderTarget(width, height, renderTargetRTVs[i], renderTargetSRVs[i], DXGI_FORMAT_R8_UNORM);
		}
		else
		{
			CreateRenderTarget(width, height, renderTargetRTVs[i], renderTargetSRVs[i]);
		}
	}
}

void Renderer::Render(std::shared_ptr<Camera> camera, float deltaTime, float totalTime)
{
	// Background color for clearing
	const float color[4] = { 0, 0, 0, 1 };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	// Clear MRTs
	for (auto& rt : renderTargetRTVs)
	{
		context->ClearRenderTargetView(rt.Get(), color);
	}
	const float depth[4] = { 1, 0, 0, 0 };
	context->ClearRenderTargetView(renderTargetRTVs[RenderTargetType::SCENE_DEPTHS].Get(), depth);

	// Setup render targets
	const int numTargets = 4;
	ID3D11RenderTargetView* targets[numTargets] = {};
	targets[0] = renderTargetRTVs[RenderTargetType::SCENE_COLORS].Get();
	targets[1] = renderTargetRTVs[RenderTargetType::SCENE_AMBIENT].Get();
	targets[2] = renderTargetRTVs[RenderTargetType::SCENE_NORMALS].Get();
	targets[3] = renderTargetRTVs[RenderTargetType::SCENE_DEPTHS].Get();
	context->OMSetRenderTargets(numTargets, targets, depthBufferDSV.Get());

	// First, draw the normal entities.
	for (int i = 0; i < *renderTypeCounts[RenderObjectType::STANDARD]; i++)
	{
		std::shared_ptr<GameEntity> ge = normalEntities[i];
		// Set the "per frame" data
		// Note that this should literally be set once PER FRAME, before
		// the draw loop, but we're currently setting it per entity since 
		// we are just using whichever shader the current entity has.  
		// Inefficient!!!
		std::shared_ptr<SimplePixelShader> ps = ge->GetMaterial()->GetPixelShader();
		ps->SetData("lights", (void*)(&lights[0]), sizeof(Light) * lightCount);
		ps->SetInt("lightCount", lightCount);
		ps->SetFloat3("cameraPosition", camera->GetTransform()->GetPosition());
		ps->SetInt("SpecIBLTotalMipLevels", sky->GetConvolvedSpecularMipLevels());
		ps->CopyBufferData("perFrame");

		// Set IBL textures
		ps->SetShaderResourceView("BrdfLookUpMap", sky->GetBRDFLookupTable());
		ps->SetShaderResourceView("IrradianceIBLMap", sky->GetIrradianceMap());
		ps->SetShaderResourceView("SpecularIBLMap", sky->GetConvolvedSpecularMap());

		// Draw the entity
		ge->Draw(context, camera);
	}

	// Draw the sky
	sky->Draw(camera);

	std::shared_ptr<SimpleVertexShader> fullscreenVS = vertexShaders->at("fullscreenVS");
	fullscreenVS->SetShader();

	// Do our scene composite here.
	targets[0] = renderTargetRTVs->GetAddressOf()[RenderTargetType::SCENE_COMPOSITE];
	context->OMSetRenderTargets(1, targets, 0);
	std::shared_ptr<SimplePixelShader> mrtCompositePS = pixelShaders->at("mrtCompositePS");
	mrtCompositePS->SetShader();
	mrtCompositePS->SetShaderResourceView("sceneColors", renderTargetSRVs[RenderTargetType::SCENE_COLORS]);
	mrtCompositePS->SetShaderResourceView("ambient", renderTargetSRVs[RenderTargetType::SCENE_AMBIENT]);
	context->Draw(3, 0);

	targets[0] = renderTargetRTVs->GetAddressOf()[RenderTargetType::REFRACTION_COMPOSITE];
	context->OMSetRenderTargets(1, targets, 0);
	mrtCompositePS->SetShader();
	mrtCompositePS->SetShaderResourceView("sceneColors", renderTargetSRVs[RenderTargetType::SCENE_COLORS]);
	mrtCompositePS->SetShaderResourceView("ambient", renderTargetSRVs[RenderTargetType::SCENE_AMBIENT]);
	context->Draw(3, 0);

	targets[0] = renderTargetRTVs[RenderTargetType::REFRACTION_SILHOUETTE].Get();
	context->OMSetRenderTargets(1, targets, depthBufferDSV.Get());

	// Depth state
	context->OMSetDepthStencilState(refractionSilhouetteDepthState.Get(), 0);

	std::shared_ptr<SimplePixelShader> solidColorPS = pixelShaders->at("solidColorPS");

	// Loop and draw each one
	for (int i = 0; i < *renderTypeCounts[RenderObjectType::REFRACTIVE]; i++)
	{
		std::shared_ptr<GameEntity> ge = refractiveEntities[i];

		// Get this material and sub the refraction PS for now
		std::shared_ptr<Material> mat = ge->GetMaterial();
		std::shared_ptr<SimplePixelShader> prevPS = mat->GetPixelShader();
		mat->SetPixelShader(solidColorPS);

		// Overall material prep
		mat->PrepareMaterial(ge->GetTransform(), camera);

		// Set up the refraction specific data
		solidColorPS->SetFloat3("Color", XMFLOAT3(1, 1, 1));
		solidColorPS->CopyBufferData("externalData");

		// Draw
		ge->GetMesh()->SetBuffersAndDraw(context);

		// Reset this material's PS
		mat->SetPixelShader(prevPS);
	}

	// Reset depth state
	context->OMSetDepthStencilState(0, 0);

	// Loop and draw refractive objects
	{
		// Set up pipeline for refractive draw
		// Same target (back buffer), but now we need the depth buffer again
		targets[0] = renderTargetRTVs[RenderTargetType::REFRACTION_COMPOSITE].Get();
		context->OMSetRenderTargets(1, targets, depthBufferDSV.Get());

		std::shared_ptr<SimplePixelShader> refractionPS = pixelShaders->at("refractionPS");

		// Loop and draw each one
		for (int i = 0; i < *renderTypeCounts[RenderObjectType::REFRACTIVE]; i++)
		{
			std::shared_ptr<GameEntity> ge = refractiveEntities[i];

			// Get this material and sub the refraction PS for now
			std::shared_ptr<Material> mat = ge->GetMaterial();
			std::shared_ptr<SimplePixelShader> prevPS = mat->GetPixelShader();
			mat->SetPixelShader(refractionPS);

			// Overall material prep
			mat->PrepareMaterial(ge->GetTransform(), camera);

			// Set up the refraction specific data
			refractionPS->SetMatrix4x4("viewMatrix", camera->GetView());
			refractionPS->SetMatrix4x4("projMatrix", camera->GetProjection());
			refractionPS->SetFloat2("screenSize", XMFLOAT2((float)width, (float)height));
			refractionPS->SetFloat("refractionScale", 0.1);
			refractionPS->SetFloat3("cameraPos", camera->GetTransform()->GetPosition());
			refractionPS->CopyBufferData("externalData");

			// Set textures
			refractionPS->SetShaderResourceView("screenPixels", renderTargetSRVs[RenderTargetType::SCENE_COMPOSITE].Get());
			refractionPS->SetShaderResourceView("refractionSilhouette", renderTargetSRVs[RenderTargetType::REFRACTION_SILHOUETTE].Get());
			refractionPS->SetShaderResourceView("environmentMap", sky->GetSkySRV());

			// Draw
			ge->GetMesh()->SetBuffersAndDraw(context);

			// Reset this material's PS
			mat->SetPixelShader(prevPS);
		}
	}

	// Draw the light sources
	DrawPointLights(camera);

	DrawEmitters(camera, totalTime);

	// Split objects outside the focus region into foreground and background mrts
	fullscreenVS->SetShader();
	targets[0] = renderTargetRTVs[RenderTargetType::FOREGROUND_OBJECTS].Get();
	targets[1] = renderTargetRTVs[RenderTargetType::BACKGROUND_OBJECTS].Get();
	context->OMSetRenderTargets(2, targets, 0);
	std::shared_ptr<SimplePixelShader> splitPS = pixelShaders->at("splitPS");
	splitPS->SetShader();
	splitPS->SetFloat("nearFocus", focusParams.get()->nearFocus);
	splitPS->SetFloat("farFocus", focusParams.get()->farFocus);
	splitPS->SetFloat2("focusCenter", focusParams.get()->focusCenter);
	splitPS->SetFloat("focusIntensity", focusParams.get()->focusIntensity);
	splitPS->CopyBufferData("FocusParams");
	splitPS->SetShaderResourceView("sceneColors", renderTargetSRVs[RenderTargetType::SCENE_COLORS]);
	splitPS->SetShaderResourceView("sceneDepths", renderTargetSRVs[RenderTargetType::SCENE_DEPTHS]);
	context->Draw(3, 0);

	// Save the REFRACTION_COMPOSITE mrt to the BLUR_COMPOSITE mrt.
	targets[0] = renderTargetRTVs->GetAddressOf()[RenderTargetType::BLUR_COMPOSITE];
	context->OMSetRenderTargets(1, targets, 0);
	std::shared_ptr<SimplePixelShader> simpleTexturePS = pixelShaders->at("simpleTexturePS");
	simpleTexturePS->SetShader();
	simpleTexturePS->SetShaderResourceView("pixels", renderTargetSRVs[RenderTargetType::REFRACTION_COMPOSITE]);
	context->Draw(3, 0);
	
	// Next, blur the two mrts
	std::shared_ptr<SimplePixelShader> blurPS = pixelShaders->at("blurPS");
	targets[0] = renderTargetRTVs[RenderTargetType::FINAL_COMPOSITE].Get();
	context->OMSetRenderTargets(1, targets, 0);
	blurPS->SetShader();
	blurPS->SetData("texWidth", &this->width, sizeof(unsigned int));
	blurPS->SetData("texHeight", &this->height, sizeof(unsigned int));
	blurPS->CopyBufferData("externalData");
	blurPS->SetShaderResourceView("texToBlur", renderTargetSRVs[RenderTargetType::REFRACTION_COMPOSITE]);
	blurPS->SetShaderResourceView("blurMask", renderTargetSRVs[RenderTargetType::BACKGROUND_OBJECTS]);
	context->Draw(3, 0);

	blurPS->SetInt("texWidth", this->width);
	blurPS->SetInt("texHeight", this->height);
	blurPS->CopyBufferData("externalData");
	blurPS->SetShaderResourceView("texToBlur", renderTargetSRVs[RenderTargetType::REFRACTION_COMPOSITE]);
	blurPS->SetShaderResourceView("blurMask", renderTargetSRVs[RenderTargetType::FOREGROUND_OBJECTS]);
	context->Draw(3, 0);

	// Next do our chromatic aberration pass on the whole screen
	std::shared_ptr<SimplePixelShader> chromAbbPS = pixelShaders->at("chromAbbPS");
	targets[0] = renderTargetRTVs->GetAddressOf()[RenderTargetType::FINAL_COMPOSITE];
	context->OMSetRenderTargets(1, targets, 0);
	chromAbbPS->SetShader();
	
	// Set chromatic aberration params
	chromAbbPS->SetFloat2("direction", chromAbbParams.get()->direction);
	chromAbbPS->SetFloat2("offset", chromAbbParams.get()->offset);
	chromAbbPS->SetFloat("colorSplitDiff", chromAbbParams.get()->colorSplitDiff);
	chromAbbPS->CopyBufferData("chromAbbData");
	chromAbbPS->SetShaderResourceView("sceneColors", renderTargetSRVs[RenderTargetType::BLUR_COMPOSITE]);
	context->Draw(3, 0);

	// This block takes our final composite and puts it on the back buffer.
	targets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, targets, 0);
	simpleTexturePS->SetShader();
	simpleTexturePS->SetShaderResourceView("pixels", renderTargetSRVs[RenderTargetType::FINAL_COMPOSITE]);
	context->Draw(3, 0);

	// Draw ImGui
	targets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, targets, depthBufferDSV.Get());
	justinGUI->Draw();


	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);
}

std::vector<int*> Renderer::GetRenderTypeCounts()
{
	return renderTypeCounts;
}

std::vector<std::vector<std::shared_ptr<GameEntity>>> Renderer::GetEntityGroups()
{
	return entityGroups;
}

std::vector<std::shared_ptr<Emitter>> Renderer::GetEmitters()
{
	return emitters;
}

unsigned int* Renderer::GetLightCount()
{
	return &lightCount;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetRenderTargetSRV(int i)
{
	return renderTargetSRVs[i];
}

unsigned int Renderer::GetRenderTargetCount()
{
	return RenderTargetType::RENDER_TARGET_TYPE_COUNT;
}

std::shared_ptr<FocusParams> Renderer::GetFocusParams()
{
	return this->focusParams;
}

std::shared_ptr<ChromaticAberrationParams> Renderer::GetChromAbbParams()
{
	return this->chromAbbParams;
}

void Renderer::DrawPointLights(std::shared_ptr<Camera> camera)
{
	// Turn on these shaders
	std::shared_ptr<SimpleVertexShader> lightVS = vertexShaders->at("basicVS");
	std::shared_ptr<SimplePixelShader> lightPS = pixelShaders->at("solidColorPS");
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", camera->GetView());
	lightVS->SetMatrix4x4("projection", camera->GetProjection());

	for (int i = 0; i < lightCount; i++)
	{
		Light light = lights[0];

		// Only drawing points, so skip others
		if (light.Type != LIGHT_TYPE_POINT)
			continue;

		// Calc quick scale based on range
		float scale = light.Range / 20.0f;

		// Make the transform for this light
		XMMATRIX rotMat = XMMatrixIdentity();
		XMMATRIX scaleMat = XMMatrixScaling(scale, scale, scale);
		XMMATRIX transMat = XMMatrixTranslation(light.Position.x, light.Position.y, light.Position.z);
		XMMATRIX worldMat = scaleMat * rotMat * transMat;

		XMFLOAT4X4 world;
		XMFLOAT4X4 worldInvTrans;
		XMStoreFloat4x4(&world, worldMat);
		XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		XMFLOAT3 finalColor = light.Color;
		finalColor.x *= light.Intensity;
		finalColor.y *= light.Intensity;
		finalColor.z *= light.Intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		lightMesh->SetBuffersAndDraw(context);
	}
}

void Renderer::DrawEmitters(std::shared_ptr<Camera> camera, float currentTime)
{
	// Set resources
	context->OMSetBlendState(particleBlendAdditive.Get(), 0, 0xffffffff);
	context->OMSetDepthStencilState(particleDepthState.Get(), 0);

	// Draw emitters
	for (int i = 0; i < *renderTypeCounts[RenderObjectType::EMITTER]; i++) {
		emitters[i]->Draw(camera, currentTime);
	}

	// Reset everything
	context->OMSetBlendState(0, 0, 0xffffffff);
	context->OMSetDepthStencilState(0, 0);
}

void Renderer::CreateRenderTarget(unsigned int width, unsigned int height, Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, DXGI_FORMAT colorFormat)
{
	Microsoft::WRL::ComPtr<ID3D11Texture2D> rtTexture;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = colorFormat;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&texDesc, 0, rtTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC rDesc = {};
	rDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rDesc.Texture2D.MipSlice = 0;
	rDesc.Format = texDesc.Format;
	device->CreateRenderTargetView(rtTexture.Get(), &rDesc, rtv.GetAddressOf());

	device->CreateShaderResourceView(rtTexture.Get(), 0, srv.GetAddressOf());
}

void Renderer::ResortEntityVectors()
{
	normalEntities.clear();
	transparentEntities.clear();
	refractiveEntities.clear();

	// Quickly go through the game entities and sort out refractive/non-refractive objects
	for (auto& ge : entities)
	{
		if (ge->GetMaterial()->IsRefractive())
		{
			refractiveEntities.push_back(ge);
		}
		else if (ge->GetMaterial()->IsTransparent())
		{
			transparentEntities.push_back(ge);
		}
		else {
			normalEntities.push_back(ge);
		}
	}

	*renderTypeCounts[RenderObjectType::STANDARD] = normalEntities.size();
	*renderTypeCounts[RenderObjectType::REFRACTIVE] = refractiveEntities.size();
	*renderTypeCounts[RenderObjectType::SEETHROUGH] = transparentEntities.size();
	*renderTypeCounts[RenderObjectType::EMITTER] = emitters.size();
}
