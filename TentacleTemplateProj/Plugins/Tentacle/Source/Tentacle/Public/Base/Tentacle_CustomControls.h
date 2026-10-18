// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tentacle_CustomControls.generated.h"

class UTclCustomControlComponent;
class UTclMsg_ServerToClient_CustomControlsInitialize;
class UTclMsg_ServerToClient_CustomControlsControlEvent;

UCLASS()
class TENTACLE_API UTentacle_CustomControls : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Custom Controls")
	TArray<UTclCustomControlComponent*> CustomControls;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void RegisterCustomControl(UTclCustomControlComponent* Control);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void UnregisterCustomControl(UTclCustomControlComponent* Control);

	/**
	 * The registered control with this id (see the control's Get Id), and
	 * whether there is one.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Custom Controls")
	UTclCustomControlComponent* FindCustomControlById(const FString& ControlId, bool& Found) const;

	void HandleMsg_Initialize(UTclMsg_ServerToClient_CustomControlsInitialize* Msg);
	void HandleMsg_ControlEvent(UTclMsg_ServerToClient_CustomControlsControlEvent* Msg);

	void SendMsg_RegisterControl(UTclCustomControlComponent* Control);
	void SendMsg_UnregisterControl(UTclCustomControlComponent* Control);
	void SendMsg_ControlState(UTclCustomControlComponent* Control);
};
