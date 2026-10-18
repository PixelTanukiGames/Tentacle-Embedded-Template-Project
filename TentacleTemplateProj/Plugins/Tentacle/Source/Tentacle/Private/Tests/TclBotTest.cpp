// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Bot.h"
#include "Base/Tentacle_CustomEvents.h"
#include "Sdk/TclBotCommand.h"
#include "Sdk/TclBotCommandAction_Announce.h"
#include "Sdk/TclBotCommandAction_Timeout.h"
#include "Sdk/TclCustomEventDefinition.h"
#include "Sdk/TclMsg_ServerToClient_BotCommands.h"
#include "Sdk/TclMsg_ServerToClient_CustomEventsDefinitions.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** Deliver a command list the way one arrives from Tentacle. */
	void SendBotCommands(ATentacle* Tentacle, const FString& Json)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_BotCommands>(Tentacle, Json);
		Tentacle->Bot->HandleMsg_Commands(Msg, Json);
	}

	/** Deliver custom event definitions the way they arrive from Tentacle. */
	void SendDefinitions(ATentacle* Tentacle, const FString& Json)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_CustomEventsDefinitions>(Tentacle, Json);
		Tentacle->CustomEvents->HandleMsg_Definitions(Msg, Json);
	}
}

/**
 * The bot command list Tentacle sends is held whole - every command with its
 * configuration and actions, and the bot-wide settings - and On Commands
 * Changed says so.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclBotHoldsTheCommandsTest,
	"Tentacle.Bot.HoldsTheCommandsTentacleSends",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclBotHoldsTheCommandsTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	UTentacle_Bot* Bot = Tentacle->Bot;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	Bot->OnCommandsChanged.AddDynamic(Recorder, &UTclTestRecorder::OnCommandsChanged);

	TestFalse(TEXT("nothing is loaded before a list arrives"), Bot->bAreCommandsLoaded);
	TestEqual(TEXT("no commands before a list arrives"), Bot->Commands.Num(), 0);

	const FString Timeout = TclTest::BotCommandJson(TEXT("timeout"),
		TEXT("\"actions\":[{\"type\":\"timeout\",\"durationSeconds\":60,\"reason\":\"chat said so\"},")
		TEXT("{\"type\":\"announce\",\"message\":\"bye\",\"color\":\"purple\"}],")
		TEXT("\"permission\":\"moderator\",\"platforms\":[\"twitch\"],\"userCooldownSeconds\":10,\"minAccountAgeDays\":7"));
	SendBotCommands(Tentacle, TclTest::BotCommandsMessage(TclTest::BotCommandJson(TEXT("dice")) + TEXT(",") + Timeout));

	TestEqual(TEXT("On Commands Changed fired once"), Recorder->Count(TEXT("OnCommandsChanged")), 1);
	TestTrue(TEXT("the list is loaded"), Bot->bAreCommandsLoaded);
	TestTrue(TEXT("the bot is enabled"), Bot->bIsEnabled);
	TestEqual(TEXT("the command prefix"), Bot->CommandPrefix, FString(TEXT("!")));
	TestEqual(TEXT("the bot-wide cooldown"), Bot->GlobalCooldownSeconds, 5);
	TestEqual(TEXT("both commands"), Bot->Commands.Num(), 2);

	bool bFound = false;
	const UTclBotCommand* Command = Bot->FindCommandByName(TEXT("timeout"), bFound);
	if (!TestTrue(TEXT("the timeout command is found"), bFound && Command != nullptr)) return false;
	TestEqual(TEXT("its permission"), Command->Permission, ETclBotCommandPermission::Moderator);
	TestEqual(TEXT("its platforms"), Command->Platforms, TArray<ETclStreamPlatform>{ ETclStreamPlatform::Twitch });
	TestEqual(TEXT("its user cooldown"), Command->UserCooldownSeconds, 10);
	TestEqual(TEXT("its minimum account age"), Command->MinAccountAgeDays, 7);
	TestEqual(TEXT("both actions"), Command->Actions.Num(), 2);

	const auto* TimeoutAction = Command->Actions.Num() > 0 ? Cast<UTclBotCommandAction_Timeout>(Command->Actions[0]) : nullptr;
	TestTrue(TEXT("the timeout action, typed"), TimeoutAction && TimeoutAction->DurationSeconds == 60 && TimeoutAction->Reason == TEXT("chat said so"));
	const auto* AnnounceAction = Command->Actions.Num() > 1 ? Cast<UTclBotCommandAction_Announce>(Command->Actions[1]) : nullptr;
	// An optional enum arrives as its wire string, empty when absent.
	TestTrue(TEXT("the announce action, typed"), AnnounceAction && AnnounceAction->Message == TEXT("bye") && AnnounceAction->Color == TEXT("purple"));

	return true;
}

/**
 * A new list replaces the last, so a removed command is gone; a list sent again
 * unchanged, as one is every time the session opens, fires nothing.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclBotReplacesTheCommandsTest,
	"Tentacle.Bot.ReplacesTheCommandsAndIgnoresAListSentAgain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclBotReplacesTheCommandsTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	UTentacle_Bot* Bot = Tentacle->Bot;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	Bot->OnCommandsChanged.AddDynamic(Recorder, &UTclTestRecorder::OnCommandsChanged);

	const FString Both = TclTest::BotCommandsMessage(TclTest::BotCommandJson(TEXT("dice")) + TEXT(",") + TclTest::BotCommandJson(TEXT("hug")));
	SendBotCommands(Tentacle, Both);
	SendBotCommands(Tentacle, Both);
	TestEqual(TEXT("the same list twice fires once"), Recorder->Count(TEXT("OnCommandsChanged")), 1);

	SendBotCommands(Tentacle, TclTest::BotCommandsMessage(TclTest::BotCommandJson(TEXT("hug")), false));
	TestEqual(TEXT("a changed list fires again"), Recorder->Count(TEXT("OnCommandsChanged")), 2);
	TestEqual(TEXT("the removed command is gone"), Bot->Commands.Num(), 1);
	TestFalse(TEXT("the bot is now off"), Bot->bIsEnabled);

	bool bFound = true;
	TestNull(TEXT("dice is no longer found"), Bot->FindCommandByName(TEXT("dice"), bFound));
	TestFalse(TEXT("and says so"), bFound);

	return true;
}

/** Find Command By Name matches case-insensitively unless asked, as every name lookup does. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclBotFindCommandTest,
	"Tentacle.Bot.FindCommandByNameIgnoresCaseUnlessAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclBotFindCommandTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	SendBotCommands(Tentacle, TclTest::BotCommandsMessage(TclTest::BotCommandJson(TEXT("Dice"))));

	bool bFound = false;
	TestNotNull(TEXT("another casing is found by default"), Tentacle->Bot->FindCommandByName(TEXT("dice"), bFound));
	TestTrue(TEXT("and says so"), bFound);
	TestNull(TEXT("another casing is not found when case sensitive"), Tentacle->Bot->FindCommandByName(TEXT("dice"), bFound, true));
	TestNotNull(TEXT("the exact name is found when case sensitive"), Tentacle->Bot->FindCommandByName(TEXT("Dice"), bFound, true));
	TestNull(TEXT("the empty name is never found"), Tentacle->Bot->FindCommandByName(TEXT(""), bFound));

	return true;
}

/**
 * The custom event definitions Tentacle sends are held with the chat commands
 * that fire each, replaced by the next set, found by name under the Subscribe
 * case rule, and a set sent again unchanged fires nothing.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomEventDefinitionsTest,
	"Tentacle.CustomEvents.HoldsTheDefinitionsTentacleSends",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomEventDefinitionsTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	UTentacle_CustomEvents* CustomEvents = Tentacle->CustomEvents;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	CustomEvents->OnDefinitionsChanged.AddDynamic(Recorder, &UTclTestRecorder::OnDefinitionsChanged);
	CustomEvents->OnCustomEvent.AddDynamic(Recorder, &UTclTestRecorder::OnCustomEvent);

	TestFalse(TEXT("nothing is loaded before definitions arrive"), CustomEvents->bAreDefinitionsLoaded);

	const FString Both = TclTest::CustomEventDefinitionsMessage(
		TclTest::CustomEventDefinitionJson(TEXT("Jump"), TEXT("\"jump\",\"hop\"")) + TEXT(",") + TclTest::CustomEventDefinitionJson(TEXT("Quake")));
	SendDefinitions(Tentacle, Both);
	SendDefinitions(Tentacle, Both);

	TestEqual(TEXT("the same definitions twice fire once"), Recorder->Count(TEXT("OnDefinitionsChanged")), 1);
	TestEqual(TEXT("definitions are not custom events"), Recorder->Count(TEXT("OnCustomEvent")), 0);
	TestTrue(TEXT("the definitions are loaded"), CustomEvents->bAreDefinitionsLoaded);
	TestEqual(TEXT("both definitions"), CustomEvents->Definitions.Num(), 2);

	bool bFound = false;
	const UTclCustomEventDefinition* Jump = CustomEvents->FindDefinitionByName(TEXT("jump"), bFound);
	if (!TestTrue(TEXT("Jump is found under another casing"), bFound && Jump != nullptr)) return false;
	TestEqual(TEXT("with the chat commands that fire it"), Jump->ChatCommands, TArray<FString>{ TEXT("jump"), TEXT("hop") });
	TestNull(TEXT("not when case sensitive"), CustomEvents->FindDefinitionByName(TEXT("jump"), bFound, true));

	SendDefinitions(Tentacle, TclTest::CustomEventDefinitionsMessage(TclTest::CustomEventDefinitionJson(TEXT("Quake"))));
	TestEqual(TEXT("changed definitions fire again"), Recorder->Count(TEXT("OnDefinitionsChanged")), 2);
	TestNull(TEXT("the removed definition is gone"), CustomEvents->FindDefinitionByName(TEXT("Jump"), bFound));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
