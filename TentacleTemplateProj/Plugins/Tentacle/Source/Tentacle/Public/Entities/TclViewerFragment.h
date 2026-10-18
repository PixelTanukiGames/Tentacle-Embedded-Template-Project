// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "TclViewerFragment.generated.h"

/**
 * Mass entity fragment for viewers.
 */
USTRUCT()
struct TENTACLE_API FTclViewerFragment : public FMassFragment
{
	GENERATED_BODY()

	/** The viewer ID. */
	UPROPERTY()
	FString
	ViewerId;

	/** The viewer's color */
	UPROPERTY()
	FLinearColor
	Color = FLinearColor::White;
};
