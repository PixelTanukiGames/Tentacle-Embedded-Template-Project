// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Base/TclViewer.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/TclViewerData.h"
#include "Sdk/TclViewerData_Properties.h"
#include "Sdk/TclViewerPropertyValue.h"
#include "Sdk/TclViewerPropertyCollection.h"
#include "Sdk/TclViewerPropertyDefinition.h"
#include "Sdk/TclViewerPropertyType.h"

namespace
{
	/**
	 * A Tentacle actor with one collection (Outsiders = C1) holding one int
	 * property (kills = D1, default 0), and a real viewer (not a mock, so its
	 * writes go to Tentacle) whose kills Tentacle last reported as 7.
	 */
	struct FWriteFixture
	{
		ATentacle* Tentacle = nullptr;
		UTclViewer* Viewer = nullptr;

		/** Keeps the actor alive while a test waits across frames; the viewer, its data and the registry all hang off it. */
		TStrongObjectPtr<ATentacle> KeepAlive;

		FWriteFixture()
		{
			UObject* Outer = GetTransientPackage();
			Tentacle = NewObject<ATentacle>(Outer);
			KeepAlive.Reset(Tentacle);

			UTclViewerPropertyCollection* Collection = NewObject<UTclViewerPropertyCollection>(Outer);
			Collection->Id = TEXT("C1");
			Collection->Name = TEXT("Outsiders");

			UTclViewerPropertyDefinition* Kills = NewObject<UTclViewerPropertyDefinition>(Outer);
			Kills->Id = TEXT("D1");
			Kills->CollectionId = TEXT("C1");
			Kills->Name = TEXT("kills");
			Kills->Type = ETclViewerPropertyType::Int;
			Kills->Value = TEXT("0");

			Tentacle->ViewerProperties->ApplyRegistry({ Collection }, { Kills });

			Viewer = Tentacle->UpsertViewer(MakeSnapshot(TEXT("7")));
		}

		/** A snapshot of the viewer as Tentacle would send it, with kills at Kills. */
		static UTclViewerData* MakeSnapshot(const FString& Kills)
		{
			UObject* Outer = GetTransientPackage();
			UTclViewerData* Data = NewObject<UTclViewerData>(Outer);
			Data->Id = TEXT("viewer-1");
			Data->bIsMock = false;

			UTclViewerPropertyValue* Slim = NewObject<UTclViewerPropertyValue>(Outer);
			Slim->Value = Kills;
			Slim->bIsDefault = false;

			FTclViewerData_Properties Values;
			Values.Values.Add(TEXT("D1"), Slim);
			Data->Properties.Add(TEXT("C1"), Values);
			return Data;
		}

		/** The viewer's kills as a game reads them. */
		int64 Kills() const
		{
			ETclFindPropertyResult Result = ETclFindPropertyResult::PropertyNotFound;
			return Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), Result);
		}
	};
}

/**
 * A write Tentacle never takes gives up its hold on the entry, so the next
 * snapshot from Tentacle applies instead of the refused value standing for the
 * rest of the session. The write goes to a loopback port nothing listens on,
 * which is what a closed Tentacle looks like from the game.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyFailedWriteTest,
	"Tentacle.ViewerProperties.FailedWriteReleasesItsHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyFailedWriteTest::RunTest(const FString& Parameters)
{
	TSharedRef<FWriteFixture> Fixture = MakeShared<FWriteFixture>();
	if (!TestNotNull(TEXT("viewer"), Fixture->Viewer)) return false;
	TestEqual(TEXT("Tentacle's value to start"), Fixture->Kills(), static_cast<int64>(7));

	// Port 1 on loopback: nothing listens there, so the write fails to connect.
	UTentacleHttpClient* Http = NewObject<UTentacleHttpClient>(Fixture->Tentacle);
	Http->Configure(TEXT("http://127.0.0.1:1"), TEXT(""));
	Fixture->Tentacle->HttpClient = Http;

	ETclSetPropertyResult SetResult = ETclSetPropertyResult::PropertyNotFound;
	Fixture->Viewer->SetProperty_Int(TEXT("Outsiders"), TEXT("kills"), 100, SetResult);
	TestEqual(TEXT("the write is accepted locally"), SetResult, ETclSetPropertyResult::Success);
	TestEqual(TEXT("and the game reads it at once"), Fixture->Kills(), static_cast<int64>(100));

	// While the write is in flight its hold protects it from an older snapshot.
	Fixture->Viewer->MergePropertiesFromData(FWriteFixture::MakeSnapshot(TEXT("7")));
	TestEqual(TEXT("an in-flight write survives a stale snapshot"), Fixture->Kills(), static_cast<int64>(100));

	// Once the write has failed, the next snapshot applies.
	ADD_LATENT_AUTOMATION_COMMAND(FUntilCommand(
		[Fixture]()
		{
			Fixture->Viewer->MergePropertiesFromData(FWriteFixture::MakeSnapshot(TEXT("7")));
			return Fixture->Kills() == 7;
		},
		[this, Fixture]()
		{
			AddError(FString::Printf(
				TEXT("the refused write still held the entry at %lld after the request failed; the snapshot's 7 never applied"),
				Fixture->Kills()));
			return true;
		},
		15.0f));

	return true;
}

/**
 * Only the write that failed gives up its hold: an entry that has been written
 * again since keeps protecting the newer value.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyReleaseKeepsNewerWriteTest,
	"Tentacle.ViewerProperties.ReleasingAFailedWriteKeepsANewerOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyReleaseKeepsNewerWriteTest::RunTest(const FString& Parameters)
{
	FWriteFixture Fixture;
	if (!TestNotNull(TEXT("viewer"), Fixture.Viewer)) return false;

	// Writes stay local here: a mock viewer's writes are never sent.
	Fixture.Viewer->Data->bIsMock = true;

	ETclSetPropertyResult SetResult = ETclSetPropertyResult::PropertyNotFound;
	Fixture.Viewer->SetProperty_Int(TEXT("Outsiders"), TEXT("kills"), 100, SetResult);
	const FTclViewerPropertyEntry* Entry = Fixture.Viewer->Properties.Find(TEXT("C1/D1/viewer-1"));
	if (!TestNotNull(TEXT("the written entry"), Entry)) return false;
	const FTclViewerPropertyEntry FirstWrite = *Entry;

	Fixture.Viewer->SetProperty_Int(TEXT("Outsiders"), TEXT("kills"), 200, SetResult);

	TestFalse(TEXT("the older write's failure releases nothing"), Fixture.Viewer->ReleaseLocalWrite(FirstWrite));
	Fixture.Viewer->MergePropertiesFromData(FWriteFixture::MakeSnapshot(TEXT("7")));
	TestEqual(TEXT("the newer write still holds against a stale snapshot"), Fixture.Kills(), static_cast<int64>(200));

	const FTclViewerPropertyEntry SecondWrite = *Fixture.Viewer->Properties.Find(TEXT("C1/D1/viewer-1"));
	TestTrue(TEXT("the newer write's own failure releases it"), Fixture.Viewer->ReleaseLocalWrite(SecondWrite));
	Fixture.Viewer->MergePropertiesFromData(FWriteFixture::MakeSnapshot(TEXT("7")));
	TestEqual(TEXT("and the snapshot applies"), Fixture.Kills(), static_cast<int64>(7));

	TestFalse(TEXT("releasing again finds no hold"), Fixture.Viewer->ReleaseLocalWrite(SecondWrite));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
