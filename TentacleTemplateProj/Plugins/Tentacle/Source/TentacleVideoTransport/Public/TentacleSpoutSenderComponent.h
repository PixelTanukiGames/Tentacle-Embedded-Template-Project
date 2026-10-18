// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "HAL/ThreadSafeBool.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TickProvider.h"
#include "TentacleSpoutSenderComponent.generated.h"

UCLASS(ClassGroup=(Spout), meta=(BlueprintSpawnableComponent))
class TENTACLEVIDEOTRANSPORT_API UTentacleSpoutSenderComponent : public UActorComponent
{
	GENERATED_BODY()

	TSharedPtr<struct FSpoutSender>
	Sender;

	FTickProvider*
	TickProvider;

	bool
	bIsInitialized = false;

	// Keeps the source render target rooted for the GC. FSpoutSender stores it
	// as a bare (non-UPROPERTY) pointer, so without this it could be collected
	// out from under the frame work.
	UPROPERTY(Transient)
	UTextureRenderTarget2D*
	SourceTexture = nullptr;

	// Set while a frame dispatched to the game thread is still pending, so the
	// worker thread drops ticks instead of queuing work it can't keep up with.
	FThreadSafeBool
	bFrameInFlight = false;

public:
	virtual void
	BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Sender")
	bool
	Start(const FString& SenderName, UTextureRenderTarget2D* Source, const double FrameRate = 60.0);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Sender")
	void
	Stop();

	UFUNCTION(BlueprintCallable, Category="Tentacle|Spout Sender")
	void
	ChangeFrameRate(const double FrameRate = 60.0);

private:
	void
	TickThread();
};
