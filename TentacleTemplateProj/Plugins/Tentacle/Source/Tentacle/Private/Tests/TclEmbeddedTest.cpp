// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "HAL/FileManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Utilities/TclConfigFiles.h"
#include "Utilities/TclSidecarEndpoints.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "Base/Tentacle_Timers.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Base/TclTimerSettings.h"
#include "Embedded/TclConnectionProcedures.h"
#include "Embedded/TclCoreLibrary.h"
#include "Embedded/TclEmbeddedCore.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclTimerDefinition.h"
#include "Sdk/PlatformApi/TclKickApiActions.h"
#include "Sdk/PlatformApi/TclTwitchApiActions.h"
#include "Utilities/TclEmbeddedOptions.h"
#include "Utilities/TclTentacleMode.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** The procedures carrier over a recorded session that can be opened and closed. */
	struct FProceduresFixture
	{
		TArray<FString> Sent;
		bool bSessionOpen = true;
		TSharedRef<FTclConnectionProcedures> Procedures = MakeShared<FTclConnectionProcedures>(
			[this](const FString& Frame)
			{
				if (!bSessionOpen)
					return false;
				Sent.Add(Frame);
				return true;
			});

		/** What the calls came to, in order: "ok:<body>" or "error:<status>:<body>". */
		TArray<FString> Outcomes;

		void Call(const FString& Procedure, const FString& Input)
		{
			Procedures->Call(Procedure, false, Input,
				UTentacleHttpClient::FOnSuccess::CreateLambda([this](const FString& Body) { Outcomes.Add(TEXT("ok:") + Body); }),
				UTentacleHttpClient::FOnError::CreateLambda([this](int32 Status, const FString& Body)
				{
					Outcomes.Add(FString::Printf(TEXT("error:%d:%s"), Status, *Body));
				}));
		}

		/** The data of sent frame Index. */
		TSharedPtr<FJsonObject> SentData(int32 Index) const
		{
			return TclTest::ObjectField(TclTest::ParseObject(Sent.IsValidIndex(Index) ? Sent[Index] : FString()), TEXT("data"));
		}
	};
}

/**
 * In Embedded mode the HTTP client's calls travel as Rpc_Request frames, and
 * each completes from the Rpc_Response carrying its id, with what HTTP would
 * have given: the body below 400, the status and body otherwise. A call with
 * no session open fails at once, and a session that closes fails what was
 * waiting.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclConnectionProceduresTest,
	"Tentacle.Embedded.ProceduresTravelOverTheConnection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclConnectionProceduresTest::RunTest(const FString& Parameters)
{
	FProceduresFixture Fixture;

	Fixture.Call(TEXT("timers.start"), TEXT("{\"id\":\"t1\"}"));
	Fixture.Call(TEXT("viewerProperty.getRegistry"), TEXT(""));
	if (!TestEqual(TEXT("two frames sent"), Fixture.Sent.Num(), 2)) return false;

	const TSharedPtr<FJsonObject> First = TclTest::ParseObject(Fixture.Sent[0]);
	TestEqual(TEXT("an Rpc_Request"), TclTest::StringField(First, TEXT("type")), FString(TEXT("Rpc_Request")));
	const FString FirstId = TclTest::StringField(Fixture.SentData(0), TEXT("id"));
	TestEqual(TEXT("naming its procedure"), TclTest::StringField(Fixture.SentData(0), TEXT("procedure")), FString(TEXT("timers.start")));
	TestEqual(TEXT("with its input as text"), TclTest::StringField(Fixture.SentData(0), TEXT("input")), FString(TEXT("{\"id\":\"t1\"}")));
	TestFalse(TEXT("no input is left out"), Fixture.SentData(1).IsValid() && Fixture.SentData(1)->HasField(TEXT("input")));
	const FString SecondId = TclTest::StringField(Fixture.SentData(1), TEXT("id"));
	TestNotEqual(TEXT("each call has its own id"), FirstId, SecondId);

	// Answered out of order, each completes its own call. A refusal is logged,
	// as the HTTP client logs one.
	AddExpectedError(TEXT("timers.start failed \\(404\\)"), EAutomationExpectedErrorFlags::Contains, 1);
	Fixture.Procedures->HandleResponse(SecondId, 200, TEXT("{\"result\":{\"data\":{}}}"));
	Fixture.Procedures->HandleResponse(FirstId, 404, TEXT("{\"error\":\"nope\"}"));
	TestEqual(TEXT("each answer completes its own call"), FString::Join(Fixture.Outcomes, TEXT(" | ")),
		FString(TEXT("ok:{\"result\":{\"data\":{}}} | error:404:{\"error\":\"nope\"}")));
	TestEqual(TEXT("nothing left waiting"), Fixture.Procedures->NumPending(), 0);

	// An answer to no waiting call changes nothing.
	Fixture.Procedures->HandleResponse(FirstId, 200, TEXT("{}"));
	TestEqual(TEXT("a repeated answer is ignored"), Fixture.Outcomes.Num(), 2);

	// A session that closes fails what was waiting, as a dropped connection does.
	Fixture.Outcomes.Reset();
	Fixture.Call(TEXT("timers.pause"), TEXT("{\"id\":\"t1\"}"));
	Fixture.Procedures->FailAll(TEXT("the session closed"));
	TestEqual(TEXT("a waiting call fails with no status"), FString::Join(Fixture.Outcomes, TEXT(" | ")), FString(TEXT("error:0:the session closed")));

	// With no session open a call fails at once.
	Fixture.Outcomes.Reset();
	Fixture.bSessionOpen = false;
	Fixture.Call(TEXT("timers.fire"), TEXT("{\"id\":\"t1\"}"));
	TestEqual(TEXT("no session, no wait"), FString::Join(Fixture.Outcomes, TEXT(" | ")), FString(TEXT("error:0:Tentacle is not connected")));
	TestEqual(TEXT("and nothing left waiting"), Fixture.Procedures->NumPending(), 0);
	return true;
}

/** The Embedded settings reach the core as its config, an empty setting left out. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclEmbeddedOptionsTest,
	"Tentacle.Embedded.OptionsBecomeTheCoreConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclEmbeddedOptionsTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<FJsonObject> Defaults = TclTest::ParseObject(FTclEmbeddedOptions().ToConfigJson());
	if (!TestTrue(TEXT("valid JSON"), Defaults.IsValid())) return false;
	TestFalse(TEXT("no data directory"), Defaults->HasField(TEXT("dataDirectory")));
	TestFalse(TEXT("no client id"), Defaults->HasField(TEXT("twitchClientId")));

	FTclEmbeddedOptions Options;
	Options.DataDirectory = TEXT("C:/Game/Saved/Tentacle");
	Options.TwitchClientId = TEXT("abc123");
	Options.ConfigFiles = { TEXT("{\"timers\":[]}"), TEXT("{\"customEvents\":[]}") };
	const TSharedPtr<FJsonObject> Config = TclTest::ParseObject(Options.ToConfigJson());
	TestEqual(TEXT("data directory"), TclTest::StringField(Config, TEXT("dataDirectory")), Options.DataDirectory);
	TestEqual(TEXT("client id"), TclTest::StringField(Config, TEXT("twitchClientId")), Options.TwitchClientId);
	const TArray<TSharedPtr<FJsonValue>>* Files = nullptr;
	if (TestTrue(TEXT("config files"), Config.IsValid() && Config->TryGetArrayField(TEXT("configFiles"), Files) && Files))
	{
		TestEqual(TEXT("both files"), Files->Num(), 2);
		TestEqual(TEXT("as their text"), (*Files)[1]->AsString(), Options.ConfigFiles[1]);
	}

	FTclEmbeddedOptions Other = Options;
	TestTrue(TEXT("equal options"), Other == Options);
	Other.TwitchClientId = TEXT("other");
	TestTrue(TEXT("a changed setting"), Other != Options);
	return true;
}

/**
 * The config files a game ships come from Config/Tentacle in the project, where
 * the Tentacle app's Data > Export writes them, unless the actor names
 * another folder: every .json file in it, in name order.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclConfigFilesTest,
	"Tentacle.Embedded.ConfigFilesComeFromTheConfigFolder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclConfigFilesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the actor's default"), GetDefault<ATentacle>()->ConfigDirectory, FString(TEXT("Config/Tentacle")));
	TestEqual(
		TEXT("a relative folder is the project's"),
		FTclConfigFiles::Resolve(TEXT("Config/Tentacle")),
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/Tentacle")));
	TestEqual(TEXT("an empty one is the default"), FTclConfigFiles::Resolve(TEXT("")), FTclConfigFiles::Resolve(TEXT("Config/Tentacle")));
	const FString Absolute = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("Elsewhere")));
	TestEqual(TEXT("a full path is itself"), FTclConfigFiles::Resolve(Absolute), Absolute);

	const FString Folder = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("TentacleTests"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	FFileHelper::SaveStringToFile(TEXT("{\"timers\":1}"), *FPaths::Combine(Folder, TEXT("tentacle-timers.json")));
	FFileHelper::SaveStringToFile(TEXT("{\"bot\":1}"), *FPaths::Combine(Folder, TEXT("tentacle-bot.json")));
	FFileHelper::SaveStringToFile(TEXT("not config"), *FPaths::Combine(Folder, TEXT("notes.txt")));

	TestEqual(TEXT("every .json file, in name order"), FTclConfigFiles::Read(Folder), TArray<FString>{ TEXT("{\"bot\":1}"), TEXT("{\"timers\":1}") });
	TestEqual(TEXT("nothing from a folder that is not there"), FTclConfigFiles::Read(FPaths::Combine(Folder, TEXT("Missing"))).Num(), 0);

	IFileManager::Get().DeleteDirectory(*Folder, false, true);
	return true;
}

/**
 * Where Tentacle runs: Editor Mode while playing in the editor, Build Mode
 * anywhere else, each Paired or Embedded, and both Paired until a game says
 * otherwise. -tentacle-mode=paired|embedded on the command line wins over both
 * (and app, Paired's name before 1.2.0-alpha.3), so a packaged game can be
 * tested against the app. A value the override does not know is ignored with a
 * warning.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclTentacleModeTest,
	"Tentacle.Embedded.ModeResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclTentacleModeTest::RunTest(const FString& Parameters)
{
	using TclTentacleMode::ResolvesToEmbedded;
	const ETclTentacleMode Paired = ETclTentacleMode::Paired;
	const ETclTentacleMode Embedded = ETclTentacleMode::Embedded;
	const TCHAR* None = TEXT("MyGame -log");

	TestEqual(TEXT("Editor Mode starts as Paired"), GetDefault<ATentacle>()->EditorMode, Paired);
	TestEqual(TEXT("Build Mode starts as Paired"), GetDefault<ATentacle>()->BuildMode, Paired);

	TestFalse(TEXT("both Paired, playing in the editor"), ResolvesToEmbedded(Paired, Paired, true, None));
	TestFalse(TEXT("both Paired, a packaged game"), ResolvesToEmbedded(Paired, Paired, false, None));
	TestTrue(TEXT("Editor Mode Embedded, playing in the editor"), ResolvesToEmbedded(Embedded, Paired, true, None));
	TestFalse(TEXT("Editor Mode Embedded, a packaged game"), ResolvesToEmbedded(Embedded, Paired, false, None));
	TestTrue(TEXT("Build Mode Embedded, a packaged game"), ResolvesToEmbedded(Paired, Embedded, false, None));
	TestFalse(TEXT("Build Mode Embedded, playing in the editor"), ResolvesToEmbedded(Paired, Embedded, true, None));
	TestTrue(TEXT("both Embedded, playing in the editor"), ResolvesToEmbedded(Embedded, Embedded, true, None));

	TestFalse(TEXT("the command line's paired wins"), ResolvesToEmbedded(Embedded, Embedded, false, TEXT("MyGame -tentacle-mode=paired")));
	TestFalse(TEXT("the command line's app, Paired's old name, still wins"), ResolvesToEmbedded(Embedded, Embedded, false, TEXT("MyGame -tentacle-mode=app")));
	TestTrue(TEXT("the command line's embedded wins"), ResolvesToEmbedded(Paired, Paired, true, TEXT("MyGame -tentacle-mode=Embedded -log")));

	AddExpectedError(TEXT("-tentacle-mode=sidecar"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("an unknown value is ignored"), ResolvesToEmbedded(Paired, Embedded, false, TEXT("MyGame -tentacle-mode=sidecar")));
	return true;
}

/**
 * A mode changed after the actor began play applies in full on the next
 * Connect: the session and the HTTP procedures move together. Embedded, a
 * procedure goes over the session, so with none open it fails at once; back in
 * Paired mode it goes to the app over HTTP again.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclModeChangeTest,
	"Tentacle.Embedded.ModeChangedBeforeConnectMovesTheProcedures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclModeChangeTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	if (!TestNotNull(TEXT("Tentacle actor"), Tentacle)) return false;
	Tentacle->bAutoConnect = false;
	Tentacle->DispatchBeginPlay();

	// Shared rather than on the stack: the Paired mode call is answered over HTTP
	// after this test has returned.
	const TSharedRef<TArray<FString>> Outcomes = MakeShared<TArray<FString>>();
	auto Call = [Tentacle, Outcomes]()
	{
		Tentacle->HttpClient->GetRegistry(TEXT(""),
			UTentacleHttpClient::FOnSuccess::CreateLambda([Outcomes](const FString&) { Outcomes->Add(TEXT("ok")); }),
			UTentacleHttpClient::FOnError::CreateLambda([Outcomes](int32 Status, const FString& Body)
			{
				Outcomes->Add(FString::Printf(TEXT("error:%d:%s"), Status, *Body));
			}));
	};

	FTclEmbeddedCore::Shutdown();
	const FString DataDirectory = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("TentacleTests"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	Tentacle->EmbeddedDataDirectory = DataDirectory;
	Tentacle->EditorMode = Tentacle->BuildMode = ETclTentacleMode::Embedded;
	TestTrue(TEXT("now embedded"), Tentacle->IsEmbedded());
	Tentacle->Connect();
	Call();
	TestEqual(TEXT("embedded, a procedure waits on the session, which is not open yet"),
		FString::Join(*Outcomes, TEXT(" | ")), FString(TEXT("error:0:Tentacle is not connected")));

	Tentacle->Disconnect();
	Tentacle->EditorMode = Tentacle->BuildMode = ETclTentacleMode::Paired;
	Tentacle->Connect();
	Outcomes->Reset();
	Call();
	TestEqual(TEXT("back in Paired mode, it goes to the app over HTTP, which answers later"), Outcomes->Num(), 0);

	Tentacle->Disconnect();
	FTclEmbeddedCore::Shutdown();
	IFileManager::Get().DeleteDirectory(*DataDirectory, false, true);
	return true;
}

/**
 * A Tentacle actor in Embedded mode runs against the real core: its session
 * opens, the platforms report their connections, the registry loads over the
 * connection, a change the game makes (creating a timer) is answered over it
 * and comes back as definitions, and a Twitch API request node reaches the core
 * and gets its refusal. Skipped when the core is not installed
 * (just unreal::native).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclEmbeddedLiveTest,
	"Tentacle.Embedded.ActorRunsAgainstTheCore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclEmbeddedLiveTest::RunTest(const FString& Parameters)
{
	FString Missing;
	if (FTclCoreLibrary::Get(Missing) == nullptr)
	{
		AddInfo(FString::Printf(TEXT("Skipped: %s"), *Missing));
		return true;
	}

	struct FLive
	{
		TclTest::FWorld World;
		ATentacle* Tentacle = nullptr;
		TStrongObjectPtr<UTclTestRecorder> Recorder;
		FString DataDirectory;

		/** One frame of the actor, which polls the core. */
		void Tick() const
		{
			if (Tentacle != nullptr)
				static_cast<AActor*>(Tentacle)->Tick(0.016f);
		}
	};
	TSharedRef<FLive> Live = MakeShared<FLive>();
	if (!TestNotNull(TEXT("test world"), Live->World.Get())) return false;

	// A fresh core with a folder of its own, so nothing a run makes survives it.
	FTclEmbeddedCore::Shutdown();
	Live->DataDirectory = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("TentacleTests"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
	Live->Recorder.Reset(NewObject<UTclTestRecorder>(GetTransientPackage()));

	Live->Tentacle = Live->World.Get()->SpawnActor<ATentacle>();
	if (!TestNotNull(TEXT("Tentacle actor"), Live->Tentacle)) return false;
	Live->Tentacle->EditorMode = Live->Tentacle->BuildMode = ETclTentacleMode::Embedded;
	Live->Tentacle->EmbeddedDataDirectory = Live->DataDirectory;

	// A config file the game ships, in a folder of its own as the Tentacle app
	// exports one.
	const FString ConfigDirectory = FPaths::Combine(Live->DataDirectory, TEXT("Config"));
	TestTrue(TEXT("a timers export"), FFileHelper::SaveStringToFile(TEXT(
		"{\"app\":\"tentacle\",\"section\":\"timers\",\"formatVersion\":1,\"exportedAt\":\"2026-10-03T00:00:00.000Z\","
		"\"data\":{\"timers\":[{\"name\":\"Shipped Boss\",\"repeating\":true,\"onlyWhileLive\":false,\"intervalMinSeconds\":600,"
		"\"intervalMaxSeconds\":600,\"minChatMessages\":0,\"scheduledMessageMode\":\"rotate\"}]}}"),
		*FPaths::Combine(ConfigDirectory, TEXT("tentacle-timers.json"))));
	Live->Tentacle->ConfigDirectory = FPaths::ConvertRelativePathToFull(ConfigDirectory);
	Live->Tentacle->DispatchBeginPlay();

	auto Until = [this, Live](TFunction<bool()> Condition, const FString& What)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
			[Live, Condition]()
			{
				Live->Tick();
				return Condition();
			},
			[this, What]()
			{
				AddError(FString::Printf(TEXT("timed out waiting for %s"), *What));
				return true;
			},
			15.0f));
	};

	Until([Live]() { return Live->Tentacle->IsConnected(); }, TEXT("the session to open"));
	Until([Live]()
	{
		bool bTwitch = false;
		bool bKick = false;
		Live->Tentacle->Connections->GetConnection(ETclStreamPlatform::Twitch, bTwitch);
		Live->Tentacle->Connections->GetConnection(ETclStreamPlatform::Kick, bKick);
		return bTwitch && bKick;
	}, TEXT("both platforms' connections"));
	Until([Live]() { return Live->Tentacle->ViewerProperties->bIsInitialLoadComplete; }, TEXT("the registry over the connection"));
	Until([Live]()
	{
		bool bFound = false;
		Live->Tentacle->Timers->FindDefinitionByName(TEXT("Shipped Boss"), bFound);
		return bFound;
	}, TEXT("the timer the shipped definition file defines"));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Live]()
	{
		FTclTimerSettings Settings;
		Settings.Name = TEXT("Embedded Boss");
		FTclOnDefinitionComplete OnComplete;
		OnComplete.BindUFunction(Live->Recorder.Get(), GET_FUNCTION_NAME_CHECKED(UTclTestRecorder, OnDefinitionComplete));
		Live->Tentacle->Timers->CreateTimer(Settings, OnComplete);
		return true;
	}));
	Until([Live]() { return Live->Recorder->Count(TEXT("OnDefinitionComplete")) == 1; }, TEXT("the timer change's answer"));
	Until([Live]()
	{
		bool bFound = false;
		Live->Tentacle->Timers->FindDefinitionByName(TEXT("Embedded Boss"), bFound);
		return bFound;
	}, TEXT("the new timer's definition"));

	// A Twitch API request node goes to the core over the session, which
	// refuses it with no account signed in.
	AddExpectedError(TEXT("twitch.getStreams failed"), EAutomationExpectedErrorFlags::Contains, 1);
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([Live]()
	{
		UTclTwitch_GetStreams* Node = UTclTwitch_GetStreams::GetStreams(Live->Tentacle, FTclTwitchApiGetStreamsInput());
		Node->OnSuccess.AddDynamic(Live->Recorder.Get(), &UTclTestRecorder::OnGetStreams);
		Node->OnFailure.AddDynamic(Live->Recorder.Get(), &UTclTestRecorder::OnPlatformRequestFailed);
		Node->Activate();
		return true;
	}));
	Until([Live]() { return Live->Recorder->Count(TEXT("OnPlatformRequestFailed")) + Live->Recorder->Count(TEXT("OnGetStreams")) > 0; },
		TEXT("the Twitch API request's answer"));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Live]()
	{
		const FTclRecordedCall* Answer = Live->Recorder->Find(TEXT("OnDefinitionComplete"));
		TestEqual(TEXT("the core made the timer"), Answer ? Answer->Detail : FString(), FString(TEXT("ETclDefinitionResult::Success")));

		const FTclRecordedCall* Refused = Live->Recorder->Find(TEXT("OnPlatformRequestFailed"));
		TestEqual(TEXT("the core refuses a Twitch request with no account"), Refused ? Refused->Detail : FString(),
			FString(TEXT("400|No Twitch account connected. Please connect the account first.")));

		bool bTwitch = false;
		const UTclPlatformConnection* Twitch = Live->Tentacle->Connections->GetConnection(ETclStreamPlatform::Twitch, bTwitch);
		TestTrue(TEXT("no Twitch account signed in"), Twitch != nullptr && Twitch->Account == nullptr);

		Live->Tentacle->Disconnect();
		FTclEmbeddedCore::Shutdown();
		IFileManager::Get().DeleteDirectory(*Live->DataDirectory, false, true);
		return true;
	}));

	return true;
}

/**
 * A Tentacle actor in Paired mode connects to a running Tentacle over its
 * WebSocket and loads the registry over HTTP: the same session the embedded
 * test opens, on the other socket. Runs against the private test backend
 * (`just cli tentacle up`, in the repository's .tentacle-test-instance), and
 * is skipped when none is running.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclAppLiveTest,
	"Tentacle.Engine.ActorConnectsToARunningTentacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclAppLiveTest::RunTest(const FString& Parameters)
{
	const FString InstanceDirectory = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("../../.tentacle-test-instance")));
	const FTclSidecarEndpoints Endpoints = FTclSidecarEndpoints::Resolve(InstanceDirectory);
	if (!Endpoints.bFromDiscoveryFile)
	{
		AddInfo(TEXT("Skipped: no test backend (just cli tentacle up)"));
		return true;
	}

	struct FLive
	{
		TclTest::FWorld World;
		ATentacle* Tentacle = nullptr;
		bool bHealthAnswered = false;
		bool bBackendUp = false;

		void Tick() const
		{
			if (Tentacle != nullptr)
				static_cast<AActor*>(Tentacle)->Tick(0.016f);
		}
	};
	TSharedRef<FLive> Live = MakeShared<FLive>();
	if (!TestNotNull(TEXT("test world"), Live->World.Get())) return false;

	// A discovery file outlives the backend that wrote it, so ask the backend.
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Health = FHttpModule::Get().CreateRequest();
	Health->SetURL(Endpoints.HttpBaseUrl + TEXT("/api/health"));
	Health->SetVerb(TEXT("GET"));
	Health->OnProcessRequestComplete().BindLambda([Live](FHttpRequestPtr, FHttpResponsePtr Response, bool bConnected)
	{
		Live->bBackendUp = bConnected && Response.IsValid() && Response->GetResponseCode() == 200;
		Live->bHealthAnswered = true;
	});
	Health->ProcessRequest();

	auto Until = [this, Live](TFunction<bool()> Condition, const FString& What)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
			[Live, Condition]()
			{
				if (!Live->bBackendUp)
					return true;
				Live->Tick();
				return Condition();
			},
			[this, What]()
			{
				AddError(FString::Printf(TEXT("timed out waiting for %s"), *What));
				return true;
			},
			15.0f));
	};

	ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
		[this, Live, InstanceDirectory]()
		{
			if (!Live->bHealthAnswered)
				return false;
			if (!Live->bBackendUp)
			{
				AddInfo(TEXT("Skipped: the test backend is not answering (just cli tentacle up)"));
				return true;
			}

			Live->Tentacle = Live->World.Get()->SpawnActor<ATentacle>();
			Live->Tentacle->DataDirectoryOverride = InstanceDirectory;
			Live->Tentacle->SessionLabel = TEXT("Unreal live test");
			Live->Tentacle->DispatchBeginPlay();
			return true;
		},
		[this]()
		{
			AddError(TEXT("the test backend's health check never answered"));
			return true;
		},
		10.0f));

	Until([Live]() { return Live->Tentacle->IsConnected(); }, TEXT("the session to open"));
	Until([Live]() { return Live->Tentacle->ViewerProperties->bIsInitialLoadComplete; }, TEXT("the registry over HTTP"));

	// A Kick API request node reaches the app's kick router over HTTP, which
	// refuses it with no account connected.
	TSharedRef<TStrongObjectPtr<UTclTestRecorder>> Recorder = MakeShared<TStrongObjectPtr<UTclTestRecorder>>(NewObject<UTclTestRecorder>(GetTransientPackage()));
	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Live, Recorder]()
	{
		if (Live->Tentacle == nullptr)
			return true;
		AddExpectedError(TEXT("HTTP request failed"), EAutomationExpectedErrorFlags::Contains, 1);
		FTclKickApiGetChannelsInput Input;
		Input.bHasSlug = true;
		Input.Slug = { TEXT("tentacle") };
		UTclKick_GetChannels* Node = UTclKick_GetChannels::GetChannels(Live->Tentacle, Input);
		Node->OnFailure.AddDynamic(Recorder->Get(), &UTclTestRecorder::OnPlatformRequestFailed);
		Node->Activate();
		return true;
	}));
	Until([Recorder]() { return (*Recorder)->Count(TEXT("OnPlatformRequestFailed")) > 0; }, TEXT("the Kick API request's answer"));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, Live, Recorder]()
	{
		if (Live->Tentacle != nullptr)
		{
			const FTclRecordedCall* Refused = (*Recorder)->Find(TEXT("OnPlatformRequestFailed"));
			TestEqual(TEXT("the app refuses a Kick request with no account"), Refused ? Refused->Detail : FString(),
				FString(TEXT("400|No Kick account connected. Please connect the account first.")));

			TestFalse(TEXT("a session id"), Live->Tentacle->SessionId.IsEmpty());
			Live->Tentacle->Disconnect();
			TestFalse(TEXT("disconnected"), Live->Tentacle->IsConnected());
		}
		return true;
	}));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
