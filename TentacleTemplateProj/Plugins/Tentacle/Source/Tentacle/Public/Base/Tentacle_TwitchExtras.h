// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Tentacle_TwitchExtras.generated.h"

/**
 * What Tentacle adds to Twitch: things Twitch itself does not have, kept apart
 * so that everything directly on the Twitch component is a Twitch event or call
 * you can look up in Twitch's own documentation. Reached as the Twitch
 * component's Extras.
 *
 * Today that is the incoming raid window. Twitch reports a raid on this channel
 * (On Channel Raid) but never reports it over, so Tentacle keeps a window open
 * from the raid until the game closes it.
 */
UCLASS(BlueprintType)
class TENTACLE_API UTentacle_TwitchExtras : public UObject
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIncomingRaidEnded);

public:
	/**
	 * Fires when End Incoming Raid closes the incoming raid window. Twitch sends no
	 * event when a raid is over, so the game decides when it is.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch|Extras")
	FOnIncomingRaidEnded OnIncomingRaidEnded;

	/** True from a raid on this channel (the Twitch component's On Channel Raid) until End Incoming Raid. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Twitch|Extras")
	bool bIsIncomingRaidOngoing = false;

	/**
	 * Close the incoming raid window: Is Incoming Raid Ongoing goes false and On
	 * Incoming Raid Ended fires. Twitch sends no event when a raid is over, so the
	 * game decides when it is.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Twitch|Extras")
	void EndIncomingRaid();

	/** Open the incoming raid window. Called by the Twitch component when a raid arrives. C++ only. */
	void BeginIncomingRaid();
};
