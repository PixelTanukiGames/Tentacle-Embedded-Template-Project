// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclScreenFacingComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"

UTclScreenFacingComponent::UTclScreenFacingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	InitialRotation = FRotator::ZeroRotator;
	RotationLimits = FRotator(-1);
}

void
UTclScreenFacingComponent::BeginPlay()
{
	Super::BeginPlay();

	InitialRotation = GetComponentRotation();
}

void
UTclScreenFacingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const auto PlayerController = GetWorld()->GetFirstPlayerController();
	if (!IsValid(PlayerController))
		return;

	const auto CameraManager = PlayerController->PlayerCameraManager;
	if (!IsValid(CameraManager))
		return;

	// Calculate the rotation that would make the component face the camera.
	const auto LookAtRotation = UKismetMathLibrary::FindLookAtRotation(
		GetComponentLocation(),
		CameraManager->GetCameraLocation()
	);

	const auto CurrentRotation = GetComponentRotation();
	auto NewRotation = FRotator::ZeroRotator;

	NewRotation.Yaw = RotationLimits.Yaw >= 0
		? GetLimitedAxisRotation(CurrentRotation.Yaw, LookAtRotation.Yaw, InitialRotation.Yaw, RotationLimits.Yaw)
		: LookAtRotation.Yaw;

	NewRotation.Pitch = RotationLimits.Pitch >= 0
		? GetLimitedAxisRotation(CurrentRotation.Pitch, LookAtRotation.Pitch, InitialRotation.Pitch, RotationLimits.Pitch)
		: LookAtRotation.Pitch;

	NewRotation.Roll = RotationLimits.Roll >= 0
		? GetLimitedAxisRotation(CurrentRotation.Roll, LookAtRotation.Roll, InitialRotation.Roll, RotationLimits.Roll)
		: LookAtRotation.Roll;

	SetWorldRotation(NewRotation);
}

float
UTclScreenFacingComponent::GetLimitedAxisRotation(float Current, float Target, float Initial, float Limit)
{
	const auto TargetDelta = FMath::Clamp(FMath::FindDeltaAngleDegrees(Target, Initial), -Limit, Limit);
	const auto CurveAlpha = FMath::Clamp(FMath::Abs(TargetDelta) / Limit, 0.f, 1.f);
	const auto Alpha = IsValid(RotationLimitsCurve)
		? RotationLimitsCurve->GetFloatValue(CurveAlpha)
		: FMath::InterpEaseOut(0.f, 1.f, CurveAlpha, 2.f);

	return Initial - TargetDelta * Alpha;
}
