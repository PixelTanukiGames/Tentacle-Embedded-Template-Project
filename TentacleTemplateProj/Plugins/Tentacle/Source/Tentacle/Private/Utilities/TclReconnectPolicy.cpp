// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclReconnectPolicy.h"

double
FTclReconnectPolicy::DelayForAttempt(int32 Attempt, double JitterSample) const
{
	const double Initial = InitialDelaySeconds > 0.0 ? InitialDelaySeconds : 0.0;
	const double Ceiling = MaxDelaySeconds > Initial ? MaxDelaySeconds : Initial;
	const double Growth = Multiplier >= 1.0 ? Multiplier : 1.0;
	const int32 Exponent = Attempt > 1 ? Attempt - 1 : 0;

	// Grow in a loop rather than with Pow so a huge attempt count saturates at
	// the ceiling instead of overflowing to infinity first.
	double Base = Initial;
	for (int32 i = 0; i < Exponent && Base < Ceiling; ++i)
		Base *= Growth;
	Base = FMath::Min(Base, Ceiling);

	const double Sample = FMath::Clamp(JitterSample, 0.0, 1.0);
	const double Jitter = Base * FMath::Clamp(JitterFraction, 0.0, 1.0) * (2.0 * Sample - 1.0);
	return FMath::Clamp(Base + Jitter, 0.0, Ceiling);
}
