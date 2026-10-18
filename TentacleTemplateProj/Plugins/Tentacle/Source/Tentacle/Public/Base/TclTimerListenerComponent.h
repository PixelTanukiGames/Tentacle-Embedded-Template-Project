// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TclTimerListenerComponent.generated.h"

class UTclTimerEvent;

/**
 * Listens for the timers named in Timer Names and fires On Fired for each of
 * their fires, so a scene reacts to a timer the streamer defined in Tentacle
 * with no code.
 */
UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Timer Listener"))
class TENTACLE_API UTclTimerListenerComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFired, const UTclTimerEvent*, Event);

public:
	/** The timers to listen for, by the names the streamer gave them in Tentacle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers")
	TArray<FString> TimerNames;

	/**
	 * Match Timer Names exactly, letter case included. Names are matched
	 * case-insensitively by default, so "Boss" also answers to "boss".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers")
	bool bCaseSensitive = false;

	/** Fires for every fire of a timer named in Timer Names. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Timers")
	FOnFired OnFired;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleTimerFired(const UTclTimerEvent* Event);

	/** The Tentacle actor bound to at BeginPlay, which EndPlay unbinds from. */
	TWeakObjectPtr<class ATentacle> BoundTentacle;
};
