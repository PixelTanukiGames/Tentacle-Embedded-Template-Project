// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle.h"
#include "Embedded/TclCoreLibrary.h"
#include "Embedded/TclEmbeddedCore.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <TlHelp32.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
	/** The process's threads and handles, where the platform can count them. */
	struct FUsage
	{
		int32 Threads = 0;
		int32 Handles = 0;
	};

	FUsage Usage()
	{
		FUsage Result;
#if PLATFORM_WINDOWS
		DWORD Handles = 0;
		::GetProcessHandleCount(::GetCurrentProcess(), &Handles);
		Result.Handles = static_cast<int32>(Handles);
		const DWORD Me = ::GetCurrentProcessId();
		HANDLE Snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		THREADENTRY32 Entry;
		Entry.dwSize = sizeof(Entry);
		for (BOOL bMore = ::Thread32First(Snapshot, &Entry); bMore; bMore = ::Thread32Next(Snapshot, &Entry))
		{
			if (Entry.th32OwnerProcessID == Me)
				++Result.Threads;
		}
		::CloseHandle(Snapshot);
#endif
		return Result;
	}

	/** An embedded Tentacle actor in a world of its own, with a data folder of its own. */
	struct FEmbeddedRun
	{
		TclTest::FWorld World;
		ATclTestTentacle* Tentacle = nullptr;
		FString DataDirectory;

		FEmbeddedRun()
		{
			DataDirectory = FPaths::Combine(FPaths::ProjectIntermediateDir(), TEXT("TentacleTests"), FGuid::NewGuid().ToString(EGuidFormats::Digits));
			Tentacle = World.Get()->SpawnActor<ATclTestTentacle>();
			Tentacle->EditorMode = Tentacle->BuildMode = ETclTentacleMode::Embedded;
			Tentacle->EmbeddedDataDirectory = DataDirectory;
			Tentacle->DispatchBeginPlay();
		}

		~FEmbeddedRun()
		{
			IFileManager::Get().DeleteDirectory(*DataDirectory, false, true);
		}

		void Tick() const
		{
			if (Tentacle != nullptr)
				static_cast<AActor*>(Tentacle)->Tick(0.016f);
		}
	};
}

/**
 * Connecting an actor that has ended play opens nothing: a callback that runs
 * as it ends (a failed procedure's completion, say) must not leave a session
 * behind on an actor nothing ticks or closes any more.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclConnectAfterEndPlayTest,
	"Tentacle.Shutdown.ConnectAfterEndPlayOpensNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclConnectAfterEndPlayTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;
	ATclTestTentacle* Tentacle = World.Get()->SpawnActor<ATclTestTentacle>();
	if (!TestNotNull(TEXT("Tentacle actor"), Tentacle)) return false;
	Tentacle->bAutoConnect = false;
	Tentacle->DispatchBeginPlay();
	Tentacle->EndPlayForTest(EEndPlayReason::RemovedFromWorld);

	AddExpectedError(TEXT("has ended play"), EAutomationExpectedErrorFlags::Contains, 1);
	Tentacle->Connect();

	TestFalse(TEXT("no connection after EndPlay"), Tentacle->GetConnection().IsValid());
	return true;
}

/**
 * An actor torn down without EndPlay (its world destroyed, then garbage
 * collected) calls nothing back into the game while it goes: its connection
 * used to report the closed session into a half-destroyed actor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclDestroyedWithoutEndPlayTest,
	"Tentacle.Shutdown.DestroyedWithoutEndPlayCallsNothingBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclDestroyedWithoutEndPlayTest::RunTest(const FString& Parameters)
{
	FString Missing;
	if (FTclCoreLibrary::Get(Missing) == nullptr)
	{
		AddInfo(FString::Printf(TEXT("Skipped: %s"), *Missing));
		return true;
	}
	FTclEmbeddedCore::Shutdown();

	struct FState
	{
		TUniquePtr<FEmbeddedRun> Run = MakeUnique<FEmbeddedRun>();
		TStrongObjectPtr<UTclTestRecorder> Recorder;
	};
	TSharedRef<FState> State = MakeShared<FState>();
	State->Recorder.Reset(NewObject<UTclTestRecorder>(GetTransientPackage()));
	State->Run->Tentacle->OnConnectionStatusChanged.AddDynamic(State->Recorder.Get(), &UTclTestRecorder::OnConnectionStatusChanged);

	ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
		[State]()
		{
			State->Run->Tick();
			return State->Run->Tentacle->IsConnected();
		},
		[this]()
		{
			AddError(TEXT("timed out waiting for the session to open"));
			return true;
		},
		15.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, State]()
	{
		const int32 Before = State->Recorder->Count(TEXT("OnConnectionStatusChanged"));
		// The world goes without routing EndPlay, and the actor with the next GC.
		State->Run.Reset();
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		TestEqual(TEXT("no status reported while the actor was destroyed"),
			State->Recorder->Count(TEXT("OnConnectionStatusChanged")), Before);
		FTclEmbeddedCore::Shutdown();
		return true;
	}));
	return true;
}

/**
 * Playing and stopping an embedded game again and again, as entering and
 * leaving Play In Editor does, leaves no threads or handles behind: the core's
 * thread, its sockets, and its files all go with each stop.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRepeatedPlayTest,
	"Tentacle.Shutdown.RepeatedPlayLeavesNothingBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRepeatedPlayTest::RunTest(const FString& Parameters)
{
#if !PLATFORM_WINDOWS
	AddInfo(TEXT("Skipped: counts threads and handles on Windows only"));
	return true;
#else
	FString Missing;
	if (FTclCoreLibrary::Get(Missing) == nullptr)
	{
		AddInfo(FString::Printf(TEXT("Skipped: %s"), *Missing));
		return true;
	}
	FTclEmbeddedCore::Shutdown();

	constexpr int32 WarmUpRuns = 3;
	constexpr int32 MeasuredRuns = 10;
	struct FState
	{
		TUniquePtr<FEmbeddedRun> Run;
		int32 Completed = 0;
		FUsage Before;
	};
	TSharedRef<FState> State = MakeShared<FState>();

	ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
		[State]()
		{
			if (!State->Run.IsValid())
				State->Run = MakeUnique<FEmbeddedRun>();
			State->Run->Tick();
			if (!State->Run->Tentacle->IsConnected())
				return false;

			// One play session: EndPlay, the world gone, the core stopped as EndPIE stops it.
			State->Run->Tentacle->EndPlayForTest(EEndPlayReason::EndPlayInEditor);
			State->Run.Reset();
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			FTclEmbeddedCore::Shutdown();

			if (++State->Completed == WarmUpRuns)
				State->Before = Usage();
			return State->Completed == WarmUpRuns + MeasuredRuns;
		},
		[this]()
		{
			AddError(TEXT("timed out running the play sessions"));
			return true;
		},
		120.0f));

	ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this, State]()
	{
		const FUsage After = Usage();
		// A thread or a handle left by every session would add ten; the
		// editor's own background work moves the counts by a few.
		TestTrue(FString::Printf(TEXT("threads %d before, %d after"), State->Before.Threads, After.Threads),
			After.Threads - State->Before.Threads < 4);
		TestTrue(FString::Printf(TEXT("handles %d before, %d after"), State->Before.Handles, After.Handles),
			After.Handles - State->Before.Handles < 10);
		return true;
	}));
	return true;
#endif
}

#endif
