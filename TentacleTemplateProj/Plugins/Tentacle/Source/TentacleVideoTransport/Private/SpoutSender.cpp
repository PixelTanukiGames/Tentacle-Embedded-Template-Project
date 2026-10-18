// © 2025 Eldritch One. All rights reserved.

#include "SpoutSender.h"
#include "SpoutContext.h"

#include "RenderingThread.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "Misc/EngineVersionComparison.h"

FSpoutSender::FSpoutSender()
{
	Context = MakeShared<FSpoutContext, ESPMode::ThreadSafe>();
	Format = PF_Unknown;
	Height = Width = 0;
	Source = nullptr;
}

FSpoutSender::~FSpoutSender()
{
	Context.Reset();
}

void
FSpoutSender::SetSenderName(const FString& newName)
{
	Context->Spout.SetSenderName(toStr(newName));
}

bool
FSpoutSender::SetSenderTexture(UTextureRenderTarget2D* Texture)
{
	ResetInfo();

	ENQUEUE_RENDER_COMMAND(SpoutSend)(
		[this, Texture](FRHICommandListImmediate& RHICmd)
		{
			OutputTexture = Texture->GetRenderTargetResource()->GetRenderTargetTexture();

			const auto Desc = FRHITextureCreateDesc::Create2D(
				TEXT("TextureBuffer"),
				Texture->SizeX,
				Texture->SizeY,
				Texture->GetFormat()
			);

			// UE 5.8 deprecated the implied-immediate-list RHICreateTexture free
			// function in favour of the FRHICommandList::CreateTexture method
			// (which does not exist back in 5.1), so branch at the 5.8 boundary.
#if UE_VERSION_NEWER_THAN(5, 7, 0)
			OutputTextureBuffer = RHICmd.CreateTexture(Desc);
#else
			OutputTextureBuffer = RHICreateTexture(Desc);
#endif
			OutputTextureBufferD3D = Context->GetD3D11Texture(OutputTextureBuffer);
			DXGI_FORMAT DXGIFormat = Context->ToDXGIFormat(Texture->GetFormat());

			Context->Spout.SetSenderFormat(DXGIFormat);
			Format = Texture->GetFormat();
			Width = Texture->SizeX;
			Height = Texture->SizeY;
			Source = Texture;
		}
	);

	FlushRenderingCommands();

	if (!OutputTextureBufferD3D)
	{
		ResetInfo();
		return false;
	}
	return true;
}

bool
FSpoutSender::CheckOutputInfoChanged() const
{
	const auto bIsChanged =
		Source->GetFormat() != Format
		|| Source->SizeX != Width
		|| Source->SizeY != Height;
	return bIsChanged;
}

void
FSpoutSender::ResetInfo()
{
	Source = nullptr;
	OutputTexture = nullptr;
	OutputTextureBuffer = nullptr;
	Format = PF_Unknown;
	Width = Height = 0;

	if (OutputTextureBufferD3D)
		OutputTextureBufferD3D->Release();
}

void
FSpoutSender::SendCurrentFrame()
{
	if (!Source
		|| !Context->DeviceContext
		|| !OutputTexture.IsValid()
		|| !OutputTextureBuffer.IsValid()
		|| !OutputTextureBufferD3D)
		return;

	if (CheckOutputInfoChanged())
	{
		SetSenderTexture(Source);
		return;
	}

	ENQUEUE_RENDER_COMMAND(SpoutSend)(
		[this](FRHICommandListImmediate& RHICmd)
		{
			if (!Context.IsValid())
				return;

			//Avoid directly using the source texture
			//Engine may update the texture during the spout sending process
			RHICmd.CopyTexture(
				OutputTexture,
				OutputTextureBuffer,
				FRHICopyTextureInfo()
			);

			Context->Spout.SendTexture(OutputTextureBufferD3D);
		}
	);
}
