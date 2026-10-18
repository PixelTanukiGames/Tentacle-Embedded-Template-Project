// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "TclTimerSettings.generated.h"

// Unity's TimerSettings, Godot's TclTimerSettings, and tentacle-web's
// TimerSettings are this struct field for field, with the same defaults, so the
// same settings make the same timer in every engine.

/**
 * How a timer runs, for Create Timer and Update Timer on the Timers component.
 * Every setting is sent, so one left alone takes its default.
 */
USTRUCT(BlueprintType)
struct TENTACLE_API FTclTimerSettings
{
	GENERATED_BODY()

	/** The timer's name, unique among the timers. On an update, empty keeps the timer's own name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers")
	FString Name;

	/** Whether it starts its countdown again after it fires, rather than stopping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers")
	bool bRepeating = true;

	/** Whether it counts down only while the stream is live on a connected platform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers")
	bool bOnlyWhileLive = true;

	/**
	 * The shortest countdown, in seconds, from 1 to 86400. Each countdown's
	 * length is drawn from the range; equal ends make it fixed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers", meta=(ClampMin="1", ClampMax="86400"))
	int32 IntervalMinSeconds = 600;

	/** The longest countdown, in seconds, from 1 to 86400. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers", meta=(ClampMin="1", ClampMax="86400"))
	int32 IntervalMaxSeconds = 600;

	/** How many viewer chat messages must come between two fires, up to 1000. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Timers", meta=(ClampMin="0", ClampMax="1000"))
	int32 MinChatMessages = 0;

	/** Every setting as Create Timer and Update Timer send it, the name only when it is not empty. */
	TSharedRef<FJsonObject> ToInput() const;
};
