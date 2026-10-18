// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Kick.h"
#include "Base/Tentacle_Stream.h"
#include "Base/Tentacle_Twitch.h"
#include "Base/Tentacle_TwitchExtras.h"
#include "Base/TclViewer.h"
#include "Enums/TclChatSendTarget.h"
#include "Sdk/TclMsg_ServerToClient_StreamChatJoin.h"
#include "Sdk/TclMsg_ServerToClient_StreamEvent.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

// The stream surface as it stands since the restructure: cross-platform members
// on the Tentacle actor, each platform's own on its Twitch or Kick component
// under the platform's own names, what Tentacle adds to Twitch on its Extras,
// and the shipped Stream component deprecated but still firing, after them.

namespace
{
	/** Parse and hand a Stream_Event message to the actor, the way a received message arrives. */
	void DeliverStreamEvent(ATentacle* Tentacle, const FString& EventJson, const FString& ViewerJson)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamEvent>(Tentacle, TclTest::StreamEventMessage(EventJson, ViewerJson));
		check(Msg);
		Tentacle->HandleStreamEvent(Msg);
	}

	/** Parse and hand a Stream_ChatJoin message to the actor. */
	void DeliverChatJoin(ATentacle* Tentacle, const FString& ViewerJson)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamChatJoin>(Tentacle, TclTest::ChatJoinMessage(ViewerJson));
		check(Msg);
		Tentacle->HandleChatJoin(Msg);
	}
}

/**
 * One stream event fires the entry point's catch-all, then the platform's own
 * event, then the entry point's cross-platform view - and only then the
 * deprecated Stream component, in its original order. The catch-all is the
 * entry point's alone.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesStreamEventOrderTest,
	"Tentacle.Surfaces.StreamEventFiresTheEntryPointAndPlatformBeforeTheDeprecatedStream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesStreamEventOrderTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);

	Tentacle->OnStreamEvent.AddDynamic(Recorder, &UTclTestRecorder::Root_OnStreamEvent);
	Tentacle->OnFollow.AddDynamic(Recorder, &UTclTestRecorder::Root_OnFollow);
	Tentacle->Twitch->OnChannelFollow.AddDynamic(Recorder, &UTclTestRecorder::Twitch_OnChannelFollow);
	Tentacle->Kick->OnChannelFollow.AddDynamic(Recorder, &UTclTestRecorder::Kick_OnChannelFollow);
	Tentacle->Stream->OnEvent_Anywhere.AddDynamic(Recorder, &UTclTestRecorder::OnEvent_Anywhere);
	Tentacle->Stream->OnEvent_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnEvent_Twitch);
	Tentacle->Stream->OnEvent_Kick.AddDynamic(Recorder, &UTclTestRecorder::OnEvent_Kick);
	Tentacle->Stream->OnFollow_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnFollow_Twitch);
	Tentacle->Stream->OnChannelFollowed_Kick.AddDynamic(Recorder, &UTclTestRecorder::OnChannelFollowed_Kick);
	Tentacle->Stream->OnFollow_Anywhere.AddDynamic(Recorder, &UTclTestRecorder::OnFollow_Anywhere);

	DeliverStreamEvent(Tentacle, TclTest::TwitchFollowEventJson(TEXT("evt-1"), TEXT("viewer-1")), TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice")));
	TestEqual(TEXT("Twitch follow: firing order"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Root_OnStreamEvent, Twitch_OnChannelFollow, Root_OnFollow, ")
		TEXT("OnEvent_Anywhere, OnEvent_Twitch, OnFollow_Twitch, OnFollow_Anywhere"));

	bool bFound = false;
	const UTclViewer* Alice = Tentacle->FindViewerById(TEXT("viewer-1"), bFound);
	if (!TestTrue(TEXT("the viewer is cached"), bFound)) return false;
	for (const FTclRecordedCall& Call : Recorder->Calls)
		TestTrue(FString::Printf(TEXT("%s carries the cached viewer"), *Call.Name), Call.Viewer.Get() == Alice);

	const FTclRecordedCall* RootCatchAll = Recorder->Find(TEXT("Root_OnStreamEvent"));
	const FTclRecordedCall* TwitchFollow = Recorder->Find(TEXT("Twitch_OnChannelFollow"));
	const FTclRecordedCall* DeprecatedFollow = Recorder->Find(TEXT("OnFollow_Twitch"));
	if (!TestTrue(TEXT("recorded"), RootCatchAll && TwitchFollow && DeprecatedFollow)) return false;
	TestTrue(TEXT("every surface hands over the same event object"),
		RootCatchAll->Event.Get() == TwitchFollow->Event.Get() && TwitchFollow->Event.Get() == DeprecatedFollow->Event.Get());

	// A replay is de-duplicated once, before any surface.
	Recorder->Calls.Reset();
	DeliverStreamEvent(Tentacle, TclTest::TwitchFollowEventJson(TEXT("evt-1"), TEXT("viewer-1")), TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice")));
	TestEqual(TEXT("a replayed event fires nothing anywhere"), Recorder->Calls.Num(), 0);

	// Kick goes to the Kick component, never the Twitch one.
	DeliverStreamEvent(Tentacle, TclTest::KickFollowEventJson(TEXT("evt-2"), TEXT("viewer-2")), TclTest::KickViewerJson(TEXT("viewer-2"), TEXT("bob")));
	TestEqual(TEXT("Kick follow: firing order"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Root_OnStreamEvent, Kick_OnChannelFollow, Root_OnFollow, ")
		TEXT("OnEvent_Anywhere, OnEvent_Kick, OnChannelFollowed_Kick, OnFollow_Anywhere"));

	// A kind this version has no delegate for still reaches every catch-all.
	Recorder->Calls.Reset();
	DeliverStreamEvent(Tentacle, TclTest::TwitchFutureEventJson(TEXT("evt-3")), FString());
	TestEqual(TEXT("unknown kind: only the catch-alls fire"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Root_OnStreamEvent, OnEvent_Anywhere, OnEvent_Twitch"));

	return true;
}

/**
 * A chat join fires the entry point's On Chat Join alone - a join is
 * Tentacle's own event, not a platform's - then the deprecated Stream
 * component's two.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesChatJoinTest,
	"Tentacle.Surfaces.ChatJoinFiresTheEntryPointAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesChatJoinTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);

	Tentacle->OnChatJoin.AddDynamic(Recorder, &UTclTestRecorder::Root_OnChatJoin);
	Tentacle->Stream->OnChatJoin_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnChatJoin_Twitch);
	Tentacle->Stream->OnChatJoin_Kick.AddDynamic(Recorder, &UTclTestRecorder::OnChatJoin_Kick);
	Tentacle->Stream->OnChatJoin_Anywhere.AddDynamic(Recorder, &UTclTestRecorder::OnChatJoin_Anywhere);

	DeliverChatJoin(Tentacle, TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice")));
	DeliverChatJoin(Tentacle, TclTest::KickViewerJson(TEXT("viewer-2"), TEXT("bob")));

	TestEqual(TEXT("firing order"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Root_OnChatJoin, OnChatJoin_Twitch, OnChatJoin_Anywhere, ")
		TEXT("Root_OnChatJoin, OnChatJoin_Kick, OnChatJoin_Anywhere"));

	bool bFound = false;
	const UTclViewer* Alice = Tentacle->FindViewerById(TEXT("viewer-1"), bFound);
	if (!TestEqual(TEXT("six calls"), Recorder->Calls.Num(), 6)) return false;
	TestTrue(TEXT("the entry point's join carries the cached viewer"), Recorder->Calls[0].Viewer.Get() == Alice);

	return true;
}

/**
 * The platform's own chat event is the whole message, commands included, and
 * fires first. The split into message and command is the entry point's alone:
 * a command never also raises its On Chat Message.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesChatSplitTest,
	"Tentacle.Surfaces.ChatSplitsIntoMessagesAndCommandsOnTheEntryPointAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesChatSplitTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);

	Tentacle->OnChatMessage.AddDynamic(Recorder, &UTclTestRecorder::Root_OnChatMessage);
	Tentacle->OnChatCommand.AddDynamic(Recorder, &UTclTestRecorder::Root_OnChatCommand);
	Tentacle->Twitch->OnChannelChatMessage.AddDynamic(Recorder, &UTclTestRecorder::Twitch_OnChannelChatMessage);

	DeliverStreamEvent(Tentacle, TclTest::TwitchChatMessageEventJson(TEXT("evt-1"), TEXT("viewer-1"), TEXT("hello")),
		TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice")));
	TestEqual(TEXT("a message"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Twitch_OnChannelChatMessage, Root_OnChatMessage"));

	Recorder->Calls.Reset();
	DeliverStreamEvent(Tentacle, TclTest::TwitchChatMessageEventJson(TEXT("evt-2"), TEXT("viewer-1"), TEXT("!join"), TEXT("join")),
		TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice")));
	TestEqual(TEXT("a command"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Twitch_OnChannelChatMessage, Root_OnChatCommand"));

	return true;
}

/**
 * The incoming raid window lives on the Twitch component's Extras, what
 * Tentacle adds to Twitch. The deprecated Stream component's copy follows it,
 * and ending the raid from either one ends it for both.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesRaidWindowTest,
	"Tentacle.Surfaces.IncomingRaidWindowLivesOnTwitchExtrasAndTheDeprecatedStreamFollows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesRaidWindowTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	UTentacle_TwitchExtras* Extras = Tentacle->Twitch->Extras;
	if (!TestNotNull(TEXT("the Twitch component has its Extras"), Extras)) return false;

	Tentacle->Twitch->OnChannelRaid.AddDynamic(Recorder, &UTclTestRecorder::Twitch_OnChannelRaid);
	Extras->OnIncomingRaidEnded.AddDynamic(Recorder, &UTclTestRecorder::TwitchExtras_OnIncomingRaidEnded);
	Tentacle->Stream->OnRaidEnded_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnRaidEnded_Twitch);

	DeliverStreamEvent(Tentacle, TclTest::TwitchRaidEventJson(TEXT("evt-1")), FString());
	TestTrue(TEXT("a raid opens the window"), Extras->bIsIncomingRaidOngoing);
	TestTrue(TEXT("the deprecated flag follows"), Tentacle->Stream->bIsTwitchRaidOngoing);

	Extras->EndIncomingRaid();
	TestFalse(TEXT("End Incoming Raid closes the window"), Extras->bIsIncomingRaidOngoing);
	TestFalse(TEXT("the deprecated flag follows"), Tentacle->Stream->bIsTwitchRaidOngoing);

	DeliverStreamEvent(Tentacle, TclTest::TwitchRaidEventJson(TEXT("evt-2")), FString());
	Tentacle->Stream->EndRaid_Twitch();
	TestFalse(TEXT("the deprecated End Raid closes the window too"), Extras->bIsIncomingRaidOngoing);
	TestFalse(TEXT("and its own flag"), Tentacle->Stream->bIsTwitchRaidOngoing);

	TestEqual(TEXT("both raid-ended events fire, the new one first, for either end"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("Twitch_OnChannelRaid, TwitchExtras_OnIncomingRaidEnded, OnRaidEnded_Twitch, ")
		TEXT("Twitch_OnChannelRaid, TwitchExtras_OnIncomingRaidEnded, OnRaidEnded_Twitch"));

	return true;
}

/**
 * Each platform component carries only that platform's own events: the
 * catch-all, the chat split, and joins are the entry point's alone.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesPlatformsCarryOnlyTheirOwnEventsTest,
	"Tentacle.Surfaces.PlatformComponentsCarryOnlyThePlatformsOwnEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesPlatformsCarryOnlyTheirOwnEventsTest::RunTest(const FString& Parameters)
{
	for (const UClass* Surface : { UTentacle_Twitch::StaticClass(), UTentacle_Kick::StaticClass() })
	{
		for (const TCHAR* RootOnly : { TEXT("OnStreamEvent"), TEXT("OnChatCommand"), TEXT("OnChatMessageWithEmojis"), TEXT("OnChatMessageWithEmotes"), TEXT("OnChatJoin") })
		{
			TestNull(FString::Printf(TEXT("%s has no %s"), *Surface->GetName(), RootOnly), Surface->FindPropertyByName(RootOnly));
		}
	}
	TestNotNull(TEXT("Twitch's own chat event"), UTentacle_Twitch::StaticClass()->FindPropertyByName(TEXT("OnChannelChatMessage")));
	TestNotNull(TEXT("Kick's own chat event"), UTentacle_Kick::StaticClass()->FindPropertyByName(TEXT("OnChatMessage")));
	return true;
}

/**
 * Chat can be sent from the entry point to every connected platform, and from
 * each platform component through that platform's own request (Twitch's Send
 * Chat Message, Kick's Post Chat Message), with the same local validation
 * everywhere. The deprecated Stream component's send still works, target and
 * all.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesChatSendTest,
	"Tentacle.Surfaces.ChatSendsFromTheEntryPointAndEachPlatform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesChatSendTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the limit"), ATentacle::ChatMessageMaxLength, 500);
	TestEqual(TEXT("the Blueprint getter"), ATentacle::GetChatMessageMaxLength(), ATentacle::ChatMessageMaxLength);

	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	TclTest::FSentMessages Sent(Tentacle);
	UTclViewer* Alice = Tentacle->UpsertViewer(TclTest::ParseObject(TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice"))));
	if (!TestNotNull(TEXT("a viewer to mention"), Alice)) return false;

	Tentacle->SendChatMessage(TEXT("hi all"));
	Tentacle->SendChatMessage(TEXT("hi all, from the bot"), true);
	Tentacle->SendChatMessageToViewer(Alice, TEXT("thanks"));
	Tentacle->Stream->SendChatMessage(TEXT("from the deprecated component"), ETclChatSendTarget::Kick);
	Tentacle->Twitch->SendChatMessage(TEXT("hi twitch"));
	Tentacle->Twitch->SendChatMessage(TEXT("replying on twitch"), TEXT("twitch-msg-1"));
	Tentacle->Kick->PostChatMessage(TEXT("hi kick"));
	Tentacle->Kick->PostChatMessage(TEXT("bot on kick"), ETclKickChatMessageType::Bot, TEXT("kick-msg-1"));
	if (!TestEqual(TEXT("eight sends"), Sent.Num(), 8)) return false;

	for (int32 Index = 0; Index < 4; ++Index)
		TestEqual(FString::Printf(TEXT("send %d is a chat message"), Index), Sent.Type(Index), FString(TEXT("Stream_SendChatMessage")));

	TestFalse(TEXT("the entry point targets every platform"), Sent.Data(0)->HasField(TEXT("platform")));
	TestFalse(TEXT("not as the bot by default"), Sent.Data(0)->GetBoolField(TEXT("isBot")));
	TestFalse(TEXT("the entry point's bot send targets every platform too"), Sent.Data(1)->HasField(TEXT("platform")));
	TestTrue(TEXT("as the bot"), Sent.Data(1)->GetBoolField(TEXT("isBot")));
	TestEqual(TEXT("a mention carries the viewer"), TclTest::StringField(Sent.Data(2), TEXT("viewerId")), FString(TEXT("viewer-1")));
	TestFalse(TEXT("to every platform"), Sent.Data(2)->HasField(TEXT("platform")));
	TestEqual(TEXT("the deprecated send keeps its target"), TclTest::StringField(Sent.Data(3), TEXT("platform")), FString(TEXT("kick")));

	TestEqual(TEXT("Twitch's send is Twitch's own request"), Sent.Type(4), FString(TEXT("Twitch_SendChatMessage")));
	TestEqual(TEXT("with Twitch's message"), TclTest::StringField(Sent.Data(4), TEXT("message")), FString(TEXT("hi twitch")));
	TestFalse(TEXT("and no reply by default"), Sent.Data(4)->HasField(TEXT("replyParentMessageId")));
	TestEqual(TEXT("a Twitch reply names its parent"), TclTest::StringField(Sent.Data(5), TEXT("replyParentMessageId")), FString(TEXT("twitch-msg-1")));

	TestEqual(TEXT("Kick's post is Kick's own request"), Sent.Type(6), FString(TEXT("Kick_PostChatMessage")));
	TestEqual(TEXT("with Kick's content"), TclTest::StringField(Sent.Data(6), TEXT("content")), FString(TEXT("hi kick")));
	TestEqual(TEXT("as the broadcaster by default"), TclTest::StringField(Sent.Data(6), TEXT("messageType")), FString(TEXT("user")));
	TestFalse(TEXT("and no reply by default"), Sent.Data(6)->HasField(TEXT("replyToMessageId")));
	TestEqual(TEXT("as the bot"), TclTest::StringField(Sent.Data(7), TEXT("messageType")), FString(TEXT("bot")));
	TestEqual(TEXT("a Kick reply names its message"), TclTest::StringField(Sent.Data(7), TEXT("replyToMessageId")), FString(TEXT("kick-msg-1")));

	// Refused locally, on every surface: nothing reaches the socket.
	Sent.Reset();
	Tentacle->SendChatMessage(FString());
	Tentacle->SendChatMessageToViewer(nullptr, TEXT("nobody to mention"));
	Tentacle->Twitch->SendChatMessage(FString::ChrN(ATentacle::ChatMessageMaxLength + 1, TEXT('a')));
	Tentacle->Kick->PostChatMessage(FString());
	Tentacle->Kick->PostChatMessage(FString::ChrN(ATentacle::ChatMessageMaxLength + 1, TEXT('a')));
	TestEqual(TEXT("empty, over-long, and viewerless sends are refused"), Sent.Num(), 0);

	Tentacle->Twitch->SendChatMessage(FString::ChrN(ATentacle::ChatMessageMaxLength, TEXT('a')));
	Tentacle->Kick->PostChatMessage(FString::ChrN(ATentacle::ChatMessageMaxLength, TEXT('a')));
	TestEqual(TEXT("a message at the limit is sent"), Sent.Num(), 2);

	return true;
}

/**
 * The platform components' own sends: the one-per-platform chat send members
 * are exactly the platform's endpoint names, and the per-platform mention send
 * is gone (the entry point keeps Send Chat Message To Viewer).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSurfacesPlatformSendNamesTest,
	"Tentacle.Surfaces.PlatformSendsAreNamedAfterThePlatformsEndpoints",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSurfacesPlatformSendNamesTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Twitch's Send Chat Message"), UTentacle_Twitch::StaticClass()->FindFunctionByName(TEXT("SendChatMessage")));
	TestNull(TEXT("no Twitch mention send"), UTentacle_Twitch::StaticClass()->FindFunctionByName(TEXT("SendChatMessageToViewer")));
	TestNotNull(TEXT("Kick's Post Chat Message"), UTentacle_Kick::StaticClass()->FindFunctionByName(TEXT("PostChatMessage")));
	TestNull(TEXT("no Kick Send Chat Message"), UTentacle_Kick::StaticClass()->FindFunctionByName(TEXT("SendChatMessage")));
	TestNull(TEXT("no Kick mention send"), UTentacle_Kick::StaticClass()->FindFunctionByName(TEXT("SendChatMessageToViewer")));
	TestNotNull(TEXT("the entry point keeps its mention send"), ATentacle::StaticClass()->FindFunctionByName(TEXT("SendChatMessageToViewer")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
