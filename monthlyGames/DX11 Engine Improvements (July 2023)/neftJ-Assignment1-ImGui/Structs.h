#pragma once

#include <DirectXMath.h>

// Struct for defining depth of field parameters
struct FocusParams
{
	float nearFocus;
	float farFocus;
	DirectX::XMFLOAT2 focusCenter;
	float focusIntensity;
};

struct ChromaticAberrationParams
{
	DirectX::XMFLOAT2 direction;
	DirectX::XMFLOAT2 offset;
	float colorSplitDiff;
};