// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TclGuest.generated.h"

class UTclScreenFacingComponent;
class UTclCustomControlGroupComponent;
class UTclCustomControlComponent_Text;
class UTclCustomControlComponent_Vector2;
class UTclCustomControlComponent_Vector3;
class UTclCustomControlComponent_Vector4;
class UTclCustomControlComponent_Number;
class UTclCustomControlComponent_Switch;
class UTclCustomControlComponent_Color;
class UTentacleSpoutReceiverComponent;
class UTextRenderComponent;

UCLASS(Abstract, Blueprintable, meta=(DisplayName="Guest"))
class TENTACLE_API ATclGuest : public AActor
{
	GENERATED_BODY()

public:
	ATclGuest();

	// ── Component Hierarchy ──────────────────────────────────────────────

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclScreenFacingComponent* ScreenFacing;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UStaticMeshComponent* Video;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTextRenderComponent* NameText;

	// ── Custom Controls ──────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlGroupComponent* CustomControlGroup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Text* CC_DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Text* CC_SpoutSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Color* CC_ChromaKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Switch* CC_ChromaKeyOn;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Vector4* CC_Cropping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Vector2* CC_Offset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Number* CC_Zoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Number* CC_ZoomRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Vector3* CC_Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Vector3* CC_Scale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Vector3* CC_RotationLimits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTclCustomControlComponent_Switch* CC_Visible;

	// ── Video Transport ──────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTentacleSpoutReceiverComponent* SpoutReceiver;

	// ── Material ─────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UMaterialInstance* MI_Guest;

	UPROPERTY()
	UMaterialInstanceDynamic* Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Guest")
	UTextureRenderTarget2D* RenderTarget;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Guest")
	bool TrySpoutConnect() const;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Guest")
	void UpdateMaterial() const;

	UFUNCTION()
	void HandleSpoutSourceChangedFromApp(const FString& NewValue, const FString& OldValue);

	UFUNCTION()
	void HandleDisplayNameKeyChangedFromApp(const FString& NewValue, const FString& OldValue);

	UFUNCTION()
	void HandleChromaKeyChangedFromApp(const FColor& NewValue, const FColor& OldValue);

	UFUNCTION()
	void HandleChromaKeyOnChangedFromApp(bool NewValue, bool OldValue);

	UFUNCTION()
	void HandleCroppingChangedFromApp(const FVector4& NewValue, const FVector4& OldValue);

	UFUNCTION()
	void HandleOffsetChangedFromApp(const FVector2D& NewValue, const FVector2D& OldValue);

	UFUNCTION()
	void HandleZoomChangedFromApp(float NewValue, float OldValue);

	UFUNCTION()
	void HandleZoomRatioChangedFromApp(float NewValue, float OldValue);

	UFUNCTION()
	void HandleLocationInitialized();

	UFUNCTION()
	void HandleLocationChangedFromApp(const FVector& NewValue, const FVector& OldValue);

	UFUNCTION()
	void HandleScaleInitialized();

	UFUNCTION()
	void HandleScaleChangedFromApp(const FVector& NewValue, const FVector& OldValue);

	UFUNCTION()
	void HandleRotationLimitsChangedFromApp(const FVector& NewValue, const FVector& OldValue);

	UFUNCTION()
	void HandleVisibleChangedFromApp(bool NewValue, bool OldValue);

private:
	// The components above are created as default subobjects, but they are
	// exposed properties, so a level or Blueprint instance can deserialize with
	// any of them set to None (an instance override saved against an older
	// version of this class, for example). Everything downstream assumes they
	// exist, so check once on BeginPlay and shut the actor down cleanly instead
	// of crashing the game as the actor streams in.
	bool ValidateComponents() const;
};
