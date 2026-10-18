// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Stream.h"
#include "Base/Tentacle_Viewers.h"
#include "Base/TclViewer.h"
#include "Sdk/TclMsg_ServerToClient_StreamChatJoin.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** Deliver a chat join for a Twitch viewer, the way a join from Tentacle arrives. */
	void JoinChat(ATentacle* Tentacle, const FString& ViewerId, const FString& Name)
	{
		auto* Join = TclTest::ParseMessage<UTclMsg_ServerToClient_StreamChatJoin>(
			GetTransientPackage(), TclTest::ChatJoinMessage(TclTest::TwitchViewerJson(ViewerId, Name)));
		Tentacle->Stream->HandleMsg_ChatJoin(Join);
	}
}

/**
 * The Viewers component is the viewer cache: one object per viewer id for the
 * session, On Viewer Added once per viewer and never on a refresh, and removal
 * and clearing that a later event recovers from.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewersComponentTest,
	"Tentacle.Viewers.ComponentCachesOneViewerPerId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewersComponentTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTentacle_Viewers* Viewers = Tentacle->Viewers;
	if (!TestNotNull(TEXT("the actor has a Viewers component"), Viewers)) return false;

	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	Viewers->OnViewerAdded.AddDynamic(Recorder, &UTclTestRecorder::OnViewerAdded_Viewers);

	TestEqual(TEXT("starts empty"), Viewers->GetViewerCount(), 0);
	TestEqual(TEXT("no viewers to list"), Viewers->GetAllViewers().Num(), 0);

	JoinChat(Tentacle, TEXT("viewer-1"), TEXT("Alice"));
	JoinChat(Tentacle, TEXT("viewer-2"), TEXT("Bob"));
	JoinChat(Tentacle, TEXT("viewer-1"), TEXT("AliceRenamed"));

	TestEqual(TEXT("two viewers cached"), Viewers->GetViewerCount(), 2);
	TestEqual(TEXT("On Viewer Added fires once per viewer, not on the refresh"), Recorder->Count(TEXT("OnViewerAdded_Viewers")), 2);

	bool bFound = false;
	UTclViewer* Alice = Viewers->FindViewerById(TEXT("viewer-1"), bFound);
	if (!TestTrue(TEXT("viewer-1 found"), bFound && Alice != nullptr)) return false;
	TestEqual(TEXT("the refresh updated the cached viewer in place"), Alice->GetDisplayName(), FString(TEXT("AliceRenamed")));
	const FTclRecordedCall* Added = Recorder->Find(TEXT("OnViewerAdded_Viewers"));
	TestTrue(TEXT("On Viewer Added carried the cached viewer"), Added && Added->Viewer.Get() == Alice);

	const TArray<UTclViewer*> All = Viewers->GetAllViewers();
	TestEqual(TEXT("every viewer listed"), All.Num(), 2);
	TestTrue(TEXT("the listed viewer is the cached one"), All.Contains(Alice));

	Viewers->FindViewerById(TEXT("nobody"), bFound);
	TestFalse(TEXT("an unknown id is not found"), bFound);

	Viewers->DestroyViewer(TEXT("viewer-1"));
	Viewers->FindViewerById(TEXT("viewer-1"), bFound);
	TestFalse(TEXT("a destroyed viewer is gone"), bFound);
	TestEqual(TEXT("one viewer left"), Viewers->GetViewerCount(), 1);

	// A later event from a forgotten viewer builds a fresh one and announces it.
	JoinChat(Tentacle, TEXT("viewer-1"), TEXT("Alice"));
	UTclViewer* AliceAgain = Viewers->FindViewerById(TEXT("viewer-1"), bFound);
	TestTrue(TEXT("the forgotten viewer is back"), bFound);
	TestTrue(TEXT("as a new object"), AliceAgain != Alice);
	TestEqual(TEXT("and announced again"), Recorder->Count(TEXT("OnViewerAdded_Viewers")), 3);

	Viewers->DestroyAllViewers();
	TestEqual(TEXT("destroying all empties the cache"), Viewers->GetViewerCount(), 0);

	return true;
}

/**
 * The actor's own viewer members keep working exactly as before, and they are
 * the component's: what one sees, the other sees. The actor's On Viewer Added
 * still fires, immediately before the component's.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewersActorForwardingTest,
	"Tentacle.Viewers.ActorMembersForwardToTheComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewersActorForwardingTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTentacle_Viewers* Viewers = Tentacle->Viewers;
	if (!TestNotNull(TEXT("the actor has a Viewers component"), Viewers)) return false;

	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	Tentacle->OnViewerAdded.AddDynamic(Recorder, &UTclTestRecorder::OnViewerAdded_Actor);
	Viewers->OnViewerAdded.AddDynamic(Recorder, &UTclTestRecorder::OnViewerAdded_Viewers);

	JoinChat(Tentacle, TEXT("viewer-1"), TEXT("Alice"));
	TestEqual(TEXT("the actor's On Viewer Added fires, then the component's"), FString::Join(Recorder->Names(), TEXT(", ")),
		TEXT("OnViewerAdded_Actor, OnViewerAdded_Viewers"));

	bool bActorFound = false;
	bool bComponentFound = false;
	UTclViewer* FromActor = Tentacle->FindViewerById(TEXT("viewer-1"), bActorFound);
	UTclViewer* FromComponent = Viewers->FindViewerById(TEXT("viewer-1"), bComponentFound);
	TestTrue(TEXT("the actor finds the viewer"), bActorFound);
	TestTrue(TEXT("the component finds the same viewer"), bComponentFound && FromActor == FromComponent);
	TestTrue(TEXT("the viewer knows its Tentacle actor"), FromActor && FromActor->Tentacle == Tentacle);

	// Upserting through the actor lands in the component's cache.
	UTclViewerData* Data = NewObject<UTclViewerData>(Outer);
	Data->Id = TEXT("viewer-2");
	UTclViewer* Bob = Tentacle->UpsertViewer(Data);
	TestTrue(TEXT("an actor upsert is in the component"), Bob && Viewers->FindViewerById(TEXT("viewer-2"), bComponentFound) == Bob);

	Tentacle->DestroyViewer(TEXT("viewer-1"));
	Viewers->FindViewerById(TEXT("viewer-1"), bComponentFound);
	TestFalse(TEXT("destroying through the actor removes it from the component"), bComponentFound);

	Tentacle->DestroyAllViewers();
	TestEqual(TEXT("destroying all through the actor empties the component"), Viewers->GetViewerCount(), 0);

	return true;
}

/**
 * Is Connected is Connection Status being Connected, and nothing else.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclIsConnectedTest,
	"Tentacle.Connection.IsConnectedFollowsConnectionStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclIsConnectedTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());

	TestFalse(TEXT("not connected before a session opens"), Tentacle->IsConnected());

	Tentacle->ConnectionStatus = ETclConnectionStatus::Connected;
	TestTrue(TEXT("connected while the status is Connected"), Tentacle->IsConnected());

	Tentacle->ConnectionStatus = ETclConnectionStatus::Disconnected;
	TestFalse(TEXT("not connected once the status is Disconnected"), Tentacle->IsConnected());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
