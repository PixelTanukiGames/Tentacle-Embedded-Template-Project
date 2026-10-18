// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Stream.h"
#include "Base/Tentacle_Viewers.h"
#include "Base/TclViewer.h"
#include "Sdk/TclMsg_ServerToClient_StreamEvent.h"
#include "Sdk/TclMsg_ServerToClient_StreamChatJoin.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** Bind every stream delegate the catch-all tests watch to Recorder. */
	void BindStreamRecorder(ATentacle* Tentacle, UTclTestRecorder* Recorder)
	{
		UTentacle_Stream* Stream = Tentacle->Stream;
		Tentacle->OnViewerAdded.AddDynamic(Recorder, &UTclTestRecorder::OnViewerAdded_Actor);
		Stream->OnEvent_Anywhere.AddDynamic(Recorder, &UTclTestRecorder::OnEvent_Anywhere);
		Stream->OnEvent_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnEvent_Twitch);
		Stream->OnEvent_Kick.AddDynamic(Recorder, &UTclTestRecorder::OnEvent_Kick);
		Stream->OnFollow_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnFollow_Twitch);
		Stream->OnChannelFollowed_Kick.AddDynamic(Recorder, &UTclTestRecorder::OnChannelFollowed_Kick);
		Stream->OnFollow_Anywhere.AddDynamic(Recorder, &UTclTestRecorder::OnFollow_Anywhere);
	}
}

/**
 * The catch-alls fire first for every stream event: On Event (Anywhere), then
 * On Event (<Platform>), then the event's own delegate, then its Anywhere view
 * - and only once the event has survived de-duplication and its viewer is in
 * the cache, so a handler sees the same cached viewer every other delegate does.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclStreamCatchAllOrderTest,
	"Tentacle.Stream.CatchAllsFireFirstForEveryEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclStreamCatchAllOrderTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	BindStreamRecorder(Tentacle, Recorder);

	// A Twitch follow from a viewer Tentacle has not mentioned before.
	auto* TwitchFollow = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamEvent>(Outer,
		TclTest::StreamEventMessage(TclTest::TwitchFollowEventJson(TEXT("evt-1"), TEXT("viewer-1")), TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice"))));
	if (!TestNotNull(TEXT("the Twitch follow parses"), TwitchFollow)) return false;
	Tentacle->Stream->HandleMsg_Event(TwitchFollow);

	TestEqual(TEXT("Twitch follow: firing order"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("OnViewerAdded_Actor, OnEvent_Anywhere, OnEvent_Twitch, OnFollow_Twitch, OnFollow_Anywhere"));

	bool bFound = false;
	const UTclViewer* Alice = Tentacle->FindViewerById(TEXT("viewer-1"), bFound);
	if (!TestTrue(TEXT("the viewer is cached"), bFound)) return false;

	for (const FTclRecordedCall& Call : Recorder->Calls)
		TestTrue(FString::Printf(TEXT("%s carries the cached viewer"), *Call.Name), Call.Viewer.Get() == Alice);

	// The catch-alls hand over the very event object the per-kind delegate gets.
	const FTclRecordedCall* Anywhere = Recorder->Find(TEXT("OnEvent_Anywhere"));
	const FTclRecordedCall* Twitch = Recorder->Find(TEXT("OnEvent_Twitch"));
	const FTclRecordedCall* Follow = Recorder->Find(TEXT("OnFollow_Twitch"));
	if (!TestTrue(TEXT("all three recorded"), Anywhere && Twitch && Follow)) return false;
	TestTrue(TEXT("On Event (Anywhere) carries the follow event"), Anywhere->Event.Get() == Follow->Event.Get());
	TestTrue(TEXT("On Event (Twitch) carries the follow event"), Twitch->Event.Get() == Follow->Event.Get());

	// A replay of the same event id is de-duplicated before the catch-alls too.
	Recorder->Calls.Reset();
	auto* Replay = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamEvent>(Outer,
		TclTest::StreamEventMessage(TclTest::TwitchFollowEventJson(TEXT("evt-1"), TEXT("viewer-1")), TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice"))));
	Tentacle->Stream->HandleMsg_Event(Replay);
	TestEqual(TEXT("a replayed event fires nothing"), Recorder->Calls.Num(), 0);

	// A Kick follow takes the Kick catch-all, and never the Twitch one.
	auto* KickFollow = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamEvent>(Outer,
		TclTest::StreamEventMessage(TclTest::KickFollowEventJson(TEXT("evt-2"), TEXT("viewer-2")), TclTest::KickViewerJson(TEXT("viewer-2"), TEXT("bob"))));
	if (!TestNotNull(TEXT("the Kick follow parses"), KickFollow)) return false;
	Tentacle->Stream->HandleMsg_Event(KickFollow);

	TestEqual(TEXT("Kick follow: firing order"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("OnViewerAdded_Actor, OnEvent_Anywhere, OnEvent_Kick, OnChannelFollowed_Kick, OnFollow_Anywhere"));

	// A kind this plugin version has no delegate for still reaches both
	// catch-alls, with no viewer since it names none.
	Recorder->Calls.Reset();
	auto* Future = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamEvent>(Outer,
		TclTest::StreamEventMessage(TclTest::TwitchFutureEventJson(TEXT("evt-3")), FString()));
	if (!TestNotNull(TEXT("the unknown kind parses"), Future)) return false;
	Tentacle->Stream->HandleMsg_Event(Future);

	TestEqual(TEXT("unknown kind: only the catch-alls fire"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("OnEvent_Anywhere, OnEvent_Twitch"));
	for (const FTclRecordedCall& Call : Recorder->Calls)
		TestFalse(FString::Printf(TEXT("%s has no viewer for a channel event"), *Call.Name), Call.Viewer.IsValid());

	return true;
}

/**
 * A chat join fires On Chat Join (Anywhere) right after the platform's own
 * delegate, with the cached viewer, on either platform.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclStreamChatJoinAnywhereTest,
	"Tentacle.Stream.ChatJoinFiresAnywhereAfterThePlatformDelegate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclStreamChatJoinAnywhereTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	Tentacle->Stream->OnChatJoin_Twitch.AddDynamic(Recorder, &UTclTestRecorder::OnChatJoin_Twitch);
	Tentacle->Stream->OnChatJoin_Kick.AddDynamic(Recorder, &UTclTestRecorder::OnChatJoin_Kick);
	Tentacle->Stream->OnChatJoin_Anywhere.AddDynamic(Recorder, &UTclTestRecorder::OnChatJoin_Anywhere);

	auto* TwitchJoin = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamChatJoin>(Outer,
		TclTest::ChatJoinMessage(TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice"))));
	if (!TestNotNull(TEXT("the Twitch join parses"), TwitchJoin)) return false;
	Tentacle->Stream->HandleMsg_ChatJoin(TwitchJoin);

	auto* KickJoin = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamChatJoin>(Outer,
		TclTest::ChatJoinMessage(TclTest::KickViewerJson(TEXT("viewer-2"), TEXT("bob"))));
	if (!TestNotNull(TEXT("the Kick join parses"), KickJoin)) return false;
	Tentacle->Stream->HandleMsg_ChatJoin(KickJoin);

	TestEqual(TEXT("firing order"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("OnChatJoin_Twitch, OnChatJoin_Anywhere, OnChatJoin_Kick, OnChatJoin_Anywhere"));

	bool bFound = false;
	const UTclViewer* Alice = Tentacle->FindViewerById(TEXT("viewer-1"), bFound);
	const UTclViewer* Bob = Tentacle->FindViewerById(TEXT("viewer-2"), bFound);
	if (!TestEqual(TEXT("four calls"), Recorder->Calls.Num(), 4)) return false;
	TestTrue(TEXT("Twitch join: Anywhere carries the cached viewer"), Recorder->Calls[1].Viewer.Get() == Alice);
	TestTrue(TEXT("Kick join: Anywhere carries the cached viewer"), Recorder->Calls[3].Viewer.Get() == Bob);

	return true;
}

/**
 * The send nodes' length limit is the published constant, and the send path
 * enforces exactly it: a message of that length goes out, one character more
 * does not.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclStreamChatMessageMaxLengthTest,
	"Tentacle.Stream.ChatMessageMaxLengthIsWhatTheSendPathEnforces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclStreamChatMessageMaxLengthTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the constant"), UTentacle_Stream::ChatMessageMaxLength, 500);
	TestEqual(TEXT("the Blueprint getter"), UTentacle_Stream::GetChatMessageMaxLength(), UTentacle_Stream::ChatMessageMaxLength);

	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	TclTest::FSentMessages Sent(Tentacle);

	Tentacle->Stream->SendChatMessage(FString::ChrN(UTentacle_Stream::GetChatMessageMaxLength(), TEXT('a')));
	TestEqual(TEXT("a message at the limit is sent"), Sent.Num(), 1);
	TestEqual(TEXT("as a chat message"), Sent.Type(0), FString(TEXT("Stream_SendChatMessage")));

	Tentacle->Stream->SendChatMessage(FString::ChrN(UTentacle_Stream::GetChatMessageMaxLength() + 1, TEXT('a')));
	TestEqual(TEXT("one character more is not"), Sent.Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
