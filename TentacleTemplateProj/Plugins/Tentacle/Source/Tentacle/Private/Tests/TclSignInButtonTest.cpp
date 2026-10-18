// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsPrompt.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsResult.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsStatus.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"
#include "UI/TclSignInButton.h"
#include "UI/TclSignInController.h"
#include "UI/TclSignInState.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* NotConnected = TEXT("{\"platform\":\"twitch\",\"status\":\"disconnected\",\"needsReconnect\":false,\"missingPermissions\":false}");
}

/**
 * What a sign-in button shows and does for each state the Connections
 * component reports: the Connections widget's words, with one action per
 * stage. The same rules as Unity's and Godot's.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInStateTest,
	"Tentacle.SignInButton.StateFollowsTheConnection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInStateTest::RunTest(const FString& Parameters)
{
	using EStage = ETclSignInStage;
	const ETclStreamPlatform Twitch = ETclStreamPlatform::Twitch;

	const FTclSignInState App = FTclSignInState::Describe(Twitch, TEXT("Twitch and Kick connect through the Tentacle app"), nullptr, nullptr, 0.0, nullptr);
	TestTrue(TEXT("with the app, unavailable"), App.Stage == EStage::Unavailable);
	TestEqual(TEXT("saying why"), App.Status, FString(TEXT("Twitch and Kick connect through the Tentacle app")));
	TestEqual(TEXT("still reading Connect"), App.Label, FString(TEXT("Connect")));
	TestFalse(TEXT("and not pressable"), App.bCanPress);
	TestEqual(TEXT("for Twitch"), App.Title, FString(TEXT("Twitch")));

	const FTclSignInState Waiting = FTclSignInState::Describe(Twitch, FString(), nullptr, nullptr, 0.0, nullptr);
	TestTrue(TEXT("before Tentacle reports, waiting"), Waiting.Stage == EStage::Waiting);
	TestEqual(TEXT("waiting"), Waiting.Status, FString(TEXT("Waiting for Tentacle")));
	TestFalse(TEXT("not pressable yet"), Waiting.bCanPress);

	const FTclSignInState None = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(NotConnected), nullptr, 0.0, nullptr);
	TestTrue(TEXT("signed out"), None.Stage == EStage::NotConnected);
	TestEqual(TEXT("not connected"), None.Status, FString(TEXT("Not connected")));
	TestEqual(TEXT("offering to connect"), None.Label, FString(TEXT("Connect")));
	TestTrue(TEXT("pressable"), None.bCanPress);
	TestEqual(TEXT("with nothing to add"), None.Detail, FString());

	UTclPlatformConnectionResult* Denied = UTclPlatformConnectionResult::ParseFromString(GetTransientPackage(), TEXT("{\"platform\":\"twitch\",\"outcome\":\"denied\"}"));
	const FTclSignInState Declined = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(NotConnected), nullptr, 0.0, Denied);
	TestEqual(TEXT("a failed sign-in says why"), Declined.Detail, FString(TEXT("Sign-in was declined")));

	UTclPlatformConnectionPrompt* Prompt = UTclPlatformConnectionPrompt::ParseFromString(GetTransientPackage(), TclTest::TwitchConnectionsPromptJson());
	const FTclSignInState Signing = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(NotConnected), Prompt, 125.0, nullptr);
	TestTrue(TEXT("signing in"), Signing.Stage == EStage::SigningIn);
	TestEqual(TEXT("the browser's turn"), Signing.Status, FString(TEXT("Finish signing in in your browser")));
	TestEqual(TEXT("with the code and time left"), Signing.Detail, FString(TEXT("Code ABCD · expires in 2:05")));
	TestEqual(TEXT("offering to cancel"), Signing.Label, FString(TEXT("Cancel")));
	TestEqual(TEXT("the code"), Signing.UserCode, FString(TEXT("ABCD")));
	TestEqual(TEXT("the page"), Signing.Url, FString(TEXT("https://www.twitch.tv/activate?device-code=ABCD")));

	const FTclSignInState Connecting = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(TclTest::TwitchSignedInJson(TEXT("connecting"))), nullptr, 0.0, nullptr);
	TestTrue(TEXT("connecting"), Connecting.Stage == EStage::Connecting);
	TestEqual(TEXT("as the account"), Connecting.Status, FString(TEXT("Connecting as Streamer...")));
	TestEqual(TEXT("offering to disconnect"), Connecting.Label, FString(TEXT("Disconnect")));
	TestEqual(TEXT("the account's name"), Connecting.AccountName, FString(TEXT("Streamer")));

	const FTclSignInState Connected = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(TclTest::TwitchSignedInJson(TEXT("connected"))), nullptr, 0.0, nullptr);
	TestTrue(TEXT("connected"), Connected.Stage == EStage::Connected);
	TestEqual(TEXT("as the account"), Connected.Status, FString(TEXT("Connected as Streamer")));
	TestFalse(TEXT("with every permission"), Connected.bMissingPermissions);

	const FTclSignInState Older = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(TclTest::TwitchSignedInJson(TEXT("connected"), false, true)), nullptr, 0.0, nullptr);
	TestTrue(TEXT("still connected"), Older.Stage == EStage::Connected);
	TestTrue(TEXT("missing newer permissions"), Older.bMissingPermissions);
	TestEqual(TEXT("saying so"), Older.Detail, FString(TEXT("Sign in again to allow newer features")));

	const FTclSignInState Expired = FTclSignInState::Describe(Twitch, FString(), TclTest::ParseConnection(TclTest::TwitchSignedInJson(TEXT("disconnected"), true)), nullptr, 0.0, nullptr);
	TestTrue(TEXT("needs the player again"), Expired.Stage == EStage::NeedsReconnect);
	TestEqual(TEXT("saying so"), Expired.Status, FString(TEXT("Streamer needs to sign in again")));
	TestEqual(TEXT("offering to sign in again"), Expired.Label, FString(TEXT("Sign in again")));
	return true;
}

/**
 * A sign-in controller follows its platform and presses the way its label
 * says: connect, cancel while signing in, and disconnect once signed in. It
 * opens the page for a sign-in it started, and the countdown moves.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInControllerTest,
	"Tentacle.SignInButton.ControllerSignsInAndOut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInControllerTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Embedded);
	TclTest::FSentMessages Sent(Tentacle);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>();
	TArray<FString> Opened;
	double Now = 100.0;

	UTclSignInController* Controller = UTclSignInController::CreateSignInController(World.Get(), ETclStreamPlatform::Twitch, true);
	if (!TestNotNull(TEXT("the controller"), Controller)) return false;
	Controller->OpenUrl = [&Opened](const FString& Url) { Opened.Add(Url); };
	Controller->Clock = [&Now]() { return Now; };
	Controller->OnStateChanged.AddDynamic(Recorder, &UTclTestRecorder::OnSignInStateChanged);

	TestTrue(TEXT("it waits for Tentacle"), Controller->GetState().Stage == ETclSignInStage::Waiting);
	Controller->Press();
	TestEqual(TEXT("pressing while waiting sends nothing"), Sent.Num(), 0);

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Status"), NotConnected);
	TestEqual(TEXT("it says so"), Recorder->Names(), TArray<FString>{ TEXT("OnSignInStateChanged") });
	TestEqual(TEXT("with the stage and label"), Recorder->Calls.Last().Detail, FString(TEXT("NotConnected|Connect")));

	Controller->Press();
	TestEqual(TEXT("Connect asks Tentacle"), Sent.Type(0), FString(TEXT("Connections_Connect")));
	TestEqual(TEXT("for Twitch"), TclTest::StringField(Sent.Data(0), TEXT("platform")), FString(TEXT("twitch")));

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TEXT("{\"platform\":\"kick\",\"url\":\"https://id.kick.com/oauth/authorize\",\"expiresInSeconds\":600}"));
	TestEqual(TEXT("Kick's prompt is not its own"), Opened.Num(), 0);

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TclTest::TwitchConnectionsPromptJson());
	TestEqual(TEXT("it opens the page it asked for"), FString::Join(Opened, TEXT(" ")), FString(TEXT("https://www.twitch.tv/activate?device-code=ABCD")));
	TestEqual(TEXT("and offers to cancel"), Recorder->Calls.Last().Detail, FString(TEXT("SigningIn|Cancel")));
	TestEqual(TEXT("with the code"), Controller->GetState().Detail, FString(TEXT("Code ABCD · expires in 30:00")));

	const int32 Before = Recorder->Calls.Num();
	Now += 61.0;
	Controller->Refresh();
	TestEqual(TEXT("the countdown moves"), Controller->GetState().Detail, FString(TEXT("Code ABCD · expires in 28:59")));
	TestEqual(TEXT("and says so"), Recorder->Calls.Num(), Before + 1);
	Controller->Refresh();
	TestEqual(TEXT("but only when something changed"), Recorder->Calls.Num(), Before + 1);

	Controller->OpenPage();
	TestEqual(TEXT("Open Page opens it again"), Opened.Num(), 2);

	Controller->Press();
	TestEqual(TEXT("Cancel asks Tentacle"), Sent.Type(1), FString(TEXT("Connections_CancelConnect")));

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Result"), TEXT("{\"platform\":\"twitch\",\"outcome\":\"denied\"}"));
	TestTrue(TEXT("back to signed out"), Controller->GetState().Stage == ETclSignInStage::NotConnected);
	TestEqual(TEXT("saying why"), Controller->GetState().Detail, FString(TEXT("Sign-in was declined")));

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Status"), TclTest::TwitchSignedInJson(TEXT("connected")));
	TestEqual(TEXT("connected, offering to disconnect"), Recorder->Calls.Last().Detail, FString(TEXT("Connected|Disconnect")));
	Controller->Press();
	TestEqual(TEXT("Disconnect asks Tentacle"), Sent.Type(2), FString(TEXT("Connections_Disconnect")));

	Controller->SignIn();
	TestEqual(TEXT("Sign In asks Tentacle whatever the stage"), Sent.Type(3), FString(TEXT("Connections_Connect")));
	// Its clock reads this test's Now, so nothing may tick it afterwards.
	Controller->Stop();
	return true;
}

/**
 * A sign-in someone else started, the game's Sign In node say, shows on the
 * controller, but only the one that started a sign-in opens its page.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInControllerOthersTest,
	"Tentacle.SignInButton.OpensOnlyPagesItAskedFor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInControllerOthersTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Embedded);
	TArray<FString> Opened;

	UTclSignInController* Controller = UTclSignInController::CreateSignInController(World.Get(), ETclStreamPlatform::Twitch, true);
	if (!TestNotNull(TEXT("the controller"), Controller)) return false;
	Controller->OpenUrl = [&Opened](const FString& Url) { Opened.Add(Url); };

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Status"), NotConnected);
	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TclTest::TwitchConnectionsPromptJson());
	TestTrue(TEXT("it shows the sign-in"), Controller->GetState().Stage == ETclSignInStage::SigningIn);
	TestEqual(TEXT("but opens nothing"), Opened.Num(), 0);

	UTclSignInController* Quiet = UTclSignInController::CreateSignInController(World.Get(), ETclStreamPlatform::Twitch, false);
	if (!TestNotNull(TEXT("a second controller"), Quiet)) return false;
	Quiet->OpenUrl = [&Opened](const FString& Url) { Opened.Add(Url); };
	Quiet->SignIn();
	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TclTest::TwitchConnectionsPromptJson());
	TestEqual(TEXT("with Open Browser off, nothing opens"), Opened.Num(), 0);
	Quiet->OpenPage();
	TestEqual(TEXT("until the game asks"), Opened.Num(), 1);
	Controller->Stop();
	Quiet->Stop();
	return true;
}

/**
 * The Sign In Button is an ordinary button: pressing it does what its state
 * says, the Text Block inside reads the label, it is disabled while it cannot
 * be pressed, and On Sign In State Changed tells the game.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInButtonTest,
	"Tentacle.SignInButton.ButtonPressesAndKeepsItsLabel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInButtonTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Embedded);
	TclTest::FSentMessages Sent(Tentacle);
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>();

	UTclSignInButton* Button = NewObject<UTclSignInButton>(World.Get());
	Button->OnSignInStateChanged.AddDynamic(Recorder, &UTclTestRecorder::OnSignInStateChanged);
	// The label inside a box beside an icon, as a styled button often has it.
	UHorizontalBox* Box = NewObject<UHorizontalBox>(Button);
	UTextBlock* Label = NewObject<UTextBlock>(Box);
	Box->AddChild(Label);
	Button->AddChild(Box);
	// Held for the whole test: releasing it releases the button, which stops it listening.
	const TSharedRef<SWidget> Slate = Button->TakeWidget();
	TArray<FString> Opened;
	Button->GetSignInController()->OpenUrl = [&Opened](const FString& Url) { Opened.Add(Url); };

	TestEqual(TEXT("it reads Connect"), Label->GetText().ToString(), FString(TEXT("Connect")));
	TestFalse(TEXT("disabled while Tentacle starts"), Button->GetIsEnabled());

	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Status"), NotConnected);
	TestTrue(TEXT("enabled once it can sign in"), Button->GetIsEnabled());
	TestEqual(TEXT("On Sign In State Changed fires"), Recorder->Calls.Last().Detail, FString(TEXT("NotConnected|Connect")));

	Button->OnClicked.Broadcast();
	TestEqual(TEXT("a click signs in"), Sent.Type(0), FString(TEXT("Connections_Connect")));
	TclTest::ReceiveConnectionsMessage(Tentacle, TEXT("Connections_Prompt"), TclTest::TwitchConnectionsPromptJson());
	TestEqual(TEXT("it opens the page"), Opened.Num(), 1);
	TestEqual(TEXT("and reads Cancel"), Label->GetText().ToString(), FString(TEXT("Cancel")));
	TestTrue(TEXT("its state says so"), Button->GetSignInState().Stage == ETclSignInStage::SigningIn);

	Button->OnClicked.Broadcast();
	TestEqual(TEXT("a click cancels"), Sent.Type(1), FString(TEXT("Connections_CancelConnect")));

	UTclSignInButton* Plain = NewObject<UTclSignInButton>(World.Get());
	Plain->bSetLabel = false;
	UTextBlock* Own = NewObject<UTextBlock>(Plain);
	Own->SetText(FText::FromString(TEXT("Twitch")));
	Plain->AddChild(Own);
	const TSharedRef<SWidget> PlainSlate = Plain->TakeWidget();
	TestEqual(TEXT("with Set Label off, its text is the game's"), Own->GetText().ToString(), FString(TEXT("Twitch")));
	Button->GetSignInController()->Stop();
	Plain->GetSignInController()->Stop();
	return true;
}

/** With the actor in Paired mode, a sign-in button says the app connects the platforms, and cannot be pressed. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSignInButtonAppModeTest,
	"Tentacle.SignInButton.InAppModeTheAppConnects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSignInButtonAppModeTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	UTclSignInController* Lonely = UTclSignInController::CreateSignInController(World.Get(), ETclStreamPlatform::Kick, true);
	TestEqual(TEXT("with no Tentacle actor, it says to add one"), Lonely->GetState().Status, FString(TEXT("Add a Tentacle actor to the level to sign in here")));

	ATentacle* Tentacle = TclTest::SpawnTentacle(World.Get(), ETclTentacleMode::Paired);
	TclTest::FSentMessages Sent(Tentacle);
	UTclSignInButton* Button = NewObject<UTclSignInButton>(World.Get());
	Button->Platform = ETclStreamPlatform::Kick;
	const TSharedRef<SWidget> Slate = Button->TakeWidget();

	const FTclSignInState State = Button->GetSignInState();
	TestTrue(TEXT("unavailable"), State.Stage == ETclSignInStage::Unavailable);
	TestEqual(TEXT("for Kick"), State.Title, FString(TEXT("Kick")));
	TestEqual(TEXT("the note"), State.Status, FString(TEXT("Twitch and Kick connect through the Tentacle app")));
	TestFalse(TEXT("disabled"), Button->GetIsEnabled());
	Button->GetSignInController()->Press();
	TestEqual(TEXT("and a press sends nothing"), Sent.Num(), 0);
	return true;
}

#endif
