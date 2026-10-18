// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TclCustomControlGroupComponent.generated.h"

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Group)"))
class TENTACLE_API UTclCustomControlGroupComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UTclCustomControlGroupComponent() { GroupName = TEXT("New Group"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FString GroupName;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetGroupName(const FString& NewGroupName);

protected:
	virtual void BeginPlay() override;

private:
	void UpdateDescendantControls();
};
