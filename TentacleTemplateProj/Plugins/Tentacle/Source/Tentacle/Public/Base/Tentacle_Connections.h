// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sdk/TclStreamPlatform.h"
#include "Tentacle_Connections.generated.h"

class UTclPlatformConnection;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;
class UTclMsg_ServerToClient_ConnectionsStatus;
class UTclMsg_ServerToClient_ConnectionsPrompt;
class UTclMsg_ServerToClient_ConnectionsResult;

// Unity's Tentacle_Connections, whose names this keeps; they follow the
// Connections_* messages. Only Tentacle embedded in the game answers them.

/**
 * Connecting the streaming platforms from the game, which signs the player in,
 * and each platform's connection as Tentacle reports it.
 *
 * This is for Tentacle embedded in the game (the Tentacle actor's Editor Mode
 * or Build Mode set to Embedded), where the game signs the player in itself.
 * The Tentacle app has its own sign-in, so with the app none of these are
 * answered.
 *
 * The Sign In node does all of signing in at once. For finer control, Connect
 * starts it: On Connect Prompt says what to show (open the page in a browser,
 * and show the code in case it asks), and On Connect Result how it ended. The
 * account stays signed in across runs until Disconnect.
 */
UCLASS()
class TENTACLE_API UTentacle_Connections : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionChanged, const UTclPlatformConnection*, Connection);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectPrompt, const UTclPlatformConnectionPrompt*, Prompt);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectResult, const UTclPlatformConnectionResult*, Result);

public:
	/**
	 * Fires when a platform's connection changes, and once for each platform as
	 * a session opens. Get Connection already returns the new connection when
	 * it fires.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Connections")
	FOnConnectionChanged OnConnectionChanged;

	/**
	 * Fires when connecting needs the player: open the prompt's Url in a
	 * browser, and show its User Code (when there is one) in case the page asks
	 * for it.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Connections")
	FOnConnectPrompt OnConnectPrompt;

	/** Fires when connecting ends: connected, denied, expired, cancelled, or failed. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Connections")
	FOnConnectResult OnConnectResult;

	/** A platform's connection as Tentacle last reported it, or null before it has reported one. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Connections")
	UTclPlatformConnection* GetConnection(ETclStreamPlatform Platform, bool& Found) const;

	/**
	 * Start connecting Platform, which signs the player in, replacing any
	 * attempt already in progress. On Connect Prompt follows with what to show
	 * them.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void Connect(ETclStreamPlatform Platform);

	/** Abandon the connection attempt in progress on Platform. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void CancelConnect(ETclStreamPlatform Platform);

	/** Disconnect Platform, signing its account out and forgetting it. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void Disconnect(ETclStreamPlatform Platform);

	void HandleMsg_Status(UTclMsg_ServerToClient_ConnectionsStatus* Msg);
	void HandleMsg_Prompt(UTclMsg_ServerToClient_ConnectionsPrompt* Msg);
	void HandleMsg_Result(UTclMsg_ServerToClient_ConnectionsResult* Msg);

private:
	/** Each platform's connection as last reported. */
	UPROPERTY()
	TMap<ETclStreamPlatform, UTclPlatformConnection*> Connections;

	/** Send one Connections_* message naming Platform. */
	void SendFor(const TCHAR* MessageType, ETclStreamPlatform Platform);
};
