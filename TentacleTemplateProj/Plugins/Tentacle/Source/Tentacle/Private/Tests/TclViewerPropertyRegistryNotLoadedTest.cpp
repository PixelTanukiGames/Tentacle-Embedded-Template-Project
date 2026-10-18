// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Base/TclViewer.h"
#include "Sdk/TclViewerData.h"
#include "Sdk/TclViewerData_Properties.h"
#include "Sdk/TclViewerPropertyValue.h"
#include "Sdk/TclViewerPropertyCollection.h"
#include "Sdk/TclViewerPropertyDefinition.h"
#include "Sdk/TclViewerPropertyType.h"

/**
 * A lookup made before the registry loads stays inside the three outcomes the
 * enum has always had.
 *
 * The state is transient and the plugin recovers from it without the game's
 * help, so a fourth outcome would put a branch in every Blueprint graph for a
 * case that resolves itself; the reason is reported to the log instead. What
 * this pins is that the early answer is an ordinary CollectionNotFound, and
 * that the two real failures still tell themselves apart once the registry is
 * in - which is what makes the outcome worth reading at all.
 *
 * TclViewerPropertyRaceTest covers the resolve-after-arrival path itself.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyRegistryNotLoadedTest,
	"Tentacle.ViewerProperties.AnswersWithoutAnOutcomeOfItsOwnBeforeTheRegistryLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyRegistryNotLoadedTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();

	// The actor's constructor creates its ViewerProperties subobject; we never
	// BeginPlay it, so no world / HTTP / websocket is involved - and crucially
	// no registry has been applied.
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	if (!TestNotNull(TEXT("ViewerProperties subobject"), Tentacle->ViewerProperties)) return false;
	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	TestFalse(TEXT("the registry starts unloaded"), VP->AreDefinitionsLoaded());

	// A viewer arrives carrying one slim property, before the registry.
	UTclViewerData* Data = NewObject<UTclViewerData>(Outer);
	Data->Id = TEXT("viewer-1");
	UTclViewerPropertyValue* Slim = NewObject<UTclViewerPropertyValue>(Outer);
	Slim->Value = TEXT("7");
	Slim->bIsDefault = false;
	FTclViewerData_Properties Wrapper;
	Wrapper.Values.Add(TEXT("D1"), Slim);
	Data->Properties.Add(TEXT("C1"), Wrapper);

	UTclViewer* Viewer = Tentacle->UpsertViewer(Data);
	if (!TestNotNull(TEXT("viewer"), Viewer)) return false;

	// Reads and writes both fall into the existing not-found outcome rather
	// than a new one.
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::Found;
	Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult);
	TestEqual(
		TEXT("a read before the registry loads"),
		FindResult,
		ETclFindPropertyResult::CollectionNotFound);

	ETclSetPropertyResult SetResult = ETclSetPropertyResult::Success;
	Viewer->SetProperty_Int(TEXT("Outsiders"), TEXT("kills"), 9, SetResult);
	TestEqual(
		TEXT("a write before the registry loads"),
		SetResult,
		ETclSetPropertyResult::CollectionNotFound);

	// The registry arrives: collection Outsiders=C1, definition kills=D1.
	UTclViewerPropertyCollection* Collection = NewObject<UTclViewerPropertyCollection>(Outer);
	Collection->Id = TEXT("C1");
	Collection->Name = TEXT("Outsiders");

	UTclViewerPropertyDefinition* Definition = NewObject<UTclViewerPropertyDefinition>(Outer);
	Definition->Id = TEXT("D1");
	Definition->CollectionId = TEXT("C1");
	Definition->Name = TEXT("kills");
	Definition->Type = ETclViewerPropertyType::Int;
	Definition->Value = TEXT("0");

	VP->ApplyRegistry({ Collection }, { Definition });
	TestTrue(TEXT("the registry is loaded"), VP->AreDefinitionsLoaded());

	// The same read now resolves, so the early answer was a state and not a
	// verdict - and the write above left nothing behind.
	const int64 Kills = Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult);
	TestEqual(TEXT("a read once the registry is in"), FindResult, ETclFindPropertyResult::Found);
	TestEqual(TEXT("the value the viewer arrived with"), Kills, static_cast<int64>(7));

	// And the two ordinary failures still tell themselves apart.
	Viewer->GetProperty_Int(TEXT("Nope"), TEXT("kills"), FindResult);
	TestEqual(
		TEXT("an unknown collection"),
		FindResult,
		ETclFindPropertyResult::CollectionNotFound);

	Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("nope"), FindResult);
	TestEqual(
		TEXT("an unknown property"),
		FindResult,
		ETclFindPropertyResult::PropertyNotFound);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
