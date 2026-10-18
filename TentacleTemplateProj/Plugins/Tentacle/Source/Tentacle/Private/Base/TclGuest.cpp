// © 2025 Eldritch One. All rights reserved.

#include "Base/TclGuest.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TentacleSpoutReceiverComponent.h"
#include "TentacleModule.h"
#include "Base/TclCustomControlComponents.h"
#include "Base/TclCustomControlGroupComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclScreenFacingComponent.h"

ATclGuest::ATclGuest()
{
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	ScreenFacing = CreateDefaultSubobject<UTclScreenFacingComponent>(TEXT("ScreenFacing"));
	ScreenFacing->SetupAttachment(Root);
	ScreenFacing->RotationLimits = FRotator(15.0f, 30.0f, 0.0f);

	Video = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Video"));
	Video->SetupAttachment(ScreenFacing);
	Video->bVisibleInRayTracing = false;
	Video->bCastShadowAsTwoSided = true;

	NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Name"));
	NameText->SetupAttachment(Video);
	NameText->SetText(FText::FromString(TEXT("Guest 1")));
	NameText->SetHorizontalAlignment(EHTA_Center);
	NameText->SetTextRenderColor(FColor(255, 0, 255, 255));
	NameText->WorldSize = 14.0f;
	NameText->SetRelativeLocation(FVector(0.0f, 0.0f, 77.689036f));

	CustomControlGroup = CreateDefaultSubobject<UTclCustomControlGroupComponent>(TEXT("CustomControlGroup"));
	CustomControlGroup->SetupAttachment(Root);
	CustomControlGroup->GroupName = TEXT("Guest");

	CC_DisplayName = CreateDefaultSubobject<UTclCustomControlComponent_Text>(TEXT("CC_DisplayName"));
	CC_DisplayName->SetupAttachment(CustomControlGroup);
	CC_DisplayName->Label = TEXT("Display Name");
	CC_DisplayName->Value = TEXT("Guest");
	CC_DisplayName->SortIndex = 0;

	CC_SpoutSource = CreateDefaultSubobject<UTclCustomControlComponent_Text>(TEXT("CC_SpoutSource"));
	CC_SpoutSource->SetupAttachment(CustomControlGroup);
	CC_SpoutSource->Label = TEXT("Spout Source");
	CC_SpoutSource->SortIndex = 1;

	CC_ChromaKey = CreateDefaultSubobject<UTclCustomControlComponent_Color>(TEXT("CC_ChromaKey"));
	CC_ChromaKey->SetupAttachment(CustomControlGroup);
	CC_ChromaKey->Label = TEXT("Chroma Key");
	CC_ChromaKey->Value = FColor(0, 255, 0, 255);
	CC_ChromaKey->SortIndex = 2;

	CC_ChromaKeyOn = CreateDefaultSubobject<UTclCustomControlComponent_Switch>(TEXT("CC_ChromaKeyOn"));
	CC_ChromaKeyOn->SetupAttachment(CustomControlGroup);
	CC_ChromaKeyOn->Label = TEXT("Chroma Key On");
	CC_ChromaKeyOn->Value = true;
	CC_ChromaKeyOn->SortIndex = 3;

	CC_Cropping = CreateDefaultSubobject<UTclCustomControlComponent_Vector4>(TEXT("CC_Cropping"));
	CC_Cropping->SetupAttachment(CustomControlGroup);
	CC_Cropping->Label = TEXT("Cropping");
	CC_Cropping->SortIndex = 4;

	CC_Offset = CreateDefaultSubobject<UTclCustomControlComponent_Vector2>(TEXT("CC_Offset"));
	CC_Offset->SetupAttachment(CustomControlGroup);
	CC_Offset->Label = TEXT("Offset");
	CC_Offset->SortIndex = 5;

	CC_Zoom = CreateDefaultSubobject<UTclCustomControlComponent_Number>(TEXT("CC_Zoom"));
	CC_Zoom->SetupAttachment(CustomControlGroup);
	CC_Zoom->Label = TEXT("Zoom");
	CC_Zoom->SortIndex = 6;

	CC_ZoomRatio = CreateDefaultSubobject<UTclCustomControlComponent_Number>(TEXT("CC_ZoomRatio"));
	CC_ZoomRatio->SetupAttachment(CustomControlGroup);
	CC_ZoomRatio->Label = TEXT("Zoom Ratio");
	CC_ZoomRatio->Value = 1.0f;
	CC_ZoomRatio->SortIndex = 7;

	CC_Location = CreateDefaultSubobject<UTclCustomControlComponent_Vector3>(TEXT("CC_Location"));
	CC_Location->SetupAttachment(CustomControlGroup);
	CC_Location->Label = TEXT("Location");
	CC_Location->SortIndex = 8;

	CC_Scale = CreateDefaultSubobject<UTclCustomControlComponent_Vector3>(TEXT("CC_Scale"));
	CC_Scale->SetupAttachment(CustomControlGroup);
	CC_Scale->Label = TEXT("Scale");
	CC_Scale->Value = FVector(1.0f, 1.0f, 1.0f);
	CC_Scale->SortIndex = 9;

	CC_RotationLimits = CreateDefaultSubobject<UTclCustomControlComponent_Vector3>(TEXT("CC_RotationLimits"));
	CC_RotationLimits->SetupAttachment(CustomControlGroup);
	CC_RotationLimits->Label = TEXT("Rotation Limits");
	CC_RotationLimits->Value = FVector(0.0f, 15.0f, 30.0f);
	CC_RotationLimits->SortIndex = 10;

	CC_Visible = CreateDefaultSubobject<UTclCustomControlComponent_Switch>(TEXT("CC_Visible"));
	CC_Visible->SetupAttachment(CustomControlGroup);
	CC_Visible->Label = TEXT("Visible");
	CC_Visible->Value = true;
	CC_Visible->SortIndex = 11;

	SpoutReceiver = CreateDefaultSubobject<UTentacleSpoutReceiverComponent>(TEXT("SpoutReceiver"));

	Material = nullptr;
	RenderTarget = nullptr;
}

void
ATclGuest::BeginPlay()
{
	Super::BeginPlay();

	if (!ValidateComponents())
	{
		SetActorTickEnabled(false);
		return;
	}

	Material = Video->CreateDynamicMaterialInstance(0, MI_Guest);
	TrySpoutConnect();

	// Events handlers
	CC_DisplayName->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleDisplayNameKeyChangedFromApp);
	CC_SpoutSource->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleSpoutSourceChangedFromApp);
	CC_ChromaKey->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleChromaKeyChangedFromApp);
	CC_ChromaKeyOn->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleChromaKeyOnChangedFromApp);
	CC_Cropping->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleCroppingChangedFromApp);
	CC_Offset->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleOffsetChangedFromApp);
	CC_Zoom->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleZoomChangedFromApp);
	CC_ZoomRatio->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleZoomRatioChangedFromApp);
	CC_Location->OnInitialize.AddDynamic(this, &ATclGuest::HandleLocationInitialized);
	CC_Location->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleLocationChangedFromApp);
	CC_Scale->OnInitialize.AddDynamic(this, &ATclGuest::HandleScaleInitialized);
	CC_Scale->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleScaleChangedFromApp);
	CC_RotationLimits->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleRotationLimitsChangedFromApp);
	CC_Visible->OnValueChangedFromApp.AddDynamic(this, &ATclGuest::HandleVisibleChangedFromApp);
}

void
ATclGuest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// BeginPlay disables ticking when validation fails, but a Blueprint or a
	// later SetActorTickEnabled call can turn it back on, so re-check here.
	if (!IsValid(Video))
		return;

	// Add micro-jitter to the video plane to keep shadows updating from video
	// even when the plane/camera are static.
	Video->SetRelativeLocation(
		UKismetMathLibrary::RandomUnitVector() * 0.001f,
		false,
		nullptr,
		ETeleportType::TeleportPhysics
	);
}

bool
ATclGuest::ValidateComponents() const
{
	TArray<FString> Missing;

	const auto Check = [&Missing](const UActorComponent* Component, const TCHAR* Name)
	{
		if (!IsValid(Component))
			Missing.Add(Name);
	};

	Check(Root, TEXT("Root"));
	Check(ScreenFacing, TEXT("ScreenFacing"));
	Check(Video, TEXT("Video"));
	Check(NameText, TEXT("Name"));
	Check(CustomControlGroup, TEXT("CustomControlGroup"));
	Check(CC_DisplayName, TEXT("CC_DisplayName"));
	Check(CC_SpoutSource, TEXT("CC_SpoutSource"));
	Check(CC_ChromaKey, TEXT("CC_ChromaKey"));
	Check(CC_ChromaKeyOn, TEXT("CC_ChromaKeyOn"));
	Check(CC_Cropping, TEXT("CC_Cropping"));
	Check(CC_Offset, TEXT("CC_Offset"));
	Check(CC_Zoom, TEXT("CC_Zoom"));
	Check(CC_ZoomRatio, TEXT("CC_ZoomRatio"));
	Check(CC_Location, TEXT("CC_Location"));
	Check(CC_Scale, TEXT("CC_Scale"));
	Check(CC_RotationLimits, TEXT("CC_RotationLimits"));
	Check(CC_Visible, TEXT("CC_Visible"));
	Check(SpoutReceiver, TEXT("SpoutReceiver"));

	tclSoftAssertMsgR(
		Missing.IsEmpty(),
		FString::Printf(
			TEXT("%s is missing components (%s); the guest has been disabled. Re-add them on the actor or its Blueprint."),
			*GetName(),
			*FString::Join(Missing, TEXT(", "))
		),
		false
	)

	return true;
}

bool
ATclGuest::TrySpoutConnect() const
{
	if (!IsValid(SpoutReceiver) || !IsValid(CC_SpoutSource))
		return false;

	SpoutReceiver->StopReceive();
	UpdateMaterial();
	if (!SpoutReceiver->Connect(CC_SpoutSource->Value, RenderTarget, 60))
		return false;
	UE_LOG(LogTentacle, Display, TEXT("Spout connected to source %s"), *CC_SpoutSource->Value);
	return true;
}

void
ATclGuest::UpdateMaterial() const
{
	// Null whenever the Video mesh has no material to instance from, which is
	// independent of the component checks done at BeginPlay.
	if (!IsValid(Material))
		return;

	Material->SetTextureParameterValue(TEXT("Render Target"), RenderTarget);
	Material->SetVectorParameterValue(TEXT("Chroma Key"), CC_ChromaKey->Value);
	Material->SetScalarParameterValue(TEXT("Chroma Key On"), CC_ChromaKeyOn->Value ? 1.0f : 0.0f);
	Material->SetVectorParameterValue(TEXT("Cropping"), CC_Cropping->Value);
	Material->SetScalarParameterValue(TEXT("Offset X"), CC_Offset->Value.X);
	Material->SetScalarParameterValue(TEXT("Offset Y"), CC_Offset->Value.Y);
	Material->SetScalarParameterValue(TEXT("Zoom"), CC_Zoom->Value);
	Material->SetScalarParameterValue(TEXT("Zoom Ratio"), CC_ZoomRatio->Value);
}

void
ATclGuest::HandleSpoutSourceChangedFromApp(const FString& NewValue, const FString& OldValue)
{
	TrySpoutConnect();
}

void
ATclGuest::HandleDisplayNameKeyChangedFromApp(const FString& NewValue, const FString& OldValue)
{
	NameText->SetText(FText::FromString(NewValue));
}

void
ATclGuest::HandleChromaKeyChangedFromApp(const FColor& NewValue, const FColor& OldValue)
{
	UpdateMaterial();
}

void
ATclGuest::HandleChromaKeyOnChangedFromApp(bool NewValue, bool OldValue)
{
	UpdateMaterial();
}

void
ATclGuest::HandleCroppingChangedFromApp(const FVector4& NewValue, const FVector4& OldValue)
{
	UpdateMaterial();
}

void
ATclGuest::HandleOffsetChangedFromApp(const FVector2D& NewValue, const FVector2D& OldValue)
{
	UpdateMaterial();
}

void
ATclGuest::HandleZoomChangedFromApp(float NewValue, float OldValue)
{
	UpdateMaterial();
}

void
ATclGuest::HandleZoomRatioChangedFromApp(float NewValue, float OldValue)
{
	UpdateMaterial();
}

void
ATclGuest::HandleLocationInitialized()
{
	CC_Location->SetValue(GetActorLocation());
}

void
ATclGuest::HandleLocationChangedFromApp(const FVector& NewValue, const FVector& OldValue)
{
	SetActorLocation(NewValue, false, nullptr, ETeleportType::TeleportPhysics);
}

void
ATclGuest::HandleScaleInitialized()
{
	CC_Scale->SetValue(GetActorScale3D());
}

void
ATclGuest::HandleScaleChangedFromApp(const FVector& NewValue, const FVector& OldValue)
{
	SetActorScale3D(NewValue);
}

void
ATclGuest::HandleRotationLimitsChangedFromApp(const FVector& NewValue, const FVector& OldValue)
{
	ScreenFacing->RotationLimits = FRotator::MakeFromEuler(FVector(NewValue.Z, NewValue.X, NewValue.Y));
}

void
ATclGuest::HandleVisibleChangedFromApp(bool NewValue, bool OldValue)
{
	SetActorHiddenInGame(!NewValue);
}
