// © 2025 Eldritch One. All rights reserved.

#include "Base/TclDefinitionChangeInternal.h"
#include "Utilities/TclCommon.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void
FTclDefinitionChange::Change(
	UTentacleHttpClient* Http, FCall Call, const TCHAR* Procedure, const TSharedRef<FJsonObject>& Input, const FTclOnDefinitionComplete& OnComplete)
{
	Change(Http, Call, Procedure, Input, [OnComplete](ETclDefinitionResult Result) { Finish(OnComplete, Result); });
}

void
FTclDefinitionChange::Change(
	UTentacleHttpClient* Http, FCall Call, const TCHAR* Procedure, const TSharedRef<FJsonObject>& Input, TFunction<void(ETclDefinitionResult)> OnResult)
{
	if (!Http)
	{
		tclWarnF("Tentacle: %s failed: there is no connection to Tentacle yet", Procedure);
		OnResult(ETclDefinitionResult::Failed);
		return;
	}

	FString InputJson;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&InputJson);
	FJsonSerializer::Serialize(Input, Writer);

	const FString ProcedureName = Procedure;
	(Http->*Call)(
		InputJson,
		UTentacleHttpClient::FOnSuccess::CreateLambda([OnResult](const FString&)
		{
			OnResult(ETclDefinitionResult::Success);
		}),
		UTentacleHttpClient::FOnError::CreateLambda([OnResult, ProcedureName](int32 StatusCode, const FString& Error)
		{
			const ETclDefinitionResult Result = ResultForStatus(StatusCode);
			if (Result == ETclDefinitionResult::Failed)
				tclWarnF("Tentacle: %s failed (%d): %s", *ProcedureName, StatusCode, *Error.Left(500));
			OnResult(Result);
		}));
}

ETclDefinitionResult
FTclDefinitionChange::ResultForStatus(int32 StatusCode)
{
	switch (StatusCode)
	{
	case 404: return ETclDefinitionResult::NotFound;
	case 409: return ETclDefinitionResult::NameTaken;
	default: return ETclDefinitionResult::Failed;
	}
}

void
FTclDefinitionChange::Finish(const FTclOnDefinitionComplete& OnComplete, ETclDefinitionResult Result)
{
	OnComplete.ExecuteIfBound(Result);
}
