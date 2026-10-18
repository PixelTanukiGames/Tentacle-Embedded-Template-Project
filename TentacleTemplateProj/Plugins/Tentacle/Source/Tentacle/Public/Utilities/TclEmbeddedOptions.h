// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

// Unity's TentacleEmbeddedOptions, and the core's CoreConfig, field for field.

/**
 * How Tentacle embedded in the game starts. The Tentacle actor builds one from
 * its Embedded settings; C++ only.
 */
struct TENTACLE_API FTclEmbeddedOptions
{
	/**
	 * The folder Tentacle keeps its database and the player's signed-in
	 * accounts in. Empty keeps nothing between runs, so the player signs in
	 * again every time.
	 */
	FString DataDirectory;

	/** The client ID of your own Twitch app to sign in with; empty uses Tentacle's. */
	FString TwitchClientId;

	/**
	 * The text of each config file the game ships, as the Tentacle app exports
	 * them (Timers, Custom Events, Viewer Properties, or Bot), applied when
	 * Tentacle starts. The Tentacle actor reads them from its ConfigDirectory.
	 */
	TArray<FString> ConfigFiles;

	/** These options as the JSON the core starts from. */
	FString ToConfigJson() const;

	bool operator==(const FTclEmbeddedOptions& Other) const
	{
		return DataDirectory == Other.DataDirectory
			&& TwitchClientId == Other.TwitchClientId
			&& ConfigFiles == Other.ConfigFiles;
	}

	bool operator!=(const FTclEmbeddedOptions& Other) const { return !(*this == Other); }
};
