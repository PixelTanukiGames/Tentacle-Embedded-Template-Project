// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_CustomEvents.h"
#include "Base/Tentacle_Viewers.h"
#include "Base/TclCustomEventListenerComponent.h"
#include "Base/TclViewer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Sdk/TclMsg_ServerToClient_CustomEventsEvent.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** Deliver a custom event the way one arrives from Tentacle. */
	void FireCustomEvent(ATentacle* Tentacle, const FString& Name, const FString& ViewerJson = FString())
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_CustomEventsEvent>(
			GetTransientPackage(), TclTest::CustomEventMessage(Name, ViewerJson));
		Tentacle->CustomEvents->HandleMsg_Event(Msg);
	}

	/** A handler bound to one of the recorder's functions. */
	FTclCustomEventHandler CustomEventHandler(UTclTestRecorder* Recorder, FName Function)
	{
		FTclCustomEventHandler Result;
		Result.BindUFunction(Recorder, Function);
		return Result;
	}
}

/**
 * Subscribe runs a handler for one name, after On Custom Event, with the cached
 * viewer. Names match case-insensitively unless the subscription asks for case
 * sensitivity, and Unsubscribe compares the name the way its subscription does.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomEventsSubscribeTest,
	"Tentacle.CustomEvents.SubscribeRunsAHandlerForOneName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomEventsSubscribeTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTentacle_CustomEvents* CustomEvents = Tentacle->CustomEvents;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	CustomEvents->OnCustomEvent.AddDynamic(Recorder, &UTclTestRecorder::OnCustomEvent);

	const FTclCustomEventHandler Party = CustomEventHandler(Recorder, GET_FUNCTION_NAME_CHECKED(UTclTestRecorder, OnSubscribed));
	const FTclCustomEventHandler Quake = CustomEventHandler(Recorder, GET_FUNCTION_NAME_CHECKED(UTclTestRecorder, OnSubscribedOther));

	CustomEvents->Subscribe(TEXT("Demo Party"), Party);
	CustomEvents->Subscribe(TEXT("Demo Quake"), Quake, true);

	// Case-insensitive by default, after On Custom Event, with the cached viewer.
	FireCustomEvent(Tentacle, TEXT("demo party"), TclTest::TwitchViewerJson(TEXT("viewer-1"), TEXT("Alice")));
	TestEqual(TEXT("default subscription: order"), FString::Join(Recorder->Names(), TEXT(", ")), TEXT("OnCustomEvent, OnSubscribed"));

	bool bFound = false;
	const UTclViewer* Alice = Tentacle->Viewers->FindViewerById(TEXT("viewer-1"), bFound);
	const FTclRecordedCall* PartyCall = Recorder->Find(TEXT("OnSubscribed"));
	TestTrue(TEXT("the handler gets the cached viewer"), PartyCall && bFound && PartyCall->Viewer.Get() == Alice);
	TestEqual(TEXT("the handler gets the event"), PartyCall ? PartyCall->Detail : FString(), FString(TEXT("demo party")));

	// A case-sensitive subscription wants its exact name.
	Recorder->Calls.Reset();
	FireCustomEvent(Tentacle, TEXT("demo quake"));
	TestEqual(TEXT("case-sensitive subscription: other casing is ignored"), Recorder->Count(TEXT("OnSubscribedOther")), 0);
	FireCustomEvent(Tentacle, TEXT("Demo Quake"));
	TestEqual(TEXT("case-sensitive subscription: its exact name runs it"), Recorder->Count(TEXT("OnSubscribedOther")), 1);
	TestEqual(TEXT("On Custom Event still hears every event"), Recorder->Count(TEXT("OnCustomEvent")), 2);

	// Subscribing the same handler to the same name again runs it once.
	Recorder->Calls.Reset();
	CustomEvents->Subscribe(TEXT("DEMO PARTY"), Party);
	FireCustomEvent(Tentacle, TEXT("Demo Party"));
	TestEqual(TEXT("a repeated subscription runs once"), Recorder->Count(TEXT("OnSubscribed")), 1);

	// Unsubscribe compares the name the way the subscription does.
	Recorder->Calls.Reset();
	CustomEvents->Unsubscribe(TEXT("demo quake"), Quake);
	FireCustomEvent(Tentacle, TEXT("Demo Quake"));
	TestEqual(TEXT("a case-sensitive subscription survives another casing's Unsubscribe"), Recorder->Count(TEXT("OnSubscribedOther")), 1);

	Recorder->Calls.Reset();
	CustomEvents->Unsubscribe(TEXT("Demo Quake"), Quake);
	CustomEvents->Unsubscribe(TEXT("DEMO party"), Party);
	FireCustomEvent(Tentacle, TEXT("Demo Quake"));
	FireCustomEvent(Tentacle, TEXT("Demo Party"));
	TestEqual(TEXT("unsubscribed handlers no longer run"), FString::Join(Recorder->Names(), TEXT(", ")), TEXT("OnCustomEvent, OnCustomEvent"));

	return true;
}

/**
 * The listener component matches its Event Names case-insensitively, as it
 * always has, unless Case Sensitive is set.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomEventListenerCaseTest,
	"Tentacle.CustomEvents.ListenerMatchesNamesCaseInsensitivelyUnlessAsked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomEventListenerCaseTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	AActor* Host = World.Get()->SpawnActor<AActor>();
	if (!TestTrue(TEXT("actors spawned"), Tentacle && Host)) return false;

	UTclCustomEventListenerComponent* Default = NewObject<UTclCustomEventListenerComponent>(Host);
	Default->EventNames = { TEXT("Demo Party") };
	Default->RegisterComponent();

	UTclCustomEventListenerComponent* Exact = NewObject<UTclCustomEventListenerComponent>(Host);
	Exact->EventNames = { TEXT("Demo Party") };
	Exact->bCaseSensitive = true;
	Exact->RegisterComponent();

	TestFalse(TEXT("case-insensitive by default"), Default->bCaseSensitive);

	UTclTestRecorder* DefaultHeard = NewObject<UTclTestRecorder>(GetTransientPackage());
	UTclTestRecorder* ExactHeard = NewObject<UTclTestRecorder>(GetTransientPackage());
	Default->OnTriggered.AddDynamic(DefaultHeard, &UTclTestRecorder::OnTriggered);
	Exact->OnTriggered.AddDynamic(ExactHeard, &UTclTestRecorder::OnTriggered);

	// The listeners subscribe when they begin play.
	Host->DispatchBeginPlay();

	FireCustomEvent(Tentacle, TEXT("demo party"));
	TestEqual(TEXT("default listener hears another casing"), DefaultHeard->Count(TEXT("OnTriggered")), 1);
	TestEqual(TEXT("case-sensitive listener does not"), ExactHeard->Count(TEXT("OnTriggered")), 0);

	FireCustomEvent(Tentacle, TEXT("Demo Party"));
	TestEqual(TEXT("default listener hears the exact name"), DefaultHeard->Count(TEXT("OnTriggered")), 2);
	TestEqual(TEXT("case-sensitive listener hears the exact name"), ExactHeard->Count(TEXT("OnTriggered")), 1);

	FireCustomEvent(Tentacle, TEXT("Demo Quake"));
	TestEqual(TEXT("neither hears another name (default)"), DefaultHeard->Count(TEXT("OnTriggered")), 2);
	TestEqual(TEXT("neither hears another name (case-sensitive)"), ExactHeard->Count(TEXT("OnTriggered")), 1);

	Host->Destroy();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
