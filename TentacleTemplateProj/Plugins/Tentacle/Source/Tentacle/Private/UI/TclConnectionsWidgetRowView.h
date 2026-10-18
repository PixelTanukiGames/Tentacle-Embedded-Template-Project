// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Sdk/TclStreamPlatform.h"
#include "UI/TclConnectionsWidgetRow.h"
#include "TclConnectionsWidgetRowView.generated.h"

class UButton;
class UTclConnectionsWidget;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;
class UTextBlock;

/**
 * One platform's row in a Tentacle Connections Widget: its parts, and what it
 * last heard. A UObject so its buttons have a function to call.
 */
UCLASS(Transient, NotBlueprintable, HideDropdown)
class UTclConnectionsWidgetRowView : public UObject
{
	GENERATED_BODY()

public:
	ETclStreamPlatform Platform = ETclStreamPlatform::Twitch;
	ETclConnectionsWidgetAction PrimaryAction = ETclConnectionsWidgetAction::None;
	ETclConnectionsWidgetAction SecondaryAction = ETclConnectionsWidgetAction::None;
	double PromptDeadline = 0.0;

	UPROPERTY()
	TObjectPtr<UTclConnectionsWidget> Owner;

	UPROPERTY()
	TObjectPtr<UTextBlock> Status;

	UPROPERTY()
	TObjectPtr<UTextBlock> Detail;

	UPROPERTY()
	TObjectPtr<UButton> Primary;

	UPROPERTY()
	TObjectPtr<UTextBlock> PrimaryLabel;

	UPROPERTY()
	TObjectPtr<UButton> Secondary;

	UPROPERTY()
	TObjectPtr<UTextBlock> SecondaryLabel;

	/** The sign-in in progress, or null. Held here so it outlives the message that carried it. */
	UPROPERTY()
	TObjectPtr<UTclPlatformConnectionPrompt> Prompt;

	/** How the last sign-in ended, or null. */
	UPROPERTY()
	TObjectPtr<UTclPlatformConnectionResult> Result;

	UFUNCTION()
	void HandlePrimaryClicked();

	UFUNCTION()
	void HandleSecondaryClicked();
};
