// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsPrompt.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsResult.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsStatus.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"
#include "Tests/TclTestSupport.h"
#include "UI/TclConnectionsWidget.h"
#include "UI/TclConnectionsWidgetRow.h"
#include "UObject/Package.h"

namespace
{
	UTclPlatformConnectionPrompt* Prompt(const FString& Json)
	{
		return UTclPlatformConnectionPrompt::ParseFromString(GetTransientPackage(), Json);
	}

	UTclPlatformConnectionResult* Result(const FString& Json)
	{
		return UTclPlatformConnectionResult::ParseFromString(GetTransientPackage(), Json);
	}
}

/**
 * What a platform's row says and offers for each state the Connections
 * component reports: the same rules and words as Unity's and Godot's.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclConnectionsWidgetRowTest,
	"Tentacle.ConnectionsWidget.RowFollowsTheConnection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclConnectionsWidgetRowTest::RunTest(const FString& Parameters)
{
	using EAction = ETclConnectionsWidgetAction;

	const FTclConnectionsWidgetRow Waiting = FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch, nullptr, nullptr, 0.0, nullptr, true);
	TestEqual(TEXT("waiting"), Waiting.Status, FString(TEXT("Waiting for Tentacle")));
	TestTrue(TEXT("with no button"), Waiting.PrimaryAction == EAction::None);

	const FTclConnectionsWidgetRow None = FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Kick,
		TclTest::ParseConnection(TEXT("{\"platform\":\"kick\",\"status\":\"disconnected\",\"needsReconnect\":false,\"missingPermissions\":false}")), nullptr, 0.0, nullptr, true);
	TestEqual(TEXT("Kick's title"), None.Title, FString(TEXT("Kick")));
	TestEqual(TEXT("not connected"), None.Status, FString(TEXT("Not connected")));
	TestEqual(TEXT("offers to connect"), None.PrimaryLabel, FString(TEXT("Connect")));

	const FTclConnectionsWidgetRow Connecting = FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch,
		TclTest::ParseConnection(TclTest::TwitchSignedInJson(TEXT("connecting"))), nullptr, 0.0, nullptr, true);
	TestEqual(TEXT("connecting"), Connecting.Status, FString(TEXT("Connecting as Streamer...")));
	TestTrue(TEXT("offers to disconnect"), Connecting.PrimaryAction == EAction::Disconnect);

	const FTclConnectionsWidgetRow Expired = FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch,
		TclTest::ParseConnection(TclTest::TwitchSignedInJson(TEXT("disconnected"), true)), nullptr, 0.0, nullptr, true);
	TestEqual(TEXT("needs the player again"), Expired.Status, FString(TEXT("Streamer needs to sign in again")));
	TestEqual(TEXT("offers to sign in again"), Expired.PrimaryLabel, FString(TEXT("Sign in again")));

	// A connection that lacks newer permissions still works: it stays connected,
	// and signing in again is offered beside Disconnect.
	const FString Outdated = TEXT("{\"platform\":\"twitch\",\"status\":\"connected\",\"needsReconnect\":false,\"missingPermissions\":true,\"account\":{\"userId\":\"1\",\"login\":\"streamer\",\"displayName\":\"Streamer\"}}");
	const FTclConnectionsWidgetRow Older = FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch, TclTest::ParseConnection(Outdated), nullptr, 0.0, nullptr, true);
	TestEqual(TEXT("still connected"), Older.Status, FString(TEXT("Connected as Streamer")));
	TestEqual(TEXT("says why to sign in again"), Older.Detail, FString(TEXT("Sign in again to allow newer features")));
	TestTrue(TEXT("can still be disconnected"), Older.PrimaryAction == EAction::Disconnect);
	TestTrue(TEXT("offers to sign in again"), Older.SecondaryAction == EAction::Connect);
	TestEqual(TEXT("labelled so"), Older.SecondaryLabel, FString(TEXT("Sign in again")));
	TestEqual(TEXT("why the last sign-in failed comes first"),
		FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch, TclTest::ParseConnection(Outdated), nullptr, 0.0, Result(TEXT("{\"platform\":\"twitch\",\"outcome\":\"denied\"}")), true).Detail,
		FString(TEXT("Sign-in was declined")));

	const UTclPlatformConnectionPrompt* Twitch = Prompt(TEXT("{\"platform\":\"twitch\",\"url\":\"https://example.test\",\"userCode\":\"ABCD-EFGH\",\"expiresInSeconds\":600}"));
	const FTclConnectionsWidgetRow Signing = FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch, nullptr, Twitch, 245.2, nullptr, true);
	TestEqual(TEXT("signing in"), Signing.Status, FString(TEXT("Finish signing in in your browser")));
	TestEqual(TEXT("the code and time left"), Signing.Detail, FString(TEXT("Code ABCD-EFGH · expires in 4:06")));
	TestEqual(TEXT("open the page again"), Signing.PrimaryLabel, FString(TEXT("Open the page again")));
	TestTrue(TEXT("or cancel"), Signing.SecondaryAction == EAction::Cancel);
	TestEqual(TEXT("nothing opened yet"),
		FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch, nullptr, Twitch, 59.0, nullptr, false).PrimaryLabel,
		FString(TEXT("Open the sign-in page")));

	const UTclPlatformConnection* Disconnected = TclTest::ParseConnection(TEXT("{\"platform\":\"twitch\",\"status\":\"disconnected\",\"needsReconnect\":false,\"missingPermissions\":false}"));
	auto DetailFor = [Disconnected](const TCHAR* Json)
	{
		return FTclConnectionsWidgetRow::Describe(ETclStreamPlatform::Twitch, Disconnected, nullptr, 0.0, Result(Json), true).Detail;
	};
	TestEqual(TEXT("declined"), DetailFor(TEXT("{\"platform\":\"twitch\",\"outcome\":\"denied\"}")), FString(TEXT("Sign-in was declined")));
	TestEqual(TEXT("ran out of time"), DetailFor(TEXT("{\"platform\":\"twitch\",\"outcome\":\"expired\"}")), FString(TEXT("Sign-in ran out of time")));
	TestEqual(TEXT("failed, saying why"), DetailFor(TEXT("{\"platform\":\"twitch\",\"outcome\":\"error\",\"message\":\"Twitch is down\"}")), FString(TEXT("Sign-in failed: Twitch is down")));
	TestEqual(TEXT("cancelling says nothing"), DetailFor(TEXT("{\"platform\":\"twitch\",\"outcome\":\"cancelled\"}")), FString());
	return true;
}

/**
 * The widget against a Tentacle actor in Embedded mode: a row per platform that
 * follows its connection, opens the sign-in page by itself, counts down, and
 * sends Connect, Cancel Connect, and Disconnect from its buttons.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclConnectionsWidgetTest,
	"Tentacle.ConnectionsWidget.SignsThePlayerIn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclConnectionsWidgetTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	if (!TestNotNull(TEXT("Tentacle actor"), Tentacle)) return false;
	Tentacle->EditorMode = Tentacle->BuildMode = ETclTentacleMode::Embedded;
	Tentacle->bAutoConnect = false;
	Tentacle->DispatchBeginPlay();
	TclTest::FSentMessages Sent(Tentacle);

	UTclConnectionsWidget* Widget = CreateWidget<UTclConnectionsWidget>(World.Get());
	if (!TestNotNull(TEXT("the widget"), Widget)) return false;
	TArray<FString> Opened;
	double Now = 100.0;
	Widget->OpenUrl = [&Opened](const FString& Url) { Opened.Add(Url); };
	Widget->Clock = [&Now]() { return Now; };
	// Held for the whole test: releasing the Slate widget destructs the widget,
	// which stops it listening, as removing it from the screen would.
	const TSharedRef<SWidget> Slate = Widget->TakeWidget();

	auto TextOf = [Widget](const TCHAR* Name)
	{
		const UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(Name));
		return Text != nullptr ? Text->GetText().ToString() : FString(TEXT("<missing>"));
	};
	auto Press = [Widget](const TCHAR* Name)
	{
		if (UButton* Button = Cast<UButton>(Widget->GetWidgetFromName(Name)))
			Button->OnClicked.Broadcast();
	};
	auto Receive = [Tentacle](const FString& Type, const FString& Data)
	{
		const FString Json = FString::Printf(TEXT("{\"type\":\"%s\",\"data\":%s}"), *Type, *Data);
		if (Type == TEXT("Connections_Status"))
			Tentacle->Connections->HandleMsg_Status(TclTest::ParseMessage<UTclMsg_ServerToClient_ConnectionsStatus>(Tentacle, Json));
		else if (Type == TEXT("Connections_Prompt"))
			Tentacle->Connections->HandleMsg_Prompt(TclTest::ParseMessage<UTclMsg_ServerToClient_ConnectionsPrompt>(Tentacle, Json));
		else
			Tentacle->Connections->HandleMsg_Result(TclTest::ParseMessage<UTclMsg_ServerToClient_ConnectionsResult>(Tentacle, Json));
	};

	TestEqual(TEXT("Twitch's row waits"), TextOf(TEXT("TwitchStatus")), FString(TEXT("Waiting for Tentacle")));
	TestEqual(TEXT("and so does Kick's"), TextOf(TEXT("KickStatus")), FString(TEXT("Waiting for Tentacle")));

	Receive(TEXT("Connections_Status"), TEXT("{\"platform\":\"twitch\",\"status\":\"disconnected\",\"needsReconnect\":false,\"missingPermissions\":false}"));
	TestEqual(TEXT("not connected"), TextOf(TEXT("TwitchStatus")), FString(TEXT("Not connected")));
	Press(TEXT("TwitchPrimary"));
	TestEqual(TEXT("Connect asks Tentacle"), Sent.Type(0), FString(TEXT("Connections_Connect")));
	TestEqual(TEXT("for Twitch"), TclTest::StringField(Sent.Data(0), TEXT("platform")), FString(TEXT("twitch")));

	Receive(TEXT("Connections_Prompt"), TEXT("{\"platform\":\"twitch\",\"url\":\"https://www.twitch.tv/activate?device-code=ABCD\",\"userCode\":\"ABCD\",\"expiresInSeconds\":1800}"));
	TestEqual(TEXT("the page opens by itself"), FString::Join(Opened, TEXT(" ")), FString(TEXT("https://www.twitch.tv/activate?device-code=ABCD")));
	TestEqual(TEXT("signing in"), TextOf(TEXT("TwitchStatus")), FString(TEXT("Finish signing in in your browser")));
	TestEqual(TEXT("with the code"), TextOf(TEXT("TwitchDetail")), FString(TEXT("Code ABCD · expires in 30:00")));

	Now += 61.0;
	Press(TEXT("TwitchPrimary"));
	TestEqual(TEXT("Open the page again opens it again"), Opened.Num(), 2);
	TestEqual(TEXT("the countdown moves"), TextOf(TEXT("TwitchDetail")), FString(TEXT("Code ABCD · expires in 28:59")));

	Press(TEXT("TwitchSecondary"));
	TestEqual(TEXT("Cancel asks Tentacle"), Sent.Type(1), FString(TEXT("Connections_CancelConnect")));

	Receive(TEXT("Connections_Result"), TEXT("{\"platform\":\"twitch\",\"outcome\":\"denied\"}"));
	TestEqual(TEXT("back to not connected"), TextOf(TEXT("TwitchStatus")), FString(TEXT("Not connected")));
	TestEqual(TEXT("saying why"), TextOf(TEXT("TwitchDetail")), FString(TEXT("Sign-in was declined")));

	Receive(TEXT("Connections_Status"), TclTest::TwitchSignedInJson(TEXT("connected")));
	TestEqual(TEXT("connected"), TextOf(TEXT("TwitchStatus")), FString(TEXT("Connected as Streamer")));
	TestEqual(TEXT("offering to disconnect"), TextOf(TEXT("TwitchPrimaryLabel")), FString(TEXT("Disconnect")));
	Press(TEXT("TwitchPrimary"));
	TestEqual(TEXT("Disconnect asks Tentacle"), Sent.Type(2), FString(TEXT("Connections_Disconnect")));

	Widget->RemoveFromParent();
	return true;
}

/** With the actor in Paired mode, the widget says the Tentacle app connects the platforms. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclConnectionsWidgetAppModeTest,
	"Tentacle.ConnectionsWidget.InAppModeTheAppConnects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclConnectionsWidgetAppModeTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	if (!TestNotNull(TEXT("Tentacle actor"), Tentacle)) return false;
	Tentacle->bAutoConnect = false;
	Tentacle->DispatchBeginPlay();

	UTclConnectionsWidget* Widget = CreateWidget<UTclConnectionsWidget>(World.Get());
	if (!TestNotNull(TEXT("the widget"), Widget)) return false;
	const TSharedRef<SWidget> Slate = Widget->TakeWidget();

	const UTextBlock* Note = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("Note")));
	TestEqual(TEXT("the note"), Note ? Note->GetText().ToString() : FString(), FString(TEXT("Twitch and Kick connect through the Tentacle app")));
	TestTrue(TEXT("shown"), Note != nullptr && Note->GetVisibility() != ESlateVisibility::Collapsed);
	return true;
}

#endif
