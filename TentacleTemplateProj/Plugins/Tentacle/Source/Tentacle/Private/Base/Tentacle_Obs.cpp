// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Obs.h"
#include "Base/Tentacle.h"
#include "Base/TclDefinitionChangeInternal.h"
#include "Sdk/TclHttpClient.h"
#include "Utilities/TclCommon.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

void
UTentacle_Obs::SetSourceFilterEnabled(const FString& SourceName, const FString& FilterName, bool bEnabled)
{
	const TSharedRef<FJsonObject> RequestData = MakeShared<FJsonObject>();
	RequestData->SetStringField(TEXT("sourceName"), SourceName);
	RequestData->SetStringField(TEXT("filterName"), FilterName);
	RequestData->SetBoolField(TEXT("filterEnabled"), bEnabled);

	Call(TEXT("SetSourceFilterEnabled"), RequestData);
}

void
UTentacle_Obs::SetCurrentProgramScene(const FString& SceneName)
{
	const TSharedRef<FJsonObject> RequestData = MakeShared<FJsonObject>();
	RequestData->SetStringField(TEXT("sceneName"), SceneName);

	Call(TEXT("SetCurrentProgramScene"), RequestData);
}

void
UTentacle_Obs::CallRequest(const FString& RequestType, const FString& RequestDataJson)
{
	// Refused here rather than sent: Tentacle would drop a malformed request on
	// its side of the socket, where whoever wrote the graph never sees why.
	if (RequestType.IsEmpty())
	{
		tclWarn("Tentacle OBS: Call needs a request type; nothing sent.");
		return;
	}

	TSharedPtr<FJsonObject> RequestData;
	if (!RequestDataJson.TrimStartAndEnd().IsEmpty())
	{
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RequestDataJson);
		if (!FJsonSerializer::Deserialize(Reader, RequestData) || !RequestData.IsValid())
		{
			tclWarnF("Tentacle OBS: Call '%s' was given request data that is not a JSON object; nothing sent. Request data: %s",
				*RequestType, *RequestDataJson.Left(200));
			return;
		}
	}

	Call(RequestType, RequestData);
}

void
UTentacle_Obs::Call(const FString& RequestType, const TSharedPtr<FJsonObject>& RequestData)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("Obs_Call"));

	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetStringField(TEXT("requestType"), RequestType);

	// Tentacle's schema makes requestData optional but not nullable: a request
	// with no fields leaves it out, since an explicit null is refused outright.
	if (RequestData.IsValid())
		DataObj->SetObjectField(TEXT("requestData"), RequestData);

	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);

	Tentacle->SendMsg(JsonString);
}

void
UTentacle_Obs::SetConnectionSettings(const FString& Host, int32 Port, const FString& Password, const FTclOnDefinitionComplete& OnComplete)
{
	const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
	Input->SetStringField(TEXT("host"), Host);
	Input->SetNumberField(TEXT("port"), Port);
	Input->SetStringField(TEXT("password"), Password);

	const ATentacle* Tentacle = GetOwner<ATentacle>();
	FTclDefinitionChange::Change(
		Tentacle ? Tentacle->HttpClient : nullptr,
		&UTentacleHttpClient::SetConnectionSettings,
		TEXT("obs.setConnectionSettings"),
		Input,
		OnComplete);
}
