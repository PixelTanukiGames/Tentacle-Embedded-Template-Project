// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "Base/TclSignInAction.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsPrompt.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsResult.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	const TCHAR* KickPrompt = TEXT("{\"platform\":\"kick\",\"url\":\"https://id.kick.com/oauth/authorize\",\"expiresInSeconds\":600}");

	/** A Sign In node for Platform with its pins recorded and the browser captured, activated as Blueprint activates it. */
	UTclSignInAction* StartSignIn(UWorld* World, ETclStreamPlatform Platform, bool bOpenBrowser, UTclTestRecorder* Recorder, TArray<FString>& Opened)
	{
		UTclSignInAction* Action = UTclSignInAction::SignIn(World, Platform, bOpenBrowser);
		Action->OpenUrl = [&Opened](const FString& Url) { Opened.Add(Url); };
		Action->OnPrompt.AddDynamic(Recorder, &UTclTestRecorder::SignIn_OnPrompt);
		Action->OnConnected.AddDynamic(Recorder, &UTclTestRecorder::SignIn_OnConnected);
		Action->OnFailed.AddDynamic(Recorder, &UTclTestRecorder::SignIn_OnFailed);
		Action->Activate();
		return Action;
	}
}

/**
 * One Sign In node does what the Connections widget's button does: asks
 * Tentacle to connect the platform, opens the page it hands back, says what
 * to show, and ends with how signing in went. Another platform's prompt and
 * result are not its own.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInTest,
	"Tentacle.SignIn.SignsInAndOpensThePage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Embedded);
	TclTest::FSentMessages Sent(Tentacle);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>();
	TArray<FString> Opened;

	StartSignIn(World.Get(), ETclStreamPlatform::Twitch, true, Recorder, Opened);
	TestEqual(TEXT("it asks Tentacle to connect"), Sent.Type(0), FString(TEXT("Connections_Connect")));
	TestEqual(TEXT("Twitch"), TclTest::StringField(Sent.Data(0), TEXT("platform")), FString(TEXT("twitch")));

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), KickPrompt);
	TestEqual(TEXT("Kick's prompt is not its own"), Recorder->Calls.Num(), 0);

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TclTest::TwitchConnectionsPromptJson());
	TestEqual(TEXT("it opens the page"), FString::Join(Opened, TEXT(" ")), FString(TEXT("https://www.twitch.tv/activate?device-code=ABCD")));
	const FTclRecordedCall* Prompt = Recorder->Find(TEXT("OnPrompt"));
	if (TestNotNull(TEXT("On Prompt fires"), Prompt))
		TestEqual(TEXT("with the page and the code"), Prompt->Detail, FString(TEXT("https://www.twitch.tv/activate?device-code=ABCD|ABCD|")));

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"kick\",\"outcome\":\"connected\"}"));
	TestEqual(TEXT("Kick's result is not its own"), Recorder->Count(TEXT("OnConnected")), 0);

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"twitch\",\"outcome\":\"connected\"}"));
	TestEqual(TEXT("On Connected fires"), Recorder->Names(), TArray<FString>{ TEXT("OnPrompt"), TEXT("OnConnected") });

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"twitch\",\"outcome\":\"connected\"}"));
	TestEqual(TEXT("and it has finished"), Recorder->Calls.Num(), 2);
	return true;
}

/**
 * A sign-in that does not connect ends at On Failed with a message saying how,
 * the same words the Connections widget shows; with Open Browser off the page
 * is left to the game.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInFailsTest,
	"Tentacle.SignIn.FailsWithTheOutcome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInFailsTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Embedded);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>();
	TArray<FString> Opened;

	StartSignIn(World.Get(), ETclStreamPlatform::Twitch, false, Recorder, Opened);
	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TclTest::TwitchConnectionsPromptJson());
	TestEqual(TEXT("with Open Browser off, nothing opens"), Opened.Num(), 0);
	TestEqual(TEXT("but On Prompt still fires"), Recorder->Count(TEXT("OnPrompt")), 1);
	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"twitch\",\"outcome\":\"denied\"}"));
	const FTclRecordedCall* Failed = Recorder->Find(TEXT("OnFailed"));
	if (TestNotNull(TEXT("On Failed fires"), Failed))
		TestEqual(TEXT("saying it was declined"), Failed->Detail, FString(TEXT("||Sign-in was declined")));

	UTclTestRecorder* Second = NewObject<UTclTestRecorder>();
	StartSignIn(World.Get(), ETclStreamPlatform::Kick, true, Second, Opened);
	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"kick\",\"outcome\":\"error\",\"message\":\"Kick did not answer\"}"));
	const FTclRecordedCall* Error = Second->Find(TEXT("OnFailed"));
	if (TestNotNull(TEXT("an error fails too"), Error))
		TestEqual(TEXT("with Tentacle's message"), Error->Detail, FString(TEXT("||Sign-in failed: Kick did not answer")));
	return true;
}

/**
 * Signing in again replaces the attempt in progress, which Tentacle drops
 * without a result, so the earlier node ends as cancelled there and then.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInReplacedTest,
	"Tentacle.SignIn.SigningInAgainCancelsTheEarlierNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInReplacedTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Embedded);
	UTclTestRecorder* First = NewObject<UTclTestRecorder>();
	UTclTestRecorder* Kick = NewObject<UTclTestRecorder>();
	UTclTestRecorder* Second = NewObject<UTclTestRecorder>();
	TArray<FString> Opened;

	StartSignIn(World.Get(), ETclStreamPlatform::Twitch, true, First, Opened);
	StartSignIn(World.Get(), ETclStreamPlatform::Kick, true, Kick, Opened);
	StartSignIn(World.Get(), ETclStreamPlatform::Twitch, true, Second, Opened);
	const FTclRecordedCall* Cancelled = First->Find(TEXT("OnFailed"));
	if (TestNotNull(TEXT("the earlier Twitch node ends"), Cancelled))
		TestEqual(TEXT("as cancelled"), Cancelled->Detail, FString(TEXT("||Sign-in was cancelled")));
	TestEqual(TEXT("Kick's carries on"), Kick->Calls.Num(), 0);

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"twitch\",\"outcome\":\"connected\"}"));
	TestEqual(TEXT("the new one connects"), Second->Names(), TArray<FString>{ TEXT("OnConnected") });
	TestEqual(TEXT("and the earlier one heard nothing more"), First->Calls.Num(), 1);
	return true;
}

/**
 * Sign In only applies to Tentacle embedded in the game: with the Tentacle app,
 * or with no Tentacle actor at all, it fails at once and asks for nothing.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInNotEmbeddedTest,
	"Tentacle.SignIn.FailsAtOnceWhenNotEmbedded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInNotEmbeddedTest::RunTest(const FString& Parameters)
{
	TArray<FString> Opened;
	{
		TclTest::FWorld World;
		if (!TestNotNull(TEXT("test world"), World.Get())) return false;
		ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Paired);
		TclTest::FSentMessages Sent(Tentacle);
		UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>();
		StartSignIn(World.Get(), ETclStreamPlatform::Twitch, true, Recorder, Opened);
		const FTclRecordedCall* Failed = Recorder->Find(TEXT("OnFailed"));
		if (TestNotNull(TEXT("in Paired mode it fails"), Failed))
			TestEqual(TEXT("saying the app connects them"), Failed->Detail, FString(TEXT("||Twitch and Kick connect through the Tentacle app")));
		TestEqual(TEXT("and asks Tentacle for nothing"), Sent.Num(), 0);
	}
	{
		TclTest::FWorld World;
		if (!TestNotNull(TEXT("test world"), World.Get())) return false;
		UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>();
		AddExpectedError(TEXT("No Tentacle actor"), EAutomationExpectedErrorFlags::Contains, 1);
		StartSignIn(World.Get(), ETclStreamPlatform::Twitch, true, Recorder, Opened);
		const FTclRecordedCall* Failed = Recorder->Find(TEXT("OnFailed"));
		if (TestNotNull(TEXT("with no Tentacle actor it fails"), Failed))
			TestEqual(TEXT("saying so"), Failed->Detail, FString(TEXT("||No Tentacle actor in the level. Have you added a Tentacle actor?")));
	}
	TestEqual(TEXT("and nothing opened"), Opened.Num(), 0);
	return true;
}

#endif
