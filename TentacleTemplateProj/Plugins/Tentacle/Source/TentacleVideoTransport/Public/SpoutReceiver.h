// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"

enum DXGI_FORMAT : int;

/**
 * Data for a Spout receiver.
 */
struct FSpoutReceiver
{
private:
	TSharedPtr<struct FSpoutContext>
	Context;

	UTextureRenderTarget2D*
	TargetTexture = nullptr;

	struct ID3D11Texture2D*
	TargetTextureD3D = nullptr;

	FString
	ConnectedName = "";

	int32
	Width;

	int32
	Height;

	DXGI_FORMAT
	Format;

	double
	NextFrameTime = 0;

	bool
	UpdateSpoutInfo();

public:
	FSpoutReceiver();
	~FSpoutReceiver();

	TArray<FString>
	GetList() const;

	bool
	Connect(const FString& ConnectedName, UTextureRenderTarget2D* BindTexture);

	void
	ReceiveCurrentFrame();
};
