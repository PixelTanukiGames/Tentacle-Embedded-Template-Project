// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "TclChatSendTarget.generated.h"

/**
 * Which stream platform a chat message is sent to.
 *
 * Deliberately separate from ETclStreamPlatform, which mirrors the wire enum
 * and therefore has no "every platform" member. AllConnected is the default the
 * chat send nodes use, and omits the platform on the wire so the app fans the
 * message out to every connected platform.
 */
UENUM(BlueprintType, Category="Tentacle")
enum class ETclChatSendTarget : uint8
{
	AllConnected UMETA(DisplayName="All Connected"),
	Twitch       UMETA(DisplayName="Twitch"),
	Kick         UMETA(DisplayName="Kick"),
};
