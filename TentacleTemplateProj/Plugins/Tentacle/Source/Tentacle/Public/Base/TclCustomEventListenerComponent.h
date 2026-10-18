// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TclCustomEventListenerComponent.generated.h"

class UTclViewer;
class UTclCustomEvent;

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Event Listener"))
class TENTACLE_API UTclCustomEventListenerComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTriggered, const UTclCustomEvent*, Event, UTclViewer*, Viewer);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Events")
	TArray<FString> EventNames;

	/**
	 * Match Event Names exactly, letter case included. Names are matched
	 * case-insensitively by default, so "Demo Party" also answers to
	 * "demo party".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Events")
	bool bCaseSensitive = false;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Events")
	FOnTriggered OnTriggered;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleCustomEvent(const UTclCustomEvent* Event, UTclViewer* Viewer);

	/** The Tentacle actor bound to at BeginPlay, which EndPlay unbinds from. */
	TWeakObjectPtr<class ATentacle> BoundTentacle;
};
