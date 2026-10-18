// © 2025 Eldritch One. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "TclPlatformRequestAction.generated.h"

class FJsonValue;

// A Blueprint async node takes its data pins from its first delegate, which is
// a request's On Success, and fills each exit's pins from the delegate behind it
// by name. So every On Success carries On Failure's StatusCode and ErrorMessage
// too: without them, Blueprint would never show why a request failed.

/**
 * Called when a Twitch or Kick API request that responds with no body succeeds,
 * with StatusCode 200 and an empty ErrorMessage.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTclPlatformRequestSucceeded, int32, StatusCode, const FString&, ErrorMessage);

/**
 * Called when a Twitch or Kick API request fails. StatusCode is the status
 * Tentacle responded with: 400 for a request Tentacle refused or a platform that
 * is not connected, 502 for one the platform refused or did not respond to, and 0
 * when Tentacle itself was not reached. ErrorMessage says why, in words safe to
 * show a player.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTclPlatformRequestFailed, int32, StatusCode, const FString&, ErrorMessage);

/**
 * The base of every Twitch and Kick API request node: one Blueprint async action
 * per endpoint in each platform's API reference (Twitch's Get Streams, Kick's
 * Patch Channels), named after its title there, with the endpoint's own fields
 * in its request and its response.
 *
 * Each sends its request to Tentacle as the app's procedure of the same name,
 * over HTTP or, embedded, over the session, and fires On Success with the
 * platform's response or On Failure with the status and a message. Both carry
 * Status Code and Error Message, which read 200 and empty on success. A request
 * never fails silently: no Tentacle actor in the level, Tentacle not running, a
 * platform that is not connected, and a refusal all reach On Failure.
 */
UCLASS(Abstract)
class TENTACLE_API UTclPlatformRequestAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Fired when the request fails, with the status Tentacle responded with and why. */
	UPROPERTY(BlueprintAssignable)
	FTclPlatformRequestFailed OnFailure;

	virtual void Activate() override;

	/**
	 * A copy of Request, a request's fields, with one optional field's flag set:
	 * whether that field is sent. Blueprint's request node calls it for a field
	 * set to Decide at Runtime; from C++, set the field's flag directly.
	 *
	 * @param Request The request's fields.
	 * @param Toggle The name of the field's flag, such as bHasReason.
	 * @param bSent Whether the field is sent.
	 * @param Result Request with the flag set.
	 */
	UFUNCTION(BlueprintPure, CustomThunk, meta = (BlueprintInternalUseOnly = "true", CustomStructureParam = "Request,Result"))
	static void WithFieldSent(const int32& Request, FName Toggle, bool bSent, int32& Result);
	DECLARE_FUNCTION(execWithFieldSent);

protected:
	/** The status On Success carries: Tentacle responded to the request. */
	static constexpr int32 AnsweredStatusCode = 200;

	/**
	 * Sets the request up, and keeps the node alive until it gets a response.
	 *
	 * @param WorldContextObject Anything in the world whose Tentacle actor sends it.
	 * @param InProcedure The app's procedure (`twitch.getStreams`).
	 * @param bInIsQuery True for a read, sent as a query; false for a mutation.
	 * @param InInputJson The request's fields as a JSON object's text.
	 */
	void Configure(UObject* WorldContextObject, const FString& InProcedure, bool bInIsQuery, const FString& InInputJson);

	/** Hands a subclass the procedure's result data, null when it responded with none. */
	virtual void HandleSuccess(const TSharedPtr<FJsonValue>& Data);

private:
	/** Fires On Failure and lets the node go. */
	void Fail(int32 StatusCode, const FString& Message);

	TWeakObjectPtr<UObject> WorldContext;
	FString Procedure;
	bool bIsQuery = true;
	FString InputJson;
};
