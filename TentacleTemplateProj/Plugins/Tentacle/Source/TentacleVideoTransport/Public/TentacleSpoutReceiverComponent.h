// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "HAL/ThreadSafeBool.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TickProvider.h"
#include "TentacleSpoutReceiverComponent.generated.h"

UCLASS(ClassGroup=(Spout), meta=(BlueprintSpawnableComponent))
class TENTACLEVIDEOTRANSPORT_API UTentacleSpoutReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

	TSharedPtr<struct FSpoutReceiver>
	Receiver;

	FTickProvider*
	TickProvider;

	bool
	bIsReceiving = false;

	// Keeps the bound render target rooted for the GC. FSpoutReceiver stores it
	// as a bare (non-UPROPERTY) pointer, so without this it could be collected
	// out from under the frame work.
	UPROPERTY(Transient)
	UTextureRenderTarget2D*
	BoundTexture = nullptr;

	// Set while a frame dispatched to the game thread is still pending, so the
	// worker thread drops ticks instead of queuing work it can't keep up with.
	FThreadSafeBool
	bFrameInFlight = false;

public:
	virtual void
	BeginDestroy() override;

	virtual void
	BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Receiver")
	TArray<FString>
	GetList() const;

	UFUNCTION(BlueprintPure, Category="Tentacle|Spout Receiver")
	bool
	GetFirstSource(FString& Name) const;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Receiver")
	bool
	Connect(const FString Name, UTextureRenderTarget2D* Output, const double FrameRate = 60.0);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Receiver")
	void
	StopReceive();

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Receiver")
	void
	ChangeFrameRate(const double FrameRate = 60.0);

private:
	void
	TickThread();
};
