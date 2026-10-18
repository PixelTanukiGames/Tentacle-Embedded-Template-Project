// © 2025 Eldritch One. All rights reserved.

#include "TentacleSpoutSenderComponent.h"
#include "SpoutSender.h"
#include "Async/Async.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderingThread.h"

void
UTentacleSpoutSenderComponent::BeginDestroy()
{
	Stop();
	Super::BeginDestroy();
}

bool
UTentacleSpoutSenderComponent::Start(
	const FString& SenderName,
	UTextureRenderTarget2D* Source,
	const double FrameRate
)
{
	if (!Source || bIsInitialized || Sender.IsValid())
		return false;

	Sender = MakeShared<FSpoutSender, ESPMode::ThreadSafe>();

	Sender->SetSenderName(SenderName);
	bIsInitialized = Sender->SetSenderTexture(Source);

	if (bIsInitialized)
	{
		SourceTexture = Source;
		TickProvider = new FTickProvider(FrameRate);
		TickProvider->Tick.BindUObject(this, &UTentacleSpoutSenderComponent::TickThread);
	}

	return bIsInitialized;
}

void
UTentacleSpoutSenderComponent::Stop()
{
	if (TickProvider)
	{
		delete TickProvider;
		TickProvider = nullptr;
		FlushRenderingCommands();
	}
	if (Sender.IsValid())
		Sender.Reset();
	Sender = nullptr;
	bIsInitialized = false;
	SourceTexture = nullptr;
}

void
UTentacleSpoutSenderComponent::ChangeFrameRate(const double FrameRate)
{
	if (!TickProvider)
		return;
	TickProvider->SetFPS(FrameRate);
}

void
UTentacleSpoutSenderComponent::TickThread()
{
	// Runs on the FTickProvider worker thread. Sending a frame reads UObjects
	// (SizeX/Format), can call FlushRenderingCommands, and enqueues render
	// commands — all of which must happen on the game thread. Marshal there and
	// drop this tick if the previous frame hasn't finished yet.
	if (bFrameInFlight.AtomicSet(true))
		return;

	TWeakObjectPtr<UTentacleSpoutSenderComponent> WeakThis(this);
	AsyncTask(ENamedThreads::GameThread, [WeakThis]()
	{
		UTentacleSpoutSenderComponent* Self = WeakThis.Get();
		if (!Self)
			return;
		if (Self->Sender.IsValid() && Self->bIsInitialized)
			Self->Sender->SendCurrentFrame();
		Self->bFrameInFlight = false;
	});
}
