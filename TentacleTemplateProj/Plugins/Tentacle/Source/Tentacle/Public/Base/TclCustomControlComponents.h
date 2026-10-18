// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Base/TclCustomControlComponent.h"
#include "TclCustomControlComponents.generated.h"

class UTclCustomControlEvent_ValueCommit;

// ============================================================================
// Button
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Button)"))
class TENTACLE_API UTclCustomControlComponent_Button : public UTclCustomControlComponent
{
	GENERATED_BODY()
public:
	UTclCustomControlComponent_Button() { Label = TEXT("New Button"); }
	virtual FString GetControlType() const override { return TEXT("Button"); }
};

// ============================================================================
// Color
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Color)"))
class TENTACLE_API UTclCustomControlComponent_Color : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnColorValueChangedFromApp, const FColor&, NewValue, const FColor&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnColorValueCommittedFromApp, const FColor&, NewValue);

public:
	UTclCustomControlComponent_Color() { Label = TEXT("New Color"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FColor Value = FColor::White;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnColorValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnColorValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Color"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(FColor NewValue);

	/**
	 * Override to accept or refuse a color the streamer set in Tentacle, already
	 * converted. Return false to refuse it: the control keeps its current value
	 * and neither value event fires. Runs after Should Receive Value, and
	 * accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Color)"))
	bool ShouldReceiveValue_Color(const FColor& NewValue);
	virtual bool ShouldReceiveValue_Color_Implementation(const FColor& NewValue) { return true; }
};

// ============================================================================
// Int
// ============================================================================

/**
 * A whole-number field in Tentacle's panel.
 *
 * The same shape as Number without the fractional part, for counts, indices and
 * anything a decimal would be wrong for. The value is int32 rather than int64
 * because it crosses the wire as a JSON number and the app parses it as a
 * 32-bit integer.
 */
UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Int)"))
class TENTACLE_API UTclCustomControlComponent_Int : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnIntValueChangedFromApp, int32, NewValue, int32, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntValueCommittedFromApp, int32, NewValue);

public:
	UTclCustomControlComponent_Int() { Label = TEXT("New Int"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	int32 Value = 0;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnIntValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnIntValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Int"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(int32 NewValue);

	/**
	 * Override to accept or refuse a whole number the streamer set in Tentacle,
	 * already converted. Return false to refuse it: the control keeps its
	 * current value and neither value event fires. Runs after Should Receive
	 * Value, and accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Int)"))
	bool ShouldReceiveValue_Int(int32 NewValue);
	virtual bool ShouldReceiveValue_Int_Implementation(int32 NewValue) { return true; }
};

// ============================================================================
// Number
// ============================================================================

/**
 * A number field in Tentacle's panel.
 *
 * Carries the same range as a Slider; the difference is how the streamer drives
 * the value - a spinner field against a track - not what the game can constrain.
 */
UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Number)"))
class TENTACLE_API UTclCustomControlComponent_Number : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNumberValueChangedFromApp, float, NewValue, float, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNumberValueCommittedFromApp, float, NewValue);

public:
	UTclCustomControlComponent_Number() { Label = TEXT("New Number"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float MaxValue = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float Step = 1.0f;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnNumberValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnNumberValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Number"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(float NewValue);

	/**
	 * Override to accept or refuse a number the streamer set in Tentacle,
	 * already converted. Return false to refuse it: the control keeps its
	 * current value and neither value event fires. Runs after Should Receive
	 * Value, and accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Float)"))
	bool ShouldReceiveValue_Float(float NewValue);
	virtual bool ShouldReceiveValue_Float_Implementation(float NewValue) { return true; }
};

// ============================================================================
// Select
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Select)"))
class TENTACLE_API UTclCustomControlComponent_Select : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStringValueChangedFromApp, const FString&, NewValue, const FString&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStringValueCommittedFromApp, const FString&, NewValue);

public:
	UTclCustomControlComponent_Select()
	{
		Label = TEXT("New Select");
		Placeholder = TEXT("Enter value...");
		Options = { TEXT("Option 1"), TEXT("Option 2"), TEXT("Option 3") };
		Value = TEXT("Option 1");
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FString Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	TArray<FString> Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FString Placeholder;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnStringValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnStringValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Select"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(const FString& NewValue);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetOptions(const TArray<FString>& NewOptions);

	/**
	 * Override to accept or refuse an option the streamer picked in Tentacle.
	 * Return false to refuse it: the control keeps its current value and
	 * neither value event fires. Runs after Should Receive Value, and accepts
	 * every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (String)"))
	bool ShouldReceiveValue_String(const FString& NewValue);
	virtual bool ShouldReceiveValue_String_Implementation(const FString& NewValue) { return true; }
};

// ============================================================================
// Slider
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Slider)"))
class TENTACLE_API UTclCustomControlComponent_Slider : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSliderValueChangedFromApp, float, NewValue, float, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSliderValueCommittedFromApp, float, NewValue);

public:
	UTclCustomControlComponent_Slider() { Label = TEXT("New Slider"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float Value = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float MinValue = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float MaxValue = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	float Step = 1.0f;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnSliderValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnSliderValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Slider"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(float NewValue);

	/**
	 * Override to accept or refuse a number the streamer set in Tentacle,
	 * already converted. Return false to refuse it: the control keeps its
	 * current value and neither value event fires. Runs after Should Receive
	 * Value, and accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Float)"))
	bool ShouldReceiveValue_Float(float NewValue);
	virtual bool ShouldReceiveValue_Float_Implementation(float NewValue) { return true; }
};

// ============================================================================
// Switch
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Switch)"))
class TENTACLE_API UTclCustomControlComponent_Switch : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBoolValueChangedFromApp, bool, NewValue, bool, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBoolValueCommittedFromApp, bool, NewValue);

public:
	UTclCustomControlComponent_Switch() { Label = TEXT("New Switch"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	bool Value = false;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnBoolValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnBoolValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Switch"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(bool NewValue);

	/**
	 * Override to accept or refuse a switch the streamer flipped in Tentacle.
	 * Return false to refuse it: the control keeps its current value and
	 * neither value event fires. Runs after Should Receive Value, and accepts
	 * every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Bool)"))
	bool ShouldReceiveValue_Bool(bool NewValue);
	virtual bool ShouldReceiveValue_Bool_Implementation(bool NewValue) { return true; }
};

// ============================================================================
// Text
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Text)"))
class TENTACLE_API UTclCustomControlComponent_Text : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTextValueChangedFromApp, const FString&, NewValue, const FString&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextValueCommittedFromApp, const FString&, NewValue);

public:
	UTclCustomControlComponent_Text()
	{
		Label = TEXT("New Text");
		Placeholder = TEXT("Enter value...");
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FString Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FString Placeholder;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnTextValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnTextValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Text"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(const FString& NewValue);

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetPlaceholder(const FString& NewPlaceholder);

	/**
	 * Override to accept or refuse text the streamer entered in Tentacle.
	 * Return false to refuse it: the control keeps its current value and
	 * neither value event fires. Runs after Should Receive Value, and accepts
	 * every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (String)"))
	bool ShouldReceiveValue_String(const FString& NewValue);
	virtual bool ShouldReceiveValue_String_Implementation(const FString& NewValue) { return true; }
};

// ============================================================================
// Vector2
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Vector2)"))
class TENTACLE_API UTclCustomControlComponent_Vector2 : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVector2ValueChangedFromApp, const FVector2D&, NewValue, const FVector2D&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVector2ValueCommittedFromApp, const FVector2D&, NewValue);

public:
	UTclCustomControlComponent_Vector2() { Label = TEXT("New Vector2"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FVector2D Value = FVector2D::ZeroVector;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnVector2ValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnVector2ValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Vector2"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(FVector2D NewValue);

	/**
	 * Override to accept or refuse a vector the streamer set in Tentacle,
	 * already converted. Return false to refuse it: the control keeps its
	 * current value and neither value event fires. Runs after Should Receive
	 * Value, and accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Vector2)"))
	bool ShouldReceiveValue_Vector2(const FVector2D& NewValue);
	virtual bool ShouldReceiveValue_Vector2_Implementation(const FVector2D& NewValue) { return true; }
};

// ============================================================================
// Vector3
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Vector3)"))
class TENTACLE_API UTclCustomControlComponent_Vector3 : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVector3ValueChangedFromApp, const FVector&, NewValue, const FVector&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVector3ValueCommittedFromApp, const FVector&, NewValue);

public:
	UTclCustomControlComponent_Vector3() { Label = TEXT("New Vector3"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FVector Value = FVector::ZeroVector;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnVector3ValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnVector3ValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Vector3"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(FVector NewValue);

	/**
	 * Override to accept or refuse a vector the streamer set in Tentacle,
	 * already converted. Return false to refuse it: the control keeps its
	 * current value and neither value event fires. Runs after Should Receive
	 * Value, and accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Vector3)"))
	bool ShouldReceiveValue_Vector3(const FVector& NewValue);
	virtual bool ShouldReceiveValue_Vector3_Implementation(const FVector& NewValue) { return true; }
};

// ============================================================================
// Vector4
// ============================================================================

UCLASS(Blueprintable, ClassGroup="Tentacle", meta=(BlueprintSpawnableComponent, DisplayName="Custom Control (Vector4)"))
class TENTACLE_API UTclCustomControlComponent_Vector4 : public UTclCustomControlComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVector4ValueChangedFromApp, const FVector4&, NewValue, const FVector4&, OldValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVector4ValueCommittedFromApp, const FVector4&, NewValue);

public:
	UTclCustomControlComponent_Vector4() { Label = TEXT("New Vector4"); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Custom Controls")
	FVector4 Value = FVector4(0, 0, 0, 0);

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnVector4ValueChangedFromApp OnValueChangedFromApp;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Custom Controls")
	FOnVector4ValueCommittedFromApp OnValueCommittedFromApp;

	virtual FString GetControlType() const override { return TEXT("Vector4"); }
	virtual TSharedRef<FJsonObject> SerializeToJson() const override;
	virtual void ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event) override;
	virtual void ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event) override;

	UFUNCTION(BlueprintCallable, Category="Tentacle|Custom Controls")
	void SetValue(FVector4 NewValue);

	/**
	 * Override to accept or refuse a vector the streamer set in Tentacle,
	 * already converted. Return false to refuse it: the control keeps its
	 * current value and neither value event fires. Runs after Should Receive
	 * Value, and accepts every value unless overridden.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Tentacle|Custom Controls", meta=(DisplayName="Should Receive Value (Vector4)"))
	bool ShouldReceiveValue_Vector4(const FVector4& NewValue);
	virtual bool ShouldReceiveValue_Vector4_Implementation(const FVector4& NewValue) { return true; }
};
