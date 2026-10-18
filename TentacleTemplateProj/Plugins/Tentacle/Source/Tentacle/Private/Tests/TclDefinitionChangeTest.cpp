// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/CriticalSection.h"
#include "Misc/OutputDevice.h"
#include "Misc/ScopeLock.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Obs.h"
#include "Base/Tentacle_Timers.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Sdk/TclViewerPropertyCollection.h"
#include "Sdk/TclViewerPropertyDefinition.h"
#include "Sdk/TclViewerPropertyType.h"
#include "Base/TclDefinitionChangeInternal.h"
#include "Base/TclTimerSettings.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/TclMsg_ServerToClient_TimersDefinitions.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** A Tentacle actor, not begun play, and a recorder its changes report to. */
	struct FChangeFixture
	{
		ATentacle* Tentacle = nullptr;
		UTclTestRecorder* Recorder = nullptr;

		/** Keep both alive while a test waits across frames. */
		TStrongObjectPtr<ATentacle> KeepTentacle;
		TStrongObjectPtr<UTclTestRecorder> KeepRecorder;

		FChangeFixture()
		{
			UObject* Outer = GetTransientPackage();
			Tentacle = NewObject<ATentacle>(Outer);
			Recorder = NewObject<UTclTestRecorder>(Outer);
			KeepTentacle.Reset(Tentacle);
			KeepRecorder.Reset(Recorder);
		}

		/** A completion that records what it came to. */
		FTclOnDefinitionComplete OnComplete() const
		{
			FTclOnDefinitionComplete Result;
			Result.BindUFunction(Recorder, GET_FUNCTION_NAME_CHECKED(UTclTestRecorder, OnDefinitionComplete));
			return Result;
		}

		/** Deliver timer definitions for these names, as Tentacle sends them. */
		void SendTimers(const TArray<FString>& Names) const
		{
			TArray<FString> Definitions;
			for (const FString& Name : Names)
				Definitions.Add(TclTest::TimerDefinitionJson(Name));
			const FString Json = TclTest::TimerDefinitionsMessage(Definitions);
			Tentacle->Timers->HandleMsg_Definitions(TclTest::ParseMessage<UTclMsg_ServerToClient_TimersDefinitions>(Tentacle, Json), Json);
		}

		/** Load a registry with the collection Game (C1) holding the int property Score (D1). */
		void LoadRegistry() const
		{
			UObject* Outer = GetTransientPackage();
			UTclViewerPropertyCollection* Game = NewObject<UTclViewerPropertyCollection>(Outer);
			Game->Id = TEXT("C1");
			Game->Name = TEXT("Game");

			UTclViewerPropertyDefinition* Score = NewObject<UTclViewerPropertyDefinition>(Outer);
			Score->Id = TEXT("D1");
			Score->CollectionId = TEXT("C1");
			Score->Name = TEXT("Score");
			Score->Type = ETclViewerPropertyType::Int;
			Score->Value = TEXT("0");

			Tentacle->ViewerProperties->ApplyRegistry({ Game }, { Score });
		}

		/** What each completion so far came to, in order. */
		FString Results() const
		{
			TArray<FString> Details;
			for (const FTclRecordedCall& Call : Recorder->Calls)
				Details.Add(Call.Detail.Replace(TEXT("ETclDefinitionResult::"), TEXT("")));
			return FString::Join(Details, TEXT(", "));
		}
	};

	/** Everything logged while it is registered, from any thread. */
	class FLogCapture : public FOutputDevice
	{
	public:
		FLogCapture() { GLog->AddOutputDevice(this); }
		virtual ~FLogCapture() override { GLog->RemoveOutputDevice(this); }

		virtual void Serialize(const TCHAR* Message, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			FScopeLock Lock(&Guard);
			Text += Message;
			Text += TEXT("\n");
		}

		virtual bool CanBeUsedOnAnyThread() const override { return true; }

		/** Whether anything logged so far contains Needle. */
		bool Contains(const FString& Needle) const
		{
			FScopeLock Lock(&Guard);
			return Text.Contains(Needle);
		}

	private:
		mutable FCriticalSection Guard;
		FString Text;
	};
}

/**
 * A change reports what it came to by its HTTP status: a 404 is Not Found, a
 * 409 Name Taken, and anything else, no answer included, Failed.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclDefinitionChangeStatusTest,
	"Tentacle.DefinitionChanges.StatusComesToAResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclDefinitionChangeStatusTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("404"), FTclDefinitionChange::ResultForStatus(404), ETclDefinitionResult::NotFound);
	TestEqual(TEXT("409"), FTclDefinitionChange::ResultForStatus(409), ETclDefinitionResult::NameTaken);
	TestEqual(TEXT("400"), FTclDefinitionChange::ResultForStatus(400), ETclDefinitionResult::Failed);
	TestEqual(TEXT("500"), FTclDefinitionChange::ResultForStatus(500), ETclDefinitionResult::Failed);
	TestEqual(TEXT("no answer"), FTclDefinitionChange::ResultForStatus(0), ETclDefinitionResult::Failed);

	// Nothing bound is nothing to run.
	FTclDefinitionChange::Finish(FTclOnDefinitionComplete(), ETclDefinitionResult::Success);
	return true;
}

/**
 * Timer settings go out as Tentacle's timers.create and timers.update read
 * them: every setting, with the defaults the other plugins use, and the name
 * only when there is one, so an update with none keeps the timer's own.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclTimerSettingsInputTest,
	"Tentacle.DefinitionChanges.TimerSettingsInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclTimerSettingsInputTest::RunTest(const FString& Parameters)
{
	const FTclTimerSettings Defaults;
	const TSharedRef<FJsonObject> Input = Defaults.ToInput();
	TestFalse(TEXT("an empty name is left out"), Input->HasField(TEXT("name")));
	TestTrue(TEXT("repeating"), Input->GetBoolField(TEXT("repeating")));
	TestTrue(TEXT("only while live"), Input->GetBoolField(TEXT("onlyWhileLive")));
	TestEqual(TEXT("shortest interval"), Input->GetIntegerField(TEXT("intervalMinSeconds")), 600);
	TestEqual(TEXT("longest interval"), Input->GetIntegerField(TEXT("intervalMaxSeconds")), 600);
	TestEqual(TEXT("chat minimum"), Input->GetIntegerField(TEXT("minChatMessages")), 0);
	TestEqual(TEXT("nothing else"), Input->Values.Num(), 5);

	FTclTimerSettings Named;
	Named.Name = TEXT("Boss");
	Named.bRepeating = false;
	Named.IntervalMinSeconds = 30;
	Named.IntervalMaxSeconds = 90;
	Named.MinChatMessages = 4;
	const TSharedRef<FJsonObject> NamedInput = Named.ToInput();
	TestEqual(TEXT("the name"), NamedInput->GetStringField(TEXT("name")), FString(TEXT("Boss")));
	TestFalse(TEXT("not repeating"), NamedInput->GetBoolField(TEXT("repeating")));
	TestEqual(TEXT("its shortest interval"), NamedInput->GetIntegerField(TEXT("intervalMinSeconds")), 30);
	TestEqual(TEXT("its longest interval"), NamedInput->GetIntegerField(TEXT("intervalMaxSeconds")), 90);
	TestEqual(TEXT("its chat minimum"), NamedInput->GetIntegerField(TEXT("minChatMessages")), 4);
	return true;
}

/**
 * A change to a timer finds it by name first, so one Tentacle has not sent
 * fails before any request: Failed until the timers arrive, since nothing is
 * known about the name then, and Not Found after. Names match ignoring case
 * unless the call asks otherwise.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclTimerChangesResolveTest,
	"Tentacle.DefinitionChanges.TimerChangesFindTheTimerFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclTimerChangesResolveTest::RunTest(const FString& Parameters)
{
	FChangeFixture Fixture;
	UTentacle_Timers* Timers = Fixture.Tentacle->Timers;

	AddExpectedError(TEXT("the timers have not arrived from Tentacle yet"), EAutomationExpectedErrorFlags::Contains, 2);
	Timers->StartTimer(TEXT("Boss"), Fixture.OnComplete());
	Timers->UpdateTimer(TEXT("Boss"), FTclTimerSettings(), Fixture.OnComplete());
	TestEqual(TEXT("before the timers arrive"), Fixture.Results(), FString(TEXT("Failed, Failed")));

	Fixture.SendTimers({ TEXT("Boss") });
	Fixture.Recorder->Calls.Reset();
	Timers->StartTimer(TEXT("Loot"), Fixture.OnComplete());
	Timers->PauseTimer(TEXT("Loot"), Fixture.OnComplete());
	Timers->ResetTimer(TEXT("Loot"), Fixture.OnComplete());
	Timers->FireTimer(TEXT("Loot"), Fixture.OnComplete());
	Timers->DeleteTimer(TEXT("Loot"), Fixture.OnComplete());
	Timers->UpdateTimer(TEXT("Loot"), FTclTimerSettings(), Fixture.OnComplete());
	Timers->StartTimer(TEXT("boss"), Fixture.OnComplete(), true);
	TestEqual(TEXT("a name Tentacle does not have"), Fixture.Results(),
		FString(TEXT("NotFound, NotFound, NotFound, NotFound, NotFound, NotFound, NotFound")));

	// Found, ignoring case, it goes on to Tentacle, which this actor has no
	// connection to until it begins play.
	Fixture.Recorder->Calls.Reset();
	AddExpectedError(TEXT("there is no connection to Tentacle yet"), EAutomationExpectedErrorFlags::Contains, 3);
	Timers->StartTimer(TEXT("boss"), Fixture.OnComplete());
	Timers->CreateTimer(FTclTimerSettings(), Fixture.OnComplete());
	Fixture.Tentacle->Obs->SetConnectionSettings(TEXT(""), 4455, TEXT(""), Fixture.OnComplete());
	TestEqual(TEXT("with no connection"), Fixture.Results(), FString(TEXT("Failed, Failed, Failed")));

	// A change nobody waits on is still made.
	Fixture.Recorder->Calls.Reset();
	Timers->StartTimer(TEXT("Loot"), FTclOnDefinitionComplete());
	TestEqual(TEXT("nothing bound, nothing recorded"), Fixture.Recorder->Calls.Num(), 0);
	return true;
}

/**
 * A change to the viewer property definitions finds its collection and property
 * through the registry first, so one it cannot find fails before any request:
 * Failed until the registry loads, and Not Found after. A default of another
 * type than the property's is Failed without a request too, since Tentacle
 * would store it as given.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyChangesResolveTest,
	"Tentacle.DefinitionChanges.ViewerPropertyChangesFindTheirTargetFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyChangesResolveTest::RunTest(const FString& Parameters)
{
	FChangeFixture Fixture;
	UTentacle_ViewerProperties* Properties = Fixture.Tentacle->ViewerProperties;

	AddExpectedError(TEXT("the registry has not loaded yet"), EAutomationExpectedErrorFlags::Contains, 2);
	Properties->RenameCollection(TEXT("Game"), TEXT("Arena"), Fixture.OnComplete());
	Properties->DeleteProperty(TEXT("Game"), TEXT("Score"), Fixture.OnComplete());
	TestEqual(TEXT("before the registry loads"), Fixture.Results(), FString(TEXT("Failed, Failed")));

	Fixture.LoadRegistry();
	Fixture.Recorder->Calls.Reset();
	Properties->RenameCollection(TEXT("Nowhere"), TEXT("Arena"), Fixture.OnComplete());
	Properties->DeleteCollection(TEXT("game"), Fixture.OnComplete(), true);
	Properties->CreateProperty_Int(TEXT("Nowhere"), TEXT("Kills"), 0, Fixture.OnComplete());
	Properties->SetPropertyDefault_Int(TEXT("Game"), TEXT("Kills"), 1, Fixture.OnComplete());
	Properties->RenameProperty(TEXT("Game"), TEXT("Kills"), TEXT("Deaths"), Fixture.OnComplete());
	Properties->DeleteProperty(TEXT("Game"), TEXT("score"), Fixture.OnComplete(), true);
	TestEqual(TEXT("names the registry does not have"), Fixture.Results(),
		FString(TEXT("NotFound, NotFound, NotFound, NotFound, NotFound, NotFound")));

	Fixture.Recorder->Calls.Reset();
	AddExpectedError(TEXT("so its default cannot be set as"), EAutomationExpectedErrorFlags::Contains, 3);
	Properties->SetPropertyDefault_String(TEXT("Game"), TEXT("Score"), TEXT("ten"), Fixture.OnComplete());
	Properties->SetPropertyDefault_Float(TEXT("Game"), TEXT("Score"), 1.5, Fixture.OnComplete());
	Properties->SetPropertyDefault_Bool(TEXT("Game"), TEXT("Score"), true, Fixture.OnComplete());
	TestEqual(TEXT("a default of another type"), Fixture.Results(), FString(TEXT("Failed, Failed, Failed")));

	// Found, ignoring case, each goes on to Tentacle, which this actor has no
	// connection to until it begins play.
	Fixture.Recorder->Calls.Reset();
	AddExpectedError(TEXT("there is no connection to Tentacle yet"), EAutomationExpectedErrorFlags::Contains, 11);
	Properties->CreateCollection(TEXT("Loot"), Fixture.OnComplete());
	Properties->RenameCollection(TEXT("game"), TEXT("Arena"), Fixture.OnComplete());
	Properties->DeleteCollection(TEXT("GAME"), Fixture.OnComplete());
	Properties->CreateProperty_Int(TEXT("game"), TEXT("Kills"), 3, Fixture.OnComplete());
	Properties->CreateProperty_Float(TEXT("game"), TEXT("Speed"), 1.5, Fixture.OnComplete());
	Properties->CreateProperty_Bool(TEXT("game"), TEXT("Vip"), true, Fixture.OnComplete());
	Properties->CreateProperty_String(TEXT("game"), TEXT("Title"), TEXT("rookie"), Fixture.OnComplete());
	Properties->SetPropertyDefault_Int(TEXT("game"), TEXT("SCORE"), 10, Fixture.OnComplete());
	Properties->RenameProperty(TEXT("game"), TEXT("score"), TEXT("Points"), Fixture.OnComplete());
	Properties->DeleteProperty(TEXT("Game"), TEXT("Score"), Fixture.OnComplete());
	Properties->CreateCollection(TEXT("Loot"), FTclOnDefinitionComplete());
	TestEqual(TEXT("with no connection"), Fixture.Results(),
		FString(TEXT("Failed, Failed, Failed, Failed, Failed, Failed, Failed, Failed, Failed, Failed")));

	bool bReloaded = false;
	Properties->ReloadRegistry([&bReloaded]() { bReloaded = true; });
	TestTrue(TEXT("a reload with no connection finishes at once"), bReloaded);
	return true;
}

/**
 * A change Tentacle never answers comes to Failed, once for each call: the
 * requests go to a loopback port nothing listens on, which is what a closed
 * Tentacle looks like from the game. The failures are logged, but never the
 * OBS password: users paste their logs into bug reports.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclDefinitionChangeUnansweredTest,
	"Tentacle.DefinitionChanges.AnUnansweredChangeFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclDefinitionChangeUnansweredTest::RunTest(const FString& Parameters)
{
	TSharedRef<FChangeFixture> Fixture = MakeShared<FChangeFixture>();
	UTentacleHttpClient* Http = NewObject<UTentacleHttpClient>(Fixture->Tentacle);
	Http->Configure(TEXT("http://127.0.0.1:1"), TEXT(""));
	Fixture->Tentacle->HttpClient = Http;
	Fixture->SendTimers({ TEXT("Boss") });

	// The HTTP client and the change each log the failure, and so does the
	// engine's HTTP module, line by line from libcurl.
	AddExpectedError(TEXT("HTTP request failed"), EAutomationExpectedErrorFlags::Contains, 0);
	AddExpectedError(TEXT("Connection failed"), EAutomationExpectedErrorFlags::Contains, 0);
	AddExpectedError(TEXT("libcurl|127\\.0\\.0\\.1"), EAutomationExpectedErrorFlags::Contains, 0);
	TSharedRef<FLogCapture> Log = MakeShared<FLogCapture>();

	FTclTimerSettings Settings;
	Settings.Name = TEXT("Loot");
	Fixture->Tentacle->Timers->CreateTimer(Settings, Fixture->OnComplete());
	Fixture->Tentacle->Timers->FireTimer(TEXT("boss"), Fixture->OnComplete());
	Fixture->Tentacle->Obs->SetConnectionSettings(TEXT("192.168.1.20"), 4455, TEXT("hunter2"), Fixture->OnComplete());
	Fixture->LoadRegistry();
	Fixture->Tentacle->ViewerProperties->CreateProperty_Int(TEXT("Game"), TEXT("Kills"), 0, Fixture->OnComplete());

	ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
		[Fixture]()
		{
			return Fixture->Recorder->Calls.Num() >= 4;
		},
		[this, Fixture]()
		{
			AddError(FString::Printf(TEXT("only %d of 4 changes completed: %s"), Fixture->Recorder->Calls.Num(), *Fixture->Results()));
			return true;
		},
		15.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand(
		[this, Fixture, Log]()
		{
			TestEqual(TEXT("each change fails once"), Fixture->Results(), FString(TEXT("Failed, Failed, Failed, Failed")));
			TestTrue(TEXT("the failures are logged"), Log->Contains(TEXT("obs.setConnectionSettings")));
			TestFalse(TEXT("the OBS password is not"), Log->Contains(TEXT("hunter2")));
			return true;
		}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
