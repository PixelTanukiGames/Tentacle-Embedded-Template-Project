// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Base/TclDefinitionChange.h"
#include "Dom/JsonObject.h"
#include "Sdk/TclHttpClient.h"

/**
 * Makes one change to the streamer's definitions and reports what it came to,
 * for every component a game changes them from.
 */
// Unity's TentacleClient.ChangeDefinitionAsync, Godot's TentacleDefinitionChange,
// and tentacle-web's changeDefinition.
struct FTclDefinitionChange
{
	/** One of the HTTP client's mutations. */
	using FCall = void (UTentacleHttpClient::*)(const FString&, const UTentacleHttpClient::FOnSuccess&, const UTentacleHttpClient::FOnError&);

	/**
	 * Call one mutation that changes the streamer's definitions, and run
	 * OnComplete once with what it came to. A null Http, which is a Tentacle
	 * actor that has not begun play, fails without a request.
	 */
	static void Change(UTentacleHttpClient* Http, FCall Call, const TCHAR* Procedure, const TSharedRef<FJsonObject>& Input, const FTclOnDefinitionComplete& OnComplete);

	/** As above, handing what the change came to to OnResult instead, for a caller with more to do before it reports. */
	static void Change(UTentacleHttpClient* Http, FCall Call, const TCHAR* Procedure, const TSharedRef<FJsonObject>& Input, TFunction<void(ETclDefinitionResult)> OnResult);

	/**
	 * What a change Tentacle refused comes to: Not Found for a 404, Name Taken
	 * for a 409, and Failed for anything else, including 0 for no answer.
	 */
	static ETclDefinitionResult ResultForStatus(int32 StatusCode);

	/** Run OnComplete with Result, when anything is bound to it. */
	static void Finish(const FTclOnDefinitionComplete& OnComplete, ETclDefinitionResult Result);
};
