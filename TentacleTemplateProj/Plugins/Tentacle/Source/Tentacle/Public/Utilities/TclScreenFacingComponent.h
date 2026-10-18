// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "TclScreenFacingComponent.generated.h"

UCLASS(ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Screen Facing Component"))
class TENTACLE_API UTclScreenFacingComponent : public USceneComponent
{
	GENERATED_BODY()

protected:
	UPROPERTY()
	FRotator
	InitialRotation;

public:
	/**
	 * Rotation limits for clamping the screen facing behavior (-1 for no limit).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Screen Facing")
	FRotator
	RotationLimits;

	/**
	 * Optional curve for controlling the rotation behavior when approaching the limits.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Screen Facing")
	UCurveFloat*
	RotationLimitsCurve;

	UTclScreenFacingComponent();

	virtual void
	BeginPlay() override;

	virtual void
	TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float
	GetLimitedAxisRotation(float Current, float Target, float Initial, float Limit);
};
