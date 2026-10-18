// © 2025 Eldritch One. All rights reserved.

#include "SpoutContext.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d11on12.h>
#include "Windows/HideWindowsPlatformTypes.h"

FSpoutContext::FSpoutContext()
{
	InitSpout();
}

FSpoutContext::~FSpoutContext()
{
	DisposeSpout();
}

void
FSpoutContext::InitSpout()
{
	const FString RHIName = GDynamicRHI->GetName();

	if (RHIName == TEXT("D3D11"))
	{
		D3D11Device = static_cast<ID3D11Device*>(GDynamicRHI->RHIGetNativeDevice());
		D3D11Device->GetImmediateContext(&DeviceContext);
	}
	else if (RHIName == TEXT("D3D12"))
	{
		const auto Device12 = static_cast<ID3D12Device*>(GDynamicRHI->RHIGetNativeDevice());
		constexpr auto DeviceFlags11 = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

		verify(
			D3D11On12CreateDevice(
				Device12,
				DeviceFlags11,
				nullptr,
				0,
				nullptr,
				0,
				0,
				&D3D11Device,
				&DeviceContext,
				nullptr
			) == S_OK
		);

		verify(D3D11Device->QueryInterface(__uuidof(ID3D11On12Device), (void**)&D3D11on12Device) == S_OK);
	}

	Spout.OpenDirectX11(D3D11Device);
}

void
FSpoutContext::DisposeSpout()
{
	if (DeviceContext)
	{
		DeviceContext->Release();
		DeviceContext = nullptr;
	}

	if (D3D11on12Device)
	{
		D3D11on12Device->Release();
		D3D11on12Device = nullptr;
	}

	if (D3D11Device)
	{
		D3D11Device->Release();
		D3D11Device = nullptr;
	}

	Spout.ReleaseSender();
	Spout.ReleaseReceiver();
	Spout.CloseDirectX11();
}

ID3D11Texture2D*
FSpoutContext::GetD3D11Texture(const FTextureRHIRef& InTexture) const
{
	if (!InTexture)
		return nullptr;
	const FString RHIName = GDynamicRHI->GetName();

	if (RHIName == TEXT("D3D11"))
		return static_cast<ID3D11Texture2D*>(InTexture->GetNativeResource());

	if (RHIName == TEXT("D3D12"))
	{
		if (!D3D11on12Device)
			return nullptr;
		const auto NativeTex = static_cast<ID3D12Resource*>(InTexture->GetNativeResource());
		if (!NativeTex)
			return nullptr; // Handle potential null resource

		ID3D11Resource* WrappedDX11Resource = nullptr;
		const D3D11_RESOURCE_FLAGS Flags = {};
		//Flags.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		//Flags.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

		const HRESULT hr = D3D11on12Device->CreateWrappedResource(
			NativeTex,
			&Flags,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_COPY_DEST,
			__uuidof(ID3D11Resource),
			(void**)&WrappedDX11Resource
		) == S_OK;

		if (FAILED(hr))
		{
			NativeTex->Release();
			return nullptr;
		}

		return static_cast<ID3D11Texture2D*>(WrappedDX11Resource);
	}

	return nullptr;
}

EPixelFormat
FSpoutContext::ToEPixelFormat(DXGI_FORMAT DXGIFormat)
{
	switch (DXGIFormat)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return PF_R8G8B8A8;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return PF_R8G8B8A8;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return PF_R8G8B8A8;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return PF_B8G8R8A8;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return PF_B8G8R8A8;
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return PF_B8G8R8A8;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return PF_A2B10G10R10;
	case DXGI_FORMAT_R11G11B10_FLOAT:
		return PF_FloatRGB;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return PF_FloatRGBA;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return PF_A16B16G16R16;
	//case DXGI_FORMAT_R32G32B32A32_FLOAT:	return PF_A32B32G32R32F;
	default:
		return PF_Unknown;
	}
}

DXGI_FORMAT
FSpoutContext::ToDXGIFormat(EPixelFormat Format)
{
	switch (Format)
	{
	case PF_R8G8B8A8:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case PF_A2B10G10R10:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case PF_FloatRGB:
		return DXGI_FORMAT_R11G11B10_FLOAT;
	case PF_FloatRGBA:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case PF_A16B16G16R16:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}
