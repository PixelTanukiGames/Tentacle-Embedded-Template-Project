// (c) 2024 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Dom/JsonObject.h"
#include "TclCustomControlComponent.generated.h"

class UTclCustomControlEvent_ValueChange;
class UTclCustomControlEvent_ValueCommit;

UCLASS(Abstract, Blueprintable, ClassGroup="Tentacle", meta=(DisplayName="Custom Control Component"))
class TENTACLE_API UTclCustomControlComponent : public USceneComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInitialize);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClick);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMouseDown);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMouseUp);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FString Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	int32 SortIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Custom Controls")
	FString ControlId;

	/**
	 * Fires once, when the control begins play: after it has registered with
	 * Tentacle, so its id exists. A value, label or option assigned here reaches
	 * Tentacle even when it is set directly rather than through a setter.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnInitialize OnInitialize;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnClick OnClick;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnMouseDown OnMouseDown;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnMouseUp OnMouseUp;

	UFUNCTION(BlueprintPure, Category="Tentacle|Custom Controls")
	FString GetId() const { return ControlId; }

	UFUNCTION(BlueprintPure, Category="Tentacle|Custom Controls")
	virtual FString GetControlType() const { return TEXT("Button"); }

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetLabel(const FString& NewLabel);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetSortIndex(int32 NewSortIndex);

	void SetGroup(const FString& NewGroup);

	/** Serialize control state to JSON object */
	virtual TSharedRef<FJsonObject> SerializeToJson() const;

	void ReceiveClick();
	void ReceiveMouseDown();
	void ReceiveMouseUp();
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event);
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event);

	/**
	 * Override to conditionally reject incoming value changes. Return false to
	 * reject. NewValue is the value as Tentacle sent it, as text. It runs before
	 * the typed check each value control has (for example Should Receive Value
	 * (Float) on a slider), which receives the value already converted.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls")
	bool ShouldReceiveValue(const FString& NewValue);
	virtual bool ShouldReceiveValue_Implementation(const FString& NewValue) { return true; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void SendControlState();

	FString Group;

private:
	/** The Tentacle actor registered with at BeginPlay, which EndPlay unregisters from. */
	TWeakObjectPtr<class ATentacle> BoundTentacle;
};

