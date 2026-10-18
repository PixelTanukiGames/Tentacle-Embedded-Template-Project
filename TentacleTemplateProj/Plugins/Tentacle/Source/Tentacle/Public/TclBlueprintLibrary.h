// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TclBlueprintLibrary.generated.h"

class ATentacle;
class UTentacle_Stream;
class UTentacle_Twitch;
class UTentacle_TwitchExtras;
class UTentacle_Kick;
class UTentacle_Obs;
class UTentacle_ViewerProperties;
class UTentacle_CustomEvents;
class UTentacle_CustomControls;
class UTentacle_Viewers;
class UTentacle_Bot;
class UTentacle_Timers;
class UTentacle_Connections;

/**
 * Blueprint function library for accessing Tentacle subsystems.
 */
UCLASS()
class TENTACLE_API UTclBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static ATentacle*
	GetTentacle(const UObject* WorldContextObject);

	/**
	 * Deprecated, like the Stream component it returns. Cross-platform stream
	 * events and chat live on the Tentacle actor (Get Tentacle), and each
	 * platform's own on Get Tentacle Twitch and Get Tentacle Kick.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Deprecated", meta=(CompactNodeTitle="Tentacle Stream", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject", DeprecatedFunction, DeprecationMessage="Use Get Tentacle for cross-platform stream events and chat, and Get Tentacle Twitch or Get Tentacle Kick for each platform's own."))
	static UTentacle_Stream*
	GetTentacleStream(const UObject* WorldContextObject);

	/** The Tentacle actor's Twitch component: every Twitch event and call, or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Twitch", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Twitch*
	GetTentacleTwitch(const UObject* WorldContextObject);

	/** What Tentacle adds to Twitch (the incoming raid window), or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Twitch Extras", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_TwitchExtras*
	GetTentacleTwitchExtras(const UObject* WorldContextObject);

	/** The Tentacle actor's Kick component: every Kick event and call, or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Kick", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Kick*
	GetTentacleKick(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle OBS", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Obs*
	GetTentacleObs(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Viewer Properties", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_ViewerProperties*
	GetTentacleViewerProperties(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Custom Events", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_CustomEvents*
	GetTentacleCustomEvents(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Custom Controls", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_CustomControls*
	GetTentacleCustomControls(const UObject* WorldContextObject);

	/** The Tentacle actor's viewer cache, or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Viewers", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Viewers*
	GetTentacleViewers(const UObject* WorldContextObject);

	/** The Tentacle actor's chat bot, or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Bot", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Bot*
	GetTentacleBot(const UObject* WorldContextObject);

	/** The Tentacle actor's timers, or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Timers", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Timers*
	GetTentacleTimers(const UObject* WorldContextObject);

	/** The Tentacle actor's platform connections, for signing in from the game, or null when the level has no Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle", meta=(CompactNodeTitle="Tentacle Connections", WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static UTentacle_Connections*
	GetTentacleConnections(const UObject* WorldContextObject);
};
