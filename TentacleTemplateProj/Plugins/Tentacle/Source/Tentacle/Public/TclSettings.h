// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TclSettings.generated.h"

/**
 * Persisted Tentacle settings via USaveGame.
 * Replaces TentacleSettings.ts.
 *
 * Save slot: "TentacleSettings", user index 0.
 */
UCLASS()
class TENTACLE_API UTclSettings : public USaveGame
{
	GENERATED_BODY()

public:
	/** Load settings from the save slot. Creates a new instance if none exists. */
	static UTclSettings*
	Load();

	/** Save current settings to the save slot. */
	void
	Save();

private:
	inline static const FString SlotName = TEXT("TentacleSettings");
	inline static constexpr int32 UserIndex = 0;
};
