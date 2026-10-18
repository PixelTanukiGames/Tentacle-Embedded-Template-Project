// © 2025 Eldritch One. All rights reserved.

#include "SpoutReceiver.h"
#include "SpoutContext.h"

#include "Engine/Engine.h"
#include "RenderingThread.h"
#include "TextureResource.h"

#include "Windows/AllowWindowsPlatformTypes.h"
#include <d3d11on12.h>
#include "Windows/HideWindowsPlatformTypes.h"

FSpoutReceiver::FSpoutReceiver()
{
	Width = Height = 0;
	Format = DXGI_FORMAT_UNKNOWN;
	Context = MakeShared<FSpoutContext, ESPMode::ThreadSafe>();
	//Context->Spout.EnableSpoutLog();
}

FSpoutReceiver::~FSpoutReceiver()
{
	Context.Reset();
}

TArray<FString>
FSpoutReceiver::GetList() const
{
	const auto SenderCount = Context->Spout.GetSenderCount();
	TArray<FString> Result = {};
	if (SenderCount < 1)
		return Result;

	char sendername[256]{};
	for (auto i = 0; i < SenderCount; i++)
	{
		if (Context->Spout.GetSender(i, sendername))
			Result.Add(FString(sendername));
	}

	return Result;
}

bool
FSpoutReceiver::Connect(const FString& Name, UTextureRenderTarget2D* BindTexture)
{
	if (Name.Len() >= 256)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			10,
			FColor::Green,
			TEXT("Sender Name too long")
		);
		return false;
	}

	char NameChar[256]{};
	strcpy_s(NameChar, 256, TCHAR_TO_ANSI(*Name));

	TargetTexture = BindTexture;
	Context->Spout.SetReceiverName(NameChar);
	Context->Spout.ReceiveTexture();

	const auto bSuccess = UpdateSpoutInfo();
	if (!bSuccess)
		Context->Spout.SetReceiverName();

	return bSuccess;
}

bool
FSpoutReceiver::UpdateSpoutInfo()
{
	if (!TargetTexture)
		return false;
	const auto Spout = &Context->Spout;
	if (!Spout || !Spout->IsConnected())
		return false;

	const int32 newWidth = Spout->GetSenderWidth();
	const int32 newHeight = Spout->GetSenderHeight();
	const auto newFormat = Spout->GetSenderFormat();
	const auto newEPixelFormat = Context->ToEPixelFormat(newFormat);

	if (newWidth < 1
		|| newHeight < 1
		|| newEPixelFormat == PF_Unknown)
		return false;

	if (newWidth != Width
		|| newHeight != Height
		|| newFormat != Format)
	{
		TargetTexture->ResizeTarget(newWidth, newHeight);
		TargetTexture->OverrideFormat = newEPixelFormat;
		TargetTexture->UpdateResourceImmediate(false);

		ID3D11Texture2D* newTextureD3D = nullptr;

		ENQUEUE_RENDER_COMMAND(SpoutRecv)(
			[this, &newTextureD3D](FRHICommandListImmediate& RHICmd)
			{
				const auto RHI = TargetTexture->GetResource()->GetTextureRHI();
				newTextureD3D = Context->GetD3D11Texture(RHI);
			}
		);

		//Wait Rendering Thread get RHI
		FlushRenderingCommands();
		if (!newTextureD3D)
			return false;

		Width = newWidth;
		Height = newHeight;
		Format = newFormat;
		TargetTextureD3D = newTextureD3D;
	}
	return true;
}

void
FSpoutReceiver::ReceiveCurrentFrame()
{
	if (!TargetTexture || !TargetTextureD3D || !Context.IsValid())
		return;

	if (Context->Spout.ReceiveTexture(&TargetTextureD3D))
		if (Context->Spout.IsUpdated())
			UpdateSpoutInfo();
}
