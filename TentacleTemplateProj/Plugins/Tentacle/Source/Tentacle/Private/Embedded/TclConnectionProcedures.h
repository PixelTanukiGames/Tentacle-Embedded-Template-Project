// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Sdk/TclHttpClient.h"

// Unity's TentacleConnectionProcedures. The embedded core has no HTTP server, so
// in Embedded mode the HTTP client hands every call here (its procedure
// transport), and each goes over the session as an Rpc_Request whose
// Rpc_Response carries exactly the status and body the app's /api/trpc would
// have answered. Everything above the HTTP client is unchanged.

/** The plugin's HTTP procedures, carried over the connection as Rpc_Request and Rpc_Response frames. */
class FTclConnectionProcedures
{
public:
	/** Send hands a frame to the open session, answering false when none is open. */
	explicit FTclConnectionProcedures(TFunction<bool(const FString& Frame)> InSend);

	/**
	 * Send one procedure call, and complete it with what HTTP would have given:
	 * OnSuccess with the body for a status below 400, otherwise OnError with the
	 * status and body. With no session open it fails at once with status 0, as
	 * an unreachable server does.
	 */
	void Call(const FString& Procedure, bool bIsQuery, const FString& InputJson,
		const UTentacleHttpClient::FOnSuccess& OnSuccess, const UTentacleHttpClient::FOnError& OnError);

	/** Complete the call an Rpc_Response answers. One that answers no waiting call is ignored. */
	void HandleResponse(const FString& Id, int32 Status, const FString& Body);

	/** Fail every call still waiting with status 0, as a dropped connection fails an HTTP request. */
	void FailAll(const FString& Why);

	/** How many calls are waiting for their answer. */
	int32 NumPending() const { return Pending.Num(); }

private:
	struct FPendingCall
	{
		FString Procedure;
		UTentacleHttpClient::FOnSuccess OnSuccess;
		UTentacleHttpClient::FOnError OnError;
	};

	TFunction<bool(const FString&)> Send;
	uint64 NextId = 1;
	TMap<FString, FPendingCall> Pending;
};
