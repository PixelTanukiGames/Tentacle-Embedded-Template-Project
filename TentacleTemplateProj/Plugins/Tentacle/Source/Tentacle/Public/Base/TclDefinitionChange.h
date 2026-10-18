// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Sdk/TclDefinitionResult.h"
#include "TclDefinitionChange.generated.h"

// UDELEGATE is what makes UBT hand this header to UHT at all: it looks for a
// reflection marker, and a dynamic delegate on its own carries none.

/**
 * Runs once when a change to the streamer's definitions is done, such as
 * creating or starting a timer, with what it came to: Success, Not Found when
 * nothing has the name it named, Name Taken when another already has the name
 * it asked for, or Failed when Tentacle is not running or refused the change
 * (the reason is logged).
 */
UDELEGATE()
DECLARE_DYNAMIC_DELEGATE_OneParam(FTclOnDefinitionComplete, ETclDefinitionResult, Result);
