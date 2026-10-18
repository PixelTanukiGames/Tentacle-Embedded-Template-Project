// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "TclTwitchBitLevel.generated.h"

/**
 * The type of a Twitch bit level.
 */
UENUM(BlueprintType, Category="Tentacle")
enum class ETclTwitchBitLevelType : uint8
{
	One,
	OneHundred,
	OneThousand,
	FiveThousand,
	TenThousand,
};

USTRUCT(BlueprintType)
struct TENTACLE_API FTclTwitchBitLevel : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(Category="Tentacle", BlueprintReadOnly)
	ETclTwitchBitLevelType
	Type = ETclTwitchBitLevelType::One;

	UPROPERTY(Category="Tentacle", BlueprintReadOnly)
	int32
	Amount = 1;

	UPROPERTY(Category="Tentacle", BlueprintReadOnly)
	UStaticMesh*
	StaticMesh = nullptr;
};
