// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclEngineIdentity.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool
FTclEngineIdentity::IsValid(FString& OutProblem) const
{
	if (Clamp(Engine).IsEmpty())
	{
		OutProblem = TEXT("the engine name is empty");
		return false;
	}
	if (Clamp(InstanceId).IsEmpty())
	{
		OutProblem = TEXT("the instance id is empty");
		return false;
	}
	OutProblem.Empty();
	return true;
}

TSharedRef<FJsonObject>
FTclEngineIdentity::ToHelloPayload() const
{
	auto Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("engine"), Clamp(Engine));
	Payload->SetStringField(TEXT("engineVersion"), Clamp(EngineVersion));
	Payload->SetStringField(TEXT("pluginVersion"), Clamp(PluginVersion));
	Payload->SetStringField(TEXT("instanceId"), Clamp(InstanceId));
	Payload->SetStringField(TEXT("label"), Clamp(Label));
	// Never clamped: a password with a space at either end is still that password.
	if (!Password.IsEmpty())
	{
		Payload->SetStringField(TEXT("password"), Password);
	}
	return Payload;
}

FString
FTclEngineIdentity::ToHelloMessage() const
{
	auto Frame = MakeShared<FJsonObject>();
	Frame->SetStringField(TEXT("type"), TEXT("Session_Hello"));
	Frame->SetObjectField(TEXT("data"), ToHelloPayload());

	FString Json;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Frame, Writer);
	return Json;
}

FString
FTclEngineIdentity::Clamp(const FString& Value)
{
	const FString Trimmed = Value.TrimStartAndEnd();
	return Trimmed.Len() <= MaxFieldLength ? Trimmed : Trimmed.Left(MaxFieldLength);
}
