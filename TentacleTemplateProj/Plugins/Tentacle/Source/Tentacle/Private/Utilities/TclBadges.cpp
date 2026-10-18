// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclBadges.h"
#include "Sdk/TclKickBadge.h"
#include "Sdk/TclTwitchChatBadge.h"

// Named rather than anonymous, because a unity build merges this file with
// others that have helpers of their own.
namespace TclBadges
{
	/** Whether an id is one of these, under the caller's case rule. */
	static bool IsOneOf(const FString& Id, const TArray<FString>& Wanted, bool bCaseSensitive)
	{
		const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
		for (const FString& Candidate : Wanted)
		{
			if (Id.Equals(Candidate, SearchCase)) return true;
		}
		return false;
	}

	const TArray<FString>& TwitchBroadcaster()
	{
		static const TArray<FString> SetIds = { TEXT("broadcaster") };
		return SetIds;
	}

	const TArray<FString>& TwitchModerator()
	{
		static const TArray<FString> SetIds = { TEXT("moderator"), TEXT("lead_moderator") };
		return SetIds;
	}

	const TArray<FString>& TwitchVip()
	{
		static const TArray<FString> SetIds = { TEXT("vip") };
		return SetIds;
	}

	const TArray<FString>& TwitchSubscriber()
	{
		static const TArray<FString> SetIds = { TEXT("subscriber"), TEXT("founder") };
		return SetIds;
	}

	const TArray<FString>& KickBroadcaster()
	{
		static const TArray<FString> Types = { TEXT("broadcaster") };
		return Types;
	}

	const TArray<FString>& KickModerator()
	{
		static const TArray<FString> Types = { TEXT("moderator") };
		return Types;
	}

	const TArray<FString>& KickVip()
	{
		static const TArray<FString> Types = { TEXT("vip") };
		return Types;
	}

	const TArray<FString>& KickSubscriber()
	{
		static const TArray<FString> Types = { TEXT("subscriber"), TEXT("founder") };
		return Types;
	}

	bool AnyTwitch(const TArray<UTclTwitchChatBadge*>& Badges, const TArray<FString>& SetIds, bool bCaseSensitive)
	{
		for (const UTclTwitchChatBadge* Badge : Badges)
		{
			if (Badge && IsOneOf(Badge->SetId, SetIds, bCaseSensitive)) return true;
		}
		return false;
	}

	bool AnyKick(const TArray<UTclKickBadge*>& Badges, const TArray<FString>& Types, bool bCaseSensitive)
	{
		for (const UTclKickBadge* Badge : Badges)
		{
			if (Badge && IsOneOf(Badge->Type, Types, bCaseSensitive)) return true;
		}
		return false;
	}
}
