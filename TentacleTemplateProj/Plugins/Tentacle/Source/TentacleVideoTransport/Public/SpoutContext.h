// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "RHI.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include "SpoutDX/SpoutDX.h"
#include "Windows/HideWindowsPlatformTypes.h"
// HideWindowsPlatformTypes.h does not undef these — leak via Windows.h breaks GenericWindow.h
#undef IsMaximized
#undef IsMinimized

/**
 * Context for a Spout sender or receiver.
 */
struct FSpoutContext
{
	struct ID3D11DeviceContext*
	DeviceContext = nullptr;

	struct ID3D11Device*
	D3D11Device = nullptr;

	struct ID3D11On12Device*
	D3D11on12Device = nullptr;

	spoutDX
	Spout;

public:
	FSpoutContext();
	~FSpoutContext();

private:
	void
	InitSpout();

public:
	void
	DisposeSpout();

	ID3D11Texture2D*
	GetD3D11Texture(const FTextureRHIRef& InTexture) const;

	static EPixelFormat
	ToEPixelFormat(DXGI_FORMAT DXGIFormat);

	static DXGI_FORMAT
	ToDXGIFormat(EPixelFormat Format);
};
