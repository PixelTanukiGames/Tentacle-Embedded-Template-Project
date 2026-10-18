// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

class UTclKickBadge;
class UTclTwitchChatBadge;

/**
 * The badge sets that stand for each role, and the lookups UTclViewer and
 * UTclChatMessageUtils run over a badge list, so the viewer's roles and a chat
 * message's roles are read the same way.
 */
namespace TclBadges
{
	/** Twitch's broadcaster badge set. */
	const TArray<FString>& TwitchBroadcaster();

	/** Twitch's moderator badge sets. A Lead Moderator wears lead_moderator in place of moderator, unless they chose the plain one. */
	const TArray<FString>& TwitchModerator();

	/** Twitch's VIP badge set. */
	const TArray<FString>& TwitchVip();

	/** Twitch's subscriber badge sets: a founder is one of the first. */
	const TArray<FString>& TwitchSubscriber();

	/** Kick's broadcaster badge type. */
	const TArray<FString>& KickBroadcaster();

	/** Kick's moderator badge type. */
	const TArray<FString>& KickModerator();

	/** Kick's VIP badge type. */
	const TArray<FString>& KickVip();

	/** Kick's subscriber badge types: a founder is one of the first. */
	const TArray<FString>& KickSubscriber();

	/** Whether a Twitch badge of one of these sets is in the list. */
	bool AnyTwitch(const TArray<UTclTwitchChatBadge*>& Badges, const TArray<FString>& SetIds, bool bCaseSensitive = true);

	/** Whether a Kick badge of one of these types is in the list. */
	bool AnyKick(const TArray<UTclKickBadge*>& Badges, const TArray<FString>& Types, bool bCaseSensitive = true);
}
