// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// NewObject and GetTransientPackage: see TclChatMessageEmojiCountTest.cpp for
// why the package build needs this include where the editor build does not.
#include "UObject/Package.h"
#include "Base/TclViewer.h"
#include "Sdk/TclKickBadge.h"
#include "Sdk/TclStreamEvent_Twitch_ChatMessage.h"
#include "Sdk/TclTwitchChatBadge.h"
#include "Sdk/TclViewerData.h"
#include "Tests/TclTestSupport.h"
#include "Utilities/TclChatMessageUtils.h"

namespace
{
	/** A viewer parsed from the wire, as Tentacle sends one, carrying this platform profile. */
	UTclViewer* ViewerFromJson(const FString& PlatformJson)
	{
		const FString Json = FString::Printf(
			TEXT("{\"id\":\"viewer-1\",\"createdAt\":\"a\",\"updatedAt\":\"b\",\"properties\":{},%s}"),
			*PlatformJson);
		UTclViewer* Viewer = NewObject<UTclViewer>(GetTransientPackage());
		Viewer->Data = UTclViewerData::Parse(Viewer, TclTest::ParseObject(Json));
		return Viewer;
	}

	/** A viewer on Twitch wearing these badges (a JSON array, or null for none kept). */
	UTclViewer* OnTwitch(const FString& BadgesJson)
	{
		return ViewerFromJson(FString::Printf(
			TEXT("\"twitch\":{\"badges\":%s,\"broadcasterType\":\"\",\"color\":null,\"createdAt\":\"a\","
				"\"description\":\"\",\"displayName\":\"Chatter\",\"id\":\"tw-1\",\"isKnownBot\":false,"
				"\"name\":\"chatter\",\"offlineImageUrl\":\"\",\"profileImageUrl\":\"\",\"type\":\"\","
				"\"updatedAt\":\"b\",\"viewerId\":\"viewer-1\"}"),
			*BadgesJson));
	}

	/** A viewer on Kick wearing these badges (a JSON array, or null for none kept). */
	UTclViewer* OnKick(const FString& BadgesJson)
	{
		return ViewerFromJson(FString::Printf(
			TEXT("\"kick\":{\"badges\":%s,\"channelSlug\":\"chatter\",\"color\":null,\"createdAt\":\"a\","
				"\"id\":7,\"isAnonymous\":false,\"isVerified\":false,\"profilePicture\":null,"
				"\"updatedAt\":\"b\",\"username\":\"chatter\",\"viewerId\":\"viewer-1\"}"),
			*BadgesJson));
	}

	/** A Twitch badge of this set, as JSON. */
	FString TwitchBadge(const TCHAR* SetId)
	{
		return FString::Printf(TEXT("{\"set_id\":\"%s\",\"id\":\"1\",\"info\":\"\"}"), SetId);
	}

	/** A Kick badge of this type, as JSON. */
	FString KickBadge(const TCHAR* Type)
	{
		return FString::Printf(TEXT("{\"type\":\"%s\"}"), Type);
	}
}

/**
 * A viewer's Twitch badges: the ones Tentacle kept from their chat, read as the
 * roles a game asks about, Lead Moderators and founders included.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerTwitchBadgesTest,
	"Tentacle.Viewer.ReadsRolesFromTwitchBadges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerTwitchBadgesTest::RunTest(const FString& Parameters)
{
	UTclViewer* Broadcaster = OnTwitch(FString::Printf(
		TEXT("[%s,%s]"), *TwitchBadge(TEXT("broadcaster")), *TwitchBadge(TEXT("subscriber"))));
	const TArray<UTclTwitchChatBadge*> Badges = Broadcaster->GetBadges_Twitch();
	if (!TestEqual(TEXT("both badges kept"), Badges.Num(), 2)) return false;
	TestEqual(TEXT("in the order they were worn"), Badges[0]->SetId, FString(TEXT("broadcaster")));
	TestTrue(TEXT("the broadcaster"), Broadcaster->IsBroadcaster_Twitch());
	TestTrue(TEXT("a subscriber"), Broadcaster->IsSubscriber_Twitch());
	TestFalse(TEXT("not a moderator"), Broadcaster->IsModerator_Twitch());
	TestFalse(TEXT("not a VIP"), Broadcaster->IsVip_Twitch());

	TestTrue(TEXT("a moderator"), OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("moderator"))))->IsModerator_Twitch());
	TestTrue(TEXT("a Lead Moderator is a moderator"), OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("lead_moderator"))))->IsModerator_Twitch());
	TestTrue(TEXT("a VIP"), OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("vip"))))->IsVip_Twitch());
	TestTrue(TEXT("a founder is a subscriber"), OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("founder"))))->IsSubscriber_Twitch());

	UTclViewer* Cheerer = OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("bits"))));
	TestTrue(TEXT("a badge looked up by its set"), Cheerer->HasBadge_Twitch(TEXT("bits")));
	TestTrue(TEXT("ignoring case by default"), Cheerer->HasBadge_Twitch(TEXT("BITS")));
	TestFalse(TEXT("or not, when asked"), Cheerer->HasBadge_Twitch(TEXT("BITS"), true));
	TestFalse(TEXT("a badge not worn"), Cheerer->HasBadge_Twitch(TEXT("glhf-pledge")));

	UTclViewer* Silent = OnTwitch(TEXT("null"));
	TestEqual(TEXT("no badges until they chat"), Silent->GetBadges_Twitch().Num(), 0);
	TestFalse(TEXT("so no role"), Silent->IsModerator_Twitch());
	TestEqual(TEXT("no Twitch profile, no badges"), OnKick(TEXT("null"))->GetBadges_Twitch().Num(), 0);
	return true;
}

namespace
{
	/** A viewer on Twitch wearing these badges (a JSON array or null), standing as Twitch said (a JSON object). */
	UTclViewer* Standing(const FString& StandingJson, const FString& BadgesJson = TEXT("null"))
	{
		return ViewerFromJson(FString::Printf(
			TEXT("\"twitch\":{\"badges\":%s,\"standing\":%s,\"broadcasterType\":\"\",\"color\":null,\"createdAt\":\"a\","
				"\"description\":\"\",\"displayName\":\"Chatter\",\"id\":\"tw-1\",\"isKnownBot\":false,"
				"\"name\":\"chatter\",\"offlineImageUrl\":\"\",\"profileImageUrl\":\"\",\"type\":\"\","
				"\"updatedAt\":\"b\",\"viewerId\":\"viewer-1\"}"),
			*BadgesJson, *StandingJson));
	}

	/** What Twitch said about a viewer, with these fields set over holding nothing. */
	FString StandingJson(const FString& Fields = FString())
	{
		return FString::Printf(
			TEXT("{\"isBroadcaster\":false,\"isModerator\":false,\"isVip\":false,\"isSubscriber\":false,"
				"\"subscription\":null,\"isFollower\":false,\"followedAt\":null,\"ban\":null,\"bits\":null%s%s}"),
			Fields.IsEmpty() ? TEXT("") : TEXT(","), *Fields);
	}

	/** An ISO time this many hours from now. */
	FString HoursFromNow(double Hours)
	{
		return (FDateTime::UtcNow() + FTimespan::FromHours(Hours)).ToIso8601();
	}
}

/**
 * A viewer's standing in the Twitch channel: their roles as Twitch says them
 * over a stale badge, whether they follow, and whether a ban or a timeout still
 * running is in force.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerTwitchStandingTest,
	"Tentacle.Viewer.ReadsTwitchStanding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerTwitchStandingTest::RunTest(const FString& Parameters)
{
	UTclViewer* Demoted = Standing(
		StandingJson(TEXT("\"isSubscriber\":true")),
		FString::Printf(TEXT("[%s,%s]"), *TwitchBadge(TEXT("moderator")), *TwitchBadge(TEXT("vip"))));
	TestFalse(TEXT("Twitch says they moderate no more, over the badge"), Demoted->IsModerator_Twitch());
	TestFalse(TEXT("nor are they a VIP"), Demoted->IsVip_Twitch());
	TestTrue(TEXT("but they subscribe"), Demoted->IsSubscriber_Twitch());
	TestFalse(TEXT("so on no platform either"), Demoted->IsModerator());
	if (!TestNotNull(TEXT("the standing is reachable"), Demoted->GetStanding_Twitch())) return false;
	TestTrue(TEXT("as Twitch said it"), Demoted->GetStanding_Twitch()->bIsSubscriber);
	TestTrue(TEXT("the broadcaster"), Standing(StandingJson(TEXT("\"isBroadcaster\":true")))->IsBroadcaster_Twitch());
	TestNull(TEXT("no standing from an older Tentacle"), OnTwitch(TEXT("null"))->GetStanding_Twitch());

	TestTrue(TEXT("a follower"), Standing(StandingJson(TEXT("\"isFollower\":true,\"followedAt\":\"2025-03-04T05:06:07.000Z\"")))->IsFollower_Twitch());
	TestFalse(TEXT("not a follower"), Standing(StandingJson())->IsFollower_Twitch());
	TestFalse(TEXT("no standing, no follow"), OnTwitch(TEXT("null"))->IsFollower_Twitch());

	const auto Banned = [](const FString& ExpiresAt)
	{
		return Standing(StandingJson(FString::Printf(
			TEXT("\"ban\":{\"bannedAt\":\"%s\",\"expiresAt\":%s,\"reason\":null}"), *HoursFromNow(-1), *ExpiresAt)));
	};
	TestTrue(TEXT("a ban lasts until lifted"), Banned(TEXT("null"))->IsBanned_Twitch());
	TestTrue(TEXT("a timeout still running"), Banned(FString::Printf(TEXT("\"%s\""), *HoursFromNow(1)))->IsBanned_Twitch());
	TestFalse(TEXT("a timeout that has run out"), Banned(FString::Printf(TEXT("\"%s\""), *HoursFromNow(-1)))->IsBanned_Twitch());
	TestFalse(TEXT("no ban"), Standing(StandingJson())->IsBanned_Twitch());
	return true;
}

/**
 * A viewer's Kick badges, read as the same roles, beside the Get Badges (Kick)
 * the viewer always had.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerKickBadgesTest,
	"Tentacle.Viewer.ReadsRolesFromKickBadges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerKickBadgesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("the broadcaster"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("broadcaster"))))->IsBroadcaster_Kick());
	TestTrue(TEXT("a moderator"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("moderator"))))->IsModerator_Kick());
	TestTrue(TEXT("a VIP"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("vip"))))->IsVip_Kick());
	TestTrue(TEXT("a subscriber"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("subscriber"))))->IsSubscriber_Kick());
	TestTrue(TEXT("a founder is a subscriber"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("founder"))))->IsSubscriber_Kick());
	TestFalse(TEXT("a VIP is not a moderator"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("vip"))))->IsModerator_Kick());

	UTclViewer* Og = OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("og"))));
	TestTrue(TEXT("a badge looked up by its type, ignoring case"), Og->HasBadge_Kick(TEXT("OG")));
	TestFalse(TEXT("or not, when asked"), Og->HasBadge_Kick(TEXT("OG"), true));
	TestEqual(TEXT("no badges kept"), OnKick(TEXT("null"))->GetBadges_Kick().Num(), 0);
	return true;
}

/**
 * A viewer's roles across platforms: true when either platform says so.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerCrossPlatformRolesTest,
	"Tentacle.Viewer.ReadsRolesAcrossPlatforms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerCrossPlatformRolesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("a Twitch moderator"), OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("moderator"))))->IsModerator());
	TestTrue(TEXT("a Kick moderator"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("moderator"))))->IsModerator());
	TestTrue(TEXT("a Kick VIP"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("vip"))))->IsVip());
	TestTrue(TEXT("a Twitch subscriber"), OnTwitch(FString::Printf(TEXT("[%s]"), *TwitchBadge(TEXT("subscriber"))))->IsSubscriber());
	TestTrue(TEXT("the Kick broadcaster"), OnKick(FString::Printf(TEXT("[%s]"), *KickBadge(TEXT("broadcaster"))))->IsBroadcaster());

	UTclViewer* Nobody = NewObject<UTclViewer>(GetTransientPackage());
	TestFalse(TEXT("no data, not the broadcaster"), Nobody->IsBroadcaster());
	TestFalse(TEXT("no data, not a moderator"), Nobody->IsModerator());
	TestFalse(TEXT("no data, not a VIP"), Nobody->IsVip());
	TestFalse(TEXT("no data, not a subscriber"), Nobody->IsSubscriber());
	return true;
}

/**
 * A Twitch chat message's own badges, read as the same roles through the chat
 * message utilities, and a null message answering no.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclChatMessageTwitchBadgesTest,
	"Tentacle.ChatMessage.ReadsRolesFromTwitchBadges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclChatMessageTwitchBadgesTest::RunTest(const FString& Parameters)
{
	UTclStreamEvent_Twitch_ChatMessage* Message = NewObject<UTclStreamEvent_Twitch_ChatMessage>(GetTransientPackage());
	for (const TCHAR* SetId : { TEXT("lead_moderator"), TEXT("founder"), TEXT("bits") })
	{
		UTclTwitchChatBadge* Badge = NewObject<UTclTwitchChatBadge>(Message);
		Badge->SetId = SetId;
		Badge->Id = TEXT("1");
		Message->Badges.Add(Badge);
	}

	TestTrue(TEXT("a Lead Moderator's message is a moderator's"), UTclChatMessageUtils::IsModerator_Twitch(Message));
	TestTrue(TEXT("a founder's message is a subscriber's"), UTclChatMessageUtils::IsSubscriber_Twitch(Message));
	TestFalse(TEXT("not the broadcaster's"), UTclChatMessageUtils::IsBroadcaster_Twitch(Message));
	TestFalse(TEXT("not a VIP's"), UTclChatMessageUtils::IsVip_Twitch(Message));
	TestTrue(TEXT("a badge looked up ignoring case"), UTclChatMessageUtils::HasBadge_Twitch(Message, TEXT("Bits")));
	TestFalse(TEXT("or not, when asked"), UTclChatMessageUtils::HasBadge_Twitch(Message, TEXT("Bits"), true));

	TestFalse(TEXT("a null message is nobody's"), UTclChatMessageUtils::IsModerator_Twitch(nullptr));
	TestFalse(TEXT("and wears nothing"), UTclChatMessageUtils::HasBadge_Twitch(nullptr, TEXT("vip")));
	return true;
}

#endif
