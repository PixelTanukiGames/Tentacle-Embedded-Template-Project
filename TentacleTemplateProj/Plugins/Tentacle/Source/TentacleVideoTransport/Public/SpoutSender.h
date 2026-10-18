// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "RHI.h"
#include "Engine/TextureRenderTarget2D.h"

/**
 * Data for a Spout sender.
 */
struct FSpoutSender
{
private:
	int32
	Width;

	int32
	Height;

	EPixelFormat
	Format;

	TSharedPtr<struct FSpoutContext>
	Context;

	UTextureRenderTarget2D*
	Source;

	FTextureRHIRef
	OutputTexture = nullptr;

	FTextureRHIRef
	OutputTextureBuffer = nullptr;

	struct ID3D11Texture2D*
	OutputTextureBufferD3D = nullptr;

	double
	NextFrameTime = 0;

	char*
	toStr(FString fstring) { return TCHAR_TO_ANSI(*fstring); }

	bool
	CheckOutputInfoChanged() const;

	void
	ResetInfo();

public:
	FSpoutSender();
	~FSpoutSender();

	void
	SetSenderName(const FString& newName);

	bool
	SetSenderTexture(UTextureRenderTarget2D* Texture);

	void
	SendCurrentFrame();
};
