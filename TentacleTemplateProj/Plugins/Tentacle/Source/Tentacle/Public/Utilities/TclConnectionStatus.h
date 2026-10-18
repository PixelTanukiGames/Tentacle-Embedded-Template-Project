// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "TclConnectionStatus.generated.h"

UENUM(BlueprintType)
enum class ETclConnectionStatus : uint8
{
	Disconnected,
	Connected,
};
