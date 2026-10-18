// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Timers.h"
#include "Base/TclTimerListenerComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Sdk/TclMsg_ServerToClient_TimersDefinitions.h"
#include "Sdk/TclMsg_ServerToClient_TimersFired.h"
#include "Sdk/TclTimerDefinition.h"
#include "Sdk/TclTimerEvent.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** Deliver a timer fire the way one arrives from Tentacle. */
	void FireTimer(ATentacle* Tentacle, const FString& Name, int32 FireCount = 1)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_TimersFired>(
			GetTransientPackage(), TclTest::TimerFiredMessage(Name, FireCount));
		Tentacle->Timers->HandleMsg_Fired(Msg);
	}

	/** Deliver timer definitions the way they arrive from Tentacle. */
	void SendTimerDefinitions(ATentacle* Tentacle, const FString& Json)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_TimersDefinitions>(Tentacle, Json);
		Tentacle->Timers->HandleMsg_Definitions(Msg, Json);
	}

	/** A handler bound to one of the recorder's functions. */
	FTclTimerHandler TimerHandler(UTclTestRecorder* Recorder, FName Function)
	{
		FTclTimerHandler Result;
		Result.BindUFunction(Recorder, Function);
		return Result;
	}
}

/**
 * A fire raises On Timer Fired with the fire, then runs the handlers subscribed
 * to its name. Names match case-insensitively unless the subscription asks for
 * case sensitivity, and Unsubscribe compares the name the way its subscription
 * does.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclTimersSubscribeTest,
	"Tentacle.Timers.SubscribeRunsAHandlerForOneTimer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclTimersSubscribeTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTentacle_Timers* Timers = Tentacle->Timers;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	Timers->OnTimerFired.AddDynamic(Recorder, &UTclTestRecorder::OnTimerFired);

	const FTclTimerHandler Boss = TimerHandler(Recorder, GET_FUNCTION_NAME_CHECKED(UTclTestRecorder, OnTimerSubscribed));
	const FTclTimerHandler Loot = TimerHandler(Recorder, GET_FUNCTION_NAME_CHECKED(UTclTestRecorder, OnTimerSubscribedOther));

	Timers->Subscribe(TEXT("Boss"), Boss);
	Timers->Subscribe(TEXT("Loot Drop"), Loot, true);

	// Case-insensitive by default, after On Timer Fired, with the fire.
	FireTimer(Tentacle, TEXT("boss"), 3);
	TestEqual(TEXT("default subscription: order"), FString::Join(Recorder->Names(), TEXT(", ")), TEXT("OnTimerFired, OnTimerSubscribed"));
	const FTclRecordedCall* Fired = Recorder->Find(TEXT("OnTimerFired"));
	TestEqual(TEXT("On Timer Fired carries the name and the count"), Fired ? Fired->Detail : FString(), FString(TEXT("boss#3")));
	const UTclTimerEvent* Event = Fired ? Cast<UTclTimerEvent>(Fired->Event.Get()) : nullptr;
	TestTrue(TEXT("the fire says when the next one is"), Event && Event->NextFireInSeconds == 600);

	// A case-sensitive subscription wants its exact name.
	Recorder->Calls.Reset();
	FireTimer(Tentacle, TEXT("loot drop"));
	TestEqual(TEXT("case-sensitive subscription: other casing is ignored"), Recorder->Count(TEXT("OnTimerSubscribedOther")), 0);
	FireTimer(Tentacle, TEXT("Loot Drop"));
	TestEqual(TEXT("case-sensitive subscription: its exact name runs it"), Recorder->Count(TEXT("OnTimerSubscribedOther")), 1);
	TestEqual(TEXT("On Timer Fired still hears every fire"), Recorder->Count(TEXT("OnTimerFired")), 2);

	// Subscribing the same handler to the same name again runs it once.
	Recorder->Calls.Reset();
	Timers->Subscribe(TEXT("BOSS"), Boss);
	FireTimer(Tentacle, TEXT("Boss"));
	TestEqual(TEXT("a repeated subscription runs once"), Recorder->Count(TEXT("OnTimerSubscribed")), 1);

	// Unsubscribe compares the name the way the subscription does.
	Recorder->Calls.Reset();
	Timers->Unsubscribe(TEXT("loot drop"), Loot);
	FireTimer(Tentacle, TEXT("Loot Drop"));
	TestEqual(TEXT("a case-sensitive subscription survives another casing's Unsubscribe"), Recorder->Count(TEXT("OnTimerSubscribedOther")), 1);

	Recorder->Calls.Reset();
	Timers->Unsubscribe(TEXT("Loot Drop"), Loot);
	Timers->Unsubscribe(TEXT("bOSS"), Boss);
	FireTimer(Tentacle, TEXT("Loot Drop"));
	FireTimer(Tentacle, TEXT("Boss"));
	TestEqual(TEXT("unsubscribed handlers no longer run"), FString::Join(Recorder->Names(), TEXT(", ")), TEXT("OnTimerFired, OnTimerFired"));

	return true;
}

/**
 * The definitions Tentacle sends are held whole, with each timer's state and
 * timing, and On Definitions Changed says so - but not for definitions sent
 * again unchanged, as they are every time the session opens.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclTimersDefinitionsTest,
	"Tentacle.Timers.HoldsTheDefinitionsTentacleSends",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclTimersDefinitionsTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	UTentacle_Timers* Timers = Tentacle->Timers;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	Timers->OnDefinitionsChanged.AddDynamic(Recorder, &UTclTestRecorder::OnDefinitionsChanged);

	TestFalse(TEXT("nothing is loaded before definitions arrive"), Timers->bAreDefinitionsLoaded);

	const FString Message = TclTest::TimerDefinitionsMessage({
		TclTest::TimerDefinitionJson(TEXT("Boss")),
		TclTest::TimerDefinitionJson(TEXT("Loot"), TEXT("paused")),
	});
	SendTimerDefinitions(Tentacle, Message);
	SendTimerDefinitions(Tentacle, Message);

	TestEqual(TEXT("On Definitions Changed fired once for the same set sent twice"), Recorder->Count(TEXT("OnDefinitionsChanged")), 1);
	TestTrue(TEXT("the definitions are loaded"), Timers->bAreDefinitionsLoaded);
	TestEqual(TEXT("both timers"), Timers->Definitions.Num(), 2);

	bool bFound = false;
	const UTclTimerDefinition* Loot = Timers->FindDefinitionByName(TEXT("loot"), bFound);
	if (!TestTrue(TEXT("found ignoring case"), bFound && Loot != nullptr)) return false;
	TestEqual(TEXT("its state"), Loot->State, ETclTimerState::Paused);
	TestTrue(TEXT("its timing"), Loot->bRepeating && Loot->bOnlyWhileLive
		&& Loot->IntervalMinSeconds == 60 && Loot->IntervalMaxSeconds == 120 && Loot->MinChatMessages == 3);

	Timers->FindDefinitionByName(TEXT("loot"), bFound, true);
	TestFalse(TEXT("not found by another casing when case sensitive"), bFound);

	SendTimerDefinitions(Tentacle, TclTest::TimerDefinitionsMessage({ TclTest::TimerDefinitionJson(TEXT("Boss"), TEXT("stopped")) }));
	TestEqual(TEXT("a changed set fires again"), Recorder->Count(TEXT("OnDefinitionsChanged")), 2);
	TestEqual(TEXT("the new set replaces the old"), Timers->Definitions.Num(), 1);

	return true;
}

/**
 * The listener component hears the timers named in Timer Names,
 * case-insensitively unless Case Sensitive is set.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclTimerListenerTest,
	"Tentacle.Timers.ListenerHearsItsTimersCaseInsensitivelyUnlessAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclTimerListenerTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	AActor* Host = World.Get()->SpawnActor<AActor>();
	if (!TestTrue(TEXT("actors spawned"), Tentacle && Host)) return false;

	UTclTimerListenerComponent* Default = NewObject<UTclTimerListenerComponent>(Host);
	Default->TimerNames = { TEXT("Boss") };
	Default->RegisterComponent();

	UTclTimerListenerComponent* Exact = NewObject<UTclTimerListenerComponent>(Host);
	Exact->TimerNames = { TEXT("Boss") };
	Exact->bCaseSensitive = true;
	Exact->RegisterComponent();

	TestFalse(TEXT("case-insensitive by default"), Default->bCaseSensitive);

	UTclTestRecorder* DefaultHeard = NewObject<UTclTestRecorder>(GetTransientPackage());
	UTclTestRecorder* ExactHeard = NewObject<UTclTestRecorder>(GetTransientPackage());
	Default->OnFired.AddDynamic(DefaultHeard, &UTclTestRecorder::OnTimerListenerFired);
	Exact->OnFired.AddDynamic(ExactHeard, &UTclTestRecorder::OnTimerListenerFired);

	// The listeners subscribe when they begin play.
	Host->DispatchBeginPlay();

	FireTimer(Tentacle, TEXT("boss"));
	TestEqual(TEXT("default listener hears another casing"), DefaultHeard->Count(TEXT("OnTimerListenerFired")), 1);
	TestEqual(TEXT("case-sensitive listener does not"), ExactHeard->Count(TEXT("OnTimerListenerFired")), 0);

	FireTimer(Tentacle, TEXT("Boss"));
	TestEqual(TEXT("default listener hears the exact name"), DefaultHeard->Count(TEXT("OnTimerListenerFired")), 2);
	TestEqual(TEXT("case-sensitive listener hears the exact name"), ExactHeard->Count(TEXT("OnTimerListenerFired")), 1);

	FireTimer(Tentacle, TEXT("Loot"));
	TestEqual(TEXT("neither hears another timer (default)"), DefaultHeard->Count(TEXT("OnTimerListenerFired")), 2);
	TestEqual(TEXT("neither hears another timer (case-sensitive)"), ExactHeard->Count(TEXT("OnTimerListenerFired")), 1);

	Host->Destroy();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
