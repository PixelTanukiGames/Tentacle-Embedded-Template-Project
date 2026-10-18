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
 * Race guard: a viewer whose slim properties are merged BEFORE the viewer-
 * property registry (collections + definitions) is cached must still resolve
 * once the registry loads. Reproduces the chat-message-before-registry ordering
 * that the normalized id-keyed payload introduced.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyRegistryRaceTest,
	"Tentacle.ViewerProperties.ResolvesPropertiesMergedBeforeRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyRegistryRaceTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();

	// The actor's constructor creates its ViewerProperties subobject; we never
	// BeginPlay it, so no world / HTTP / websocket is involved.
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	if (!TestNotNull(TEXT("ViewerProperties subobject"), Tentacle->ViewerProperties)) return false;
	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	// A viewer arrives with one slim property (collection "C1", definition "D1")
	// BEFORE the registry has loaded.
	UTclViewerData* Data = NewObject<UTclViewerData>(Outer);
	Data->Id = TEXT("viewer-1");
	UTclViewerPropertyValue* Slim = NewObject<UTclViewerPropertyValue>(Outer);
	Slim->Value = TEXT("42");
	Slim->bIsDefault = false;
	FTclViewerData_Properties Wrapper;
	Wrapper.Values.Add(TEXT("D1"), Slim);
	Data->Properties.Add(TEXT("C1"), Wrapper);

	UTclViewer* Viewer = Tentacle->UpsertViewer(Data);  // registers + merges (unresolved)
	if (!TestNotNull(TEXT("Viewer"), Viewer)) return false;

	// Before the registry: the property cannot be resolved by name yet
	// (self-healing window - the collection cache also isn't loaded).
	ETclFindPropertyResult Before = ETclFindPropertyResult::Found;
	Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), Before);
	TestTrue(TEXT("Unresolved before registry"), Before != ETclFindPropertyResult::Found);

	// The registry arrives: collection Outsiders=C1, definition kills=D1 (int).
	UTclViewerPropertyCollection* Col = NewObject<UTclViewerPropertyCollection>(Outer);
	Col->Id = TEXT("C1");
	Col->Name = TEXT("Outsiders");
	UTclViewerPropertyDefinition* Def = NewObject<UTclViewerPropertyDefinition>(Outer);
	Def->Id = TEXT("D1");
	Def->CollectionId = TEXT("C1");
	Def->Name = TEXT("kills");
	Def->Type = ETclViewerPropertyType::Int;
	VP->ApplyRegistry({ Col }, { Def });  // caches + re-merges every viewer

	// After the registry: the same lookup resolves to the value.
	ETclFindPropertyResult After = ETclFindPropertyResult::PropertyNotFound;
	const int64 Value = Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), After);
	TestTrue(TEXT("Resolved after registry"), After == ETclFindPropertyResult::Found);
	TestEqual(TEXT("Resolved value"), Value, static_cast<int64>(42));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
