// © 2025 Eldritch One. All rights reserved.

#include "TentacleSpoutReceiverComponent.h"
#include "SpoutReceiver.h"
#include "Async/Async.h"

void
UTentacleSpoutReceiverComponent::BeginDestroy()
{
	// Stop the worker thread first so it can't fire TickThread on a component
	// that is being torn down (and so the thread isn't leaked).
	StopReceive();
	if (Receiver.IsValid())
		Receiver.Reset();
	Receiver = nullptr;
	Super::BeginDestroy();
}

void
UTentacleSpoutReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	Receiver = MakeShared<FSpoutReceiver>();
}

TArray<FString>
UTentacleSpoutReceiverComponent::GetList() const
{
	if (!Receiver.IsValid())
		return TArray<FString>();
	return Receiver->GetList();
}

bool
UTentacleSpoutReceiverComponent::GetFirstSource(FString& Name) const
{
	auto List = GetList();
	const auto bSuccess = !List.IsEmpty();
	if (bSuccess)
		Name = List[0];
	return bSuccess;
}

void
UTentacleSpoutReceiverComponent::ChangeFrameRate(const double FrameRate)
{
	if (!TickProvider)
		return;
	TickProvider->SetFPS(FrameRate);
}

bool
UTentacleSpoutReceiverComponent::Connect(
	const FString Name,
	UTextureRenderTarget2D* Output,
	const double FrameRate
)
{
	if (!IsValid(Output) || !Receiver.IsValid())
		return false;
	if (bIsReceiving)
		StopReceive();

	bIsReceiving = Receiver->Connect(Name, Output);

	if (bIsReceiving)
	{
		BoundTexture = Output;
		TickProvider = new FTickProvider(FrameRate);
		TickProvider->Tick.BindUObject(this, &UTentacleSpoutReceiverComponent::TickThread);
	}

	return bIsReceiving;
}

void
UTentacleSpoutReceiverComponent::StopReceive()
{
	if (TickProvider)
		delete TickProvider;
	TickProvider = nullptr;
	bIsReceiving = false;
	BoundTexture = nullptr;
}

void
UTentacleSpoutReceiverComponent::TickThread()
{
	// Runs on the FTickProvider worker thread. Receiving a frame can resize the
	// target render target (ResizeTarget / UpdateResourceImmediate), call
	// FlushRenderingCommands, and enqueue render commands — all game-thread-only
	// operations. Marshal there and drop this tick if the previous frame hasn't
	// finished yet.
	if (bFrameInFlight.AtomicSet(true))
		return;

	TWeakObjectPtr<UTentacleSpoutReceiverComponent> WeakThis(this);
	AsyncTask(ENamedThreads::GameThread, [WeakThis]()
	{
		UTentacleSpoutReceiverComponent* Self = WeakThis.Get();
		if (!Self)
			return;
		if (Self->Receiver.IsValid() && Self->bIsReceiving)
			Self->Receiver->ReceiveCurrentFrame();
		Self->bFrameInFlight = false;
	});
}
