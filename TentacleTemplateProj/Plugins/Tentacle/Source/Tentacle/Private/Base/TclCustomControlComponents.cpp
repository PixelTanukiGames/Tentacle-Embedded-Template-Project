// © 2025 Eldritch One. All rights reserved.

#include "Base/TclCustomControlComponents.h"
#include "Sdk/TclCustomControlEvent_ValueChange.h"
#include "Sdk/TclCustomControlEvent_ValueCommit.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// Every value control checks an inbound value twice before taking it: the raw
// Should Receive Value with the text Tentacle sent, then the typed check with
// the value converted to the control's own type. Either can refuse, and a
// refusal returns before the control's value is touched, so the control and
// what it serializes for Tentacle stay exactly as they were.

// ============================================================================
// Color
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Color::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	const FString HexValue = FString::Printf(TEXT("#%02X%02X%02X"), Value.R, Value.G, Value.B);
	Obj->SetStringField(TEXT("value"), HexValue);
	return Obj;
}

void UTclCustomControlComponent_Color::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	FString Hex = Event->Value;
	Hex.RemoveFromStart(TEXT("#"));
	if (Hex.Len() != 6 && Hex.Len() != 8) return;

	const FColor NewValue = FColor::FromHex(Event->Value);
	if (!ShouldReceiveValue_Color(NewValue)) return;

	const FColor OldValue = Value;
	Value = NewValue;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Color::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	FString Hex = Event->Value;
	Hex.RemoveFromStart(TEXT("#"));
	if (Hex.Len() != 6 && Hex.Len() != 8) return;

	const FColor NewValue = FColor::FromHex(Event->Value);
	if (!ShouldReceiveValue_Color(NewValue)) return;

	Value = NewValue;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Color::SetValue(FColor NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Int
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Int::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	Obj->SetNumberField(TEXT("value"), Value);
	return Obj;
}

void UTclCustomControlComponent_Int::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const int32 NewValue = FCString::Atoi(*Event->Value);
	if (!ShouldReceiveValue_Int(NewValue)) return;

	const int32 OldValue = Value;
	Value = NewValue;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Int::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const int32 NewValue = FCString::Atoi(*Event->Value);
	if (!ShouldReceiveValue_Int(NewValue)) return;

	Value = NewValue;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Int::SetValue(int32 NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Number
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Number::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	Obj->SetNumberField(TEXT("value"), Value);
	Obj->SetNumberField(TEXT("min"), MinValue);
	Obj->SetNumberField(TEXT("max"), MaxValue);
	Obj->SetNumberField(TEXT("step"), Step);
	return Obj;
}

void UTclCustomControlComponent_Number::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const float NewValue = FCString::Atof(*Event->Value);
	if (!ShouldReceiveValue_Float(NewValue)) return;

	const float OldValue = Value;
	Value = NewValue;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Number::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const float NewValue = FCString::Atof(*Event->Value);
	if (!ShouldReceiveValue_Float(NewValue)) return;

	Value = NewValue;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Number::SetValue(float NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Select
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Select::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	Obj->SetStringField(TEXT("value"), Value);
	if (!Placeholder.IsEmpty())
		Obj->SetStringField(TEXT("placeholder"), Placeholder);

	TArray<TSharedPtr<FJsonValue>> OptionsArray;
	for (const FString& Opt : Options)
		OptionsArray.Add(MakeShared<FJsonValueString>(Opt));
	Obj->SetArrayField(TEXT("options"), OptionsArray);
	return Obj;
}

void UTclCustomControlComponent_Select::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;
	if (!ShouldReceiveValue_String(Event->Value)) return;

	const FString OldValue = Value;
	Value = Event->Value;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Select::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;
	if (!ShouldReceiveValue_String(Event->Value)) return;

	Value = Event->Value;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Select::SetValue(const FString& NewValue)
{
	Value = NewValue;
	SendControlState();
}

void UTclCustomControlComponent_Select::SetOptions(const TArray<FString>& NewOptions)
{
	Options = NewOptions;
	SendControlState();
}

// ============================================================================
// Slider
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Slider::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	Obj->SetNumberField(TEXT("value"), Value);
	Obj->SetNumberField(TEXT("min"), MinValue);
	Obj->SetNumberField(TEXT("max"), MaxValue);
	Obj->SetNumberField(TEXT("step"), Step);
	return Obj;
}

void UTclCustomControlComponent_Slider::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const float NewValue = FCString::Atof(*Event->Value);
	if (!ShouldReceiveValue_Float(NewValue)) return;

	const float OldValue = Value;
	Value = NewValue;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Slider::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const float NewValue = FCString::Atof(*Event->Value);
	if (!ShouldReceiveValue_Float(NewValue)) return;

	Value = NewValue;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Slider::SetValue(float NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Switch
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Switch::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	Obj->SetBoolField(TEXT("value"), Value);
	return Obj;
}

void UTclCustomControlComponent_Switch::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const bool NewValue = Event->Value.ToBool();
	if (!ShouldReceiveValue_Bool(NewValue)) return;

	const bool OldValue = Value;
	Value = NewValue;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Switch::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	const bool NewValue = Event->Value.ToBool();
	if (!ShouldReceiveValue_Bool(NewValue)) return;

	Value = NewValue;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Switch::SetValue(bool NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Text
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Text::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	Obj->SetStringField(TEXT("value"), Value);
	if (!Placeholder.IsEmpty())
		Obj->SetStringField(TEXT("placeholder"), Placeholder);
	return Obj;
}

void UTclCustomControlComponent_Text::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;
	if (!ShouldReceiveValue_String(Event->Value)) return;

	const FString OldValue = Value;
	Value = Event->Value;
	OnValueChangedFromApp.Broadcast(Value, OldValue);
}

void UTclCustomControlComponent_Text::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;
	if (!ShouldReceiveValue_String(Event->Value)) return;

	Value = Event->Value;
	OnValueCommittedFromApp.Broadcast(Value);
}

void UTclCustomControlComponent_Text::SetValue(const FString& NewValue)
{
	Value = NewValue;
	SendControlState();
}

void UTclCustomControlComponent_Text::SetPlaceholder(const FString& NewPlaceholder)
{
	Placeholder = NewPlaceholder;
	SendControlState();
}

// ============================================================================
// Vector2
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Vector2::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	TArray<TSharedPtr<FJsonValue>> Arr;
	Arr.Add(MakeShared<FJsonValueNumber>(Value.X));
	Arr.Add(MakeShared<FJsonValueNumber>(Value.Y));
	Obj->SetArrayField(TEXT("value"), Arr);
	return Obj;
}

void UTclCustomControlComponent_Vector2::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	// Parse [X, Y] array from JSON string
	TArray<TSharedPtr<FJsonValue>> Arr;
	const auto Reader = TJsonReaderFactory<>::Create(Event->Value);
	if (FJsonSerializer::Deserialize(Reader, Arr) && Arr.Num() >= 2)
	{
		const FVector2D NewValue(Arr[0]->AsNumber(), Arr[1]->AsNumber());
		if (!ShouldReceiveValue_Vector2(NewValue)) return;

		const FVector2D OldValue = Value;
		Value = NewValue;
		OnValueChangedFromApp.Broadcast(Value, OldValue);
	}
}

void UTclCustomControlComponent_Vector2::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	const auto Reader = TJsonReaderFactory<>::Create(Event->Value);
	if (FJsonSerializer::Deserialize(Reader, Arr) && Arr.Num() >= 2)
	{
		const FVector2D NewValue(Arr[0]->AsNumber(), Arr[1]->AsNumber());
		if (!ShouldReceiveValue_Vector2(NewValue)) return;

		Value = NewValue;
		OnValueCommittedFromApp.Broadcast(Value);
	}
}

void UTclCustomControlComponent_Vector2::SetValue(FVector2D NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Vector3
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Vector3::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	TArray<TSharedPtr<FJsonValue>> Arr;
	Arr.Add(MakeShared<FJsonValueNumber>(Value.X));
	Arr.Add(MakeShared<FJsonValueNumber>(Value.Y));
	Arr.Add(MakeShared<FJsonValueNumber>(Value.Z));
	Obj->SetArrayField(TEXT("value"), Arr);
	return Obj;
}

void UTclCustomControlComponent_Vector3::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	const auto Reader = TJsonReaderFactory<>::Create(Event->Value);
	if (FJsonSerializer::Deserialize(Reader, Arr) && Arr.Num() >= 3)
	{
		const FVector NewValue(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
		if (!ShouldReceiveValue_Vector3(NewValue)) return;

		const FVector OldValue = Value;
		Value = NewValue;
		OnValueChangedFromApp.Broadcast(Value, OldValue);
	}
}

void UTclCustomControlComponent_Vector3::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	const auto Reader = TJsonReaderFactory<>::Create(Event->Value);
	if (FJsonSerializer::Deserialize(Reader, Arr) && Arr.Num() >= 3)
	{
		const FVector NewValue(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
		if (!ShouldReceiveValue_Vector3(NewValue)) return;

		Value = NewValue;
		OnValueCommittedFromApp.Broadcast(Value);
	}
}

void UTclCustomControlComponent_Vector3::SetValue(FVector NewValue)
{
	Value = NewValue;
	SendControlState();
}

// ============================================================================
// Vector4
// ============================================================================

TSharedRef<FJsonObject> UTclCustomControlComponent_Vector4::SerializeToJson() const
{
	auto Obj = Super::SerializeToJson();
	TArray<TSharedPtr<FJsonValue>> Arr;
	Arr.Add(MakeShared<FJsonValueNumber>(Value.X));
	Arr.Add(MakeShared<FJsonValueNumber>(Value.Y));
	Arr.Add(MakeShared<FJsonValueNumber>(Value.Z));
	Arr.Add(MakeShared<FJsonValueNumber>(Value.W));
	Obj->SetArrayField(TEXT("value"), Arr);
	return Obj;
}

void UTclCustomControlComponent_Vector4::ReceiveValueChange(UTclCustomControlEvent_ValueChange* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	const auto Reader = TJsonReaderFactory<>::Create(Event->Value);
	if (FJsonSerializer::Deserialize(Reader, Arr) && Arr.Num() >= 4)
	{
		const FVector4 NewValue(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber(), Arr[3]->AsNumber());
		if (!ShouldReceiveValue_Vector4(NewValue)) return;

		const FVector4 OldValue = Value;
		Value = NewValue;
		OnValueChangedFromApp.Broadcast(Value, OldValue);
	}
}

void UTclCustomControlComponent_Vector4::ReceiveValueCommit(UTclCustomControlEvent_ValueCommit* Event)
{
	if (!Event || !ShouldReceiveValue(Event->Value)) return;

	TArray<TSharedPtr<FJsonValue>> Arr;
	const auto Reader = TJsonReaderFactory<>::Create(Event->Value);
	if (FJsonSerializer::Deserialize(Reader, Arr) && Arr.Num() >= 4)
	{
		const FVector4 NewValue(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber(), Arr[3]->AsNumber());
		if (!ShouldReceiveValue_Vector4(NewValue)) return;

		Value = NewValue;
		OnValueCommittedFromApp.Broadcast(Value);
	}
}

void UTclCustomControlComponent_Vector4::SetValue(FVector4 NewValue)
{
	Value = NewValue;
	SendControlState();
}
