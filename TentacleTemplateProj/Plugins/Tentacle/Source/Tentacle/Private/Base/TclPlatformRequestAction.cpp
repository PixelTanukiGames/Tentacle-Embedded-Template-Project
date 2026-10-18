// © 2025 Eldritch One. All rights reserved.

#include "Base/TclPlatformRequestAction.h"

#include "Base/Tentacle.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sdk/TclHttpClient.h"
#include "TentacleModule.h"
#include "Utilities/TclCommon.h"

namespace
{
	/** The body Tentacle answered with, as a JSON object, or null when it is not one. */
	TSharedPtr<FJsonObject> ParseObject(const FString& Body)
	{
		TSharedPtr<FJsonObject> Object;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Body);
		if (!FJsonSerializer::Deserialize(Reader, Object))
		{
			return nullptr;
		}
		return Object;
	}
}

void UTclPlatformRequestAction::Configure(UObject* WorldContextObject, const FString& InProcedure, bool bInIsQuery, const FString& InInputJson)
{
	WorldContext = WorldContextObject;
	Procedure = InProcedure;
	bIsQuery = bInIsQuery;
	InputJson = InInputJson;
	RegisterWithGameInstance(WorldContextObject);
}

void UTclPlatformRequestAction::Activate()
{
	ATentacle* Tentacle = WorldContext.IsValid() ? UTclCommon::GetActorOfClass<ATentacle>(WorldContext.Get()) : nullptr;
	if (!IsValid(Tentacle) || !IsValid(Tentacle->HttpClient))
	{
		// The only failure nothing below has logged: the HTTP client and the
		// embedded session's carrier each log the requests they fail.
		const FString Message = TEXT("No Tentacle actor in the level. Have you added a Tentacle actor?");
		UE_LOG(LogTentacle, Warning, TEXT("Tentacle: %s failed: %s"), *Procedure, *Message);
		Fail(0, Message);
		return;
	}

	TWeakObjectPtr<UTclPlatformRequestAction> WeakThis(this);
	const UTentacleHttpClient::FOnSuccess OnAnswer = UTentacleHttpClient::FOnSuccess::CreateLambda([WeakThis](const FString& Body)
	{
		UTclPlatformRequestAction* This = WeakThis.Get();
		if (This == nullptr)
		{
			return;
		}
		TSharedPtr<FJsonValue> Data;
		if (const TSharedPtr<FJsonObject> Envelope = ParseObject(Body))
		{
			const TSharedPtr<FJsonObject>* Result = nullptr;
			if (Envelope->TryGetObjectField(TEXT("result"), Result) && Result != nullptr && (*Result)->HasField(TEXT("data")))
			{
				Data = (*Result)->TryGetField(TEXT("data"));
			}
		}
		This->HandleSuccess(Data);
		This->SetReadyToDestroy();
	});
	const UTentacleHttpClient::FOnError OnError = UTentacleHttpClient::FOnError::CreateLambda([WeakThis](int32 StatusCode, const FString& Body)
	{
		UTclPlatformRequestAction* This = WeakThis.Get();
		if (This == nullptr)
		{
			return;
		}
		// Tentacle's own message rides in tRPC's error envelope; a failure that
		// never reached it carries the transport's words instead.
		FString Message = Body;
		if (const TSharedPtr<FJsonObject> Envelope = ParseObject(Body))
		{
			const TSharedPtr<FJsonObject>* Error = nullptr;
			FString Text;
			if (Envelope->TryGetObjectField(TEXT("error"), Error) && Error != nullptr && (*Error)->TryGetStringField(TEXT("message"), Text))
			{
				Message = Text;
			}
		}
		This->Fail(StatusCode, Message.IsEmpty() ? TEXT("the request failed") : Message);
	});
	Tentacle->HttpClient->CallProcedure(Procedure, bIsQuery, InputJson, OnAnswer, OnError);
}

void UTclPlatformRequestAction::HandleSuccess(const TSharedPtr<FJsonValue>& Data)
{
}

void UTclPlatformRequestAction::WithFieldSent(const int32& Request, FName Toggle, bool bSent, int32& Result)
{
	// A custom thunk: Blueprint calls execWithFieldSent, which reads the
	// request's struct type from the call.
	checkNoEntry();
}

DEFINE_FUNCTION(UTclPlatformRequestAction::execWithFieldSent)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const FStructProperty* RequestProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
	const void* Request = Stack.MostRecentPropertyAddress;
	P_GET_PROPERTY(FNameProperty, Toggle);
	P_GET_UBOOL(bSent);
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const FStructProperty* ResultProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* Result = Stack.MostRecentPropertyAddress;
	P_FINISH;

	P_NATIVE_BEGIN;
	if (RequestProperty != nullptr && ResultProperty != nullptr && RequestProperty->Struct == ResultProperty->Struct && Request != nullptr && Result != nullptr)
	{
		RequestProperty->Struct->CopyScriptStruct(Result, Request);
		if (const FBoolProperty* Flag = FindFProperty<FBoolProperty>(RequestProperty->Struct, Toggle))
		{
			Flag->SetPropertyValue_InContainer(Result, bSent);
		}
	}
	P_NATIVE_END;
}

void UTclPlatformRequestAction::Fail(int32 StatusCode, const FString& Message)
{
	OnFailure.Broadcast(StatusCode, Message);
	SetReadyToDestroy();
}
