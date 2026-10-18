// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

/**
 * How long to wait before each reconnect attempt to the sidecar.
 *
 * Exponential backoff with a ceiling and jitter. Tentacle not running is the
 * ordinary case (the game is often started first), so the first retries are
 * quick and the ceiling keeps an idle game from hammering the port forever.
 *
 * Features:
 * - Pure: the delay is a function of the attempt number and a caller-supplied
 *   jitter sample in [0, 1], so it is testable without waiting.
 * - Degenerate settings are clamped rather than trusted: a multiplier below 1
 *   or a ceiling below the initial delay cannot make the delay shrink or the
 *   loop spin.
 */
struct TENTACLE_API FTclReconnectPolicy
{
	/** Delay before the first retry, in seconds. */
	double InitialDelaySeconds = 1.0;

	/** Ceiling the delay grows to, in seconds. */
	double MaxDelaySeconds = 30.0;

	/** Growth factor between attempts. */
	double Multiplier = 2.0;

	/** Fraction of the delay randomised in either direction, so a fleet of instances does not retry in lockstep. */
	double JitterFraction = 0.25;

	/**
	 * Delay before retry number Attempt (1-based), with JitterSample in [0, 1]
	 * selecting where in the jitter band it lands. Never negative, never above
	 * the ceiling.
	 */
	double
	DelayForAttempt(int32 Attempt, double JitterSample) const;
};
