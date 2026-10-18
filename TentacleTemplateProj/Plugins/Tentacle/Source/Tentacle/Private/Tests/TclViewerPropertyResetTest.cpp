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
 * Collection reset: after the server confirms viewerProperty.resetCollectionValues,
 * every cached viewer's entries in that collection revert to the definition
 * defaults - including pending optimistic local writes - while entries in other
 * collections are untouched. OnPropertyUpdated fires once per value that
 * actually changed, and a later re-merge of the stored viewer data must not
 * resurrect the pre-reset values.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyCollectionResetTest,
	"Tentacle.ViewerProperties.ResetCollectionRevertsCachedViewersToDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyCollectionResetTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();

	// The actor's constructor creates its ViewerProperties subobject; we never
	// BeginPlay it, so no world / HTTP / websocket is involved.
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	if (!TestNotNull(TEXT("ViewerProperties subobject"), Tentacle->ViewerProperties)) return false;
	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	// Registry: collection Outsiders=C1 (kills=D1 int default 0, title=D2 string
	// default "Rookie") and collection Other=C2 (gold=D3 int default 5).
	UTclViewerPropertyCollection* Col1 = NewObject<UTclViewerPropertyCollection>(Outer);
	Col1->Id = TEXT("C1");
	Col1->Name = TEXT("Outsiders");
	UTclViewerPropertyCollection* Col2 = NewObject<UTclViewerPropertyCollection>(Outer);
	Col2->Id = TEXT("C2");
	Col2->Name = TEXT("Other");

	UTclViewerPropertyDefinition* DefKills = NewObject<UTclViewerPropertyDefinition>(Outer);
	DefKills->Id = TEXT("D1");
	DefKills->CollectionId = TEXT("C1");
	DefKills->Name = TEXT("kills");
	DefKills->Type = ETclViewerPropertyType::Int;
	DefKills->Value = TEXT("0");

	UTclViewerPropertyDefinition* DefTitle = NewObject<UTclViewerPropertyDefinition>(Outer);
	DefTitle->Id = TEXT("D2");
	DefTitle->CollectionId = TEXT("C1");
	DefTitle->Name = TEXT("title");
	DefTitle->Type = ETclViewerPropertyType::String;
	DefTitle->Value = TEXT("Rookie");

	UTclViewerPropertyDefinition* DefGold = NewObject<UTclViewerPropertyDefinition>(Outer);
	DefGold->Id = TEXT("D3");
	DefGold->CollectionId = TEXT("C2");
	DefGold->Name = TEXT("gold");
	DefGold->Type = ETclViewerPropertyType::Int;
	DefGold->Value = TEXT("5");

	VP->ApplyRegistry({ Col1, Col2 }, { DefKills, DefTitle, DefGold });

	// A viewer arrives with overrides in both collections. bIsMock skips the
	// server sync on local writes (no HTTP client in this test).
	UTclViewerData* Data = NewObject<UTclViewerData>(Outer);
	Data->Id = TEXT("viewer-1");
	Data->bIsMock = true;

	UTclViewerPropertyValue* SlimKills = NewObject<UTclViewerPropertyValue>(Outer);
	SlimKills->Value = TEXT("42");
	SlimKills->bIsDefault = false;
	UTclViewerPropertyValue* SlimTitle = NewObject<UTclViewerPropertyValue>(Outer);
	SlimTitle->Value = TEXT("Veteran");
	SlimTitle->bIsDefault = false;
	FTclViewerData_Properties WrapperC1;
	WrapperC1.Values.Add(TEXT("D1"), SlimKills);
	WrapperC1.Values.Add(TEXT("D2"), SlimTitle);
	Data->Properties.Add(TEXT("C1"), WrapperC1);

	UTclViewerPropertyValue* SlimGold = NewObject<UTclViewerPropertyValue>(Outer);
	SlimGold->Value = TEXT("99");
	SlimGold->bIsDefault = false;
	FTclViewerData_Properties WrapperC2;
	WrapperC2.Values.Add(TEXT("D3"), SlimGold);
	Data->Properties.Add(TEXT("C2"), WrapperC2);

	UTclViewer* Viewer = Tentacle->UpsertViewer(Data);
	if (!TestNotNull(TEXT("Viewer"), Viewer)) return false;

	// Sanity: overrides resolved.
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("kills override"), Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult), static_cast<int64>(42));
	TestTrue(TEXT("kills found"), FindResult == ETclFindPropertyResult::Found);

	// A pending optimistic local write must not survive the reset either - the
	// server has deleted every override, including the one this write created.
	ETclSetPropertyResult SetResult = ETclSetPropertyResult::PropertyNotFound;
	Viewer->SetProperty_Int(TEXT("Outsiders"), TEXT("kills"), 100, SetResult);
	TestTrue(TEXT("local write succeeded"), SetResult == ETclSetPropertyResult::Success);

	// Count change events per collection from here on.
	int32 EventsC1 = 0;
	int32 EventsC2 = 0;
	Viewer->OnPropertyUpdated.Subscribe([&EventsC1, &EventsC2](const FTclViewerPropertyUpdatedData& EventData)
	{
		if (EventData.Collection && EventData.Collection->Id == TEXT("C1")) EventsC1++;
		if (EventData.Collection && EventData.Collection->Id == TEXT("C2")) EventsC2++;
	});

	// The reset (local-cache part; the HTTP round-trip is the server's side).
	Tentacle->ResetAllViewerPropertiesToDefaults(TEXT("C1"));

	// C1 entries reverted to the definition defaults.
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("kills reverted to default"), Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult), static_cast<int64>(0));
	TestTrue(TEXT("kills still found"), FindResult == ETclFindPropertyResult::Found);

	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("title reverted to default"), Viewer->GetProperty_String(TEXT("Outsiders"), TEXT("title"), FindResult), FString(TEXT("Rookie")));
	TestTrue(TEXT("title still found"), FindResult == ETclFindPropertyResult::Found);

	// The other collection is untouched.
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("gold untouched"), Viewer->GetProperty_Int(TEXT("Other"), TEXT("gold"), FindResult), static_cast<int64>(99));
	TestTrue(TEXT("gold still found"), FindResult == ETclFindPropertyResult::Found);

	// One event per changed C1 value (kills 100 -> 0, title Veteran -> Rookie),
	// none for C2.
	TestEqual(TEXT("C1 change events"), EventsC1, 2);
	TestEqual(TEXT("no C2 change events"), EventsC2, 0);

	// Idempotence: a later re-merge of the stored data must keep the defaults
	// and fire no further events (the reset rewrote the stored slim payload).
	Viewer->MergePropertiesFromData(Data);
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("kills stays default after re-merge"), Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult), static_cast<int64>(0));
	TestEqual(TEXT("no extra events after re-merge"), EventsC1, 2);

	return true;
}

/**
 * Definition reset: after the server confirms viewerProperty.resetPropertyValues,
 * every cached viewer's entry for that one definition reverts to its default -
 * including a pending optimistic local write - while sibling definitions in the
 * same collection and entries in other collections are untouched.
 * OnPropertyUpdated fires once per value that actually changed, and a later
 * re-merge of the stored viewer data must not resurrect the pre-reset value.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclViewerPropertyDefinitionResetTest,
	"Tentacle.ViewerProperties.ResetDefinitionRevertsCachedViewersToDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclViewerPropertyDefinitionResetTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();

	// The actor's constructor creates its ViewerProperties subobject; we never
	// BeginPlay it, so no world / HTTP / websocket is involved.
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	if (!TestNotNull(TEXT("ViewerProperties subobject"), Tentacle->ViewerProperties)) return false;
	UTentacle_ViewerProperties* VP = Tentacle->ViewerProperties;

	// Registry: collection Outsiders=C1 (kills=D1 int default 0, title=D2 string
	// default "Rookie") and collection Other=C2 (gold=D3 int default 5).
	UTclViewerPropertyCollection* Col1 = NewObject<UTclViewerPropertyCollection>(Outer);
	Col1->Id = TEXT("C1");
	Col1->Name = TEXT("Outsiders");
	UTclViewerPropertyCollection* Col2 = NewObject<UTclViewerPropertyCollection>(Outer);
	Col2->Id = TEXT("C2");
	Col2->Name = TEXT("Other");

	UTclViewerPropertyDefinition* DefKills = NewObject<UTclViewerPropertyDefinition>(Outer);
	DefKills->Id = TEXT("D1");
	DefKills->CollectionId = TEXT("C1");
	DefKills->Name = TEXT("kills");
	DefKills->Type = ETclViewerPropertyType::Int;
	DefKills->Value = TEXT("0");

	UTclViewerPropertyDefinition* DefTitle = NewObject<UTclViewerPropertyDefinition>(Outer);
	DefTitle->Id = TEXT("D2");
	DefTitle->CollectionId = TEXT("C1");
	DefTitle->Name = TEXT("title");
	DefTitle->Type = ETclViewerPropertyType::String;
	DefTitle->Value = TEXT("Rookie");

	UTclViewerPropertyDefinition* DefGold = NewObject<UTclViewerPropertyDefinition>(Outer);
	DefGold->Id = TEXT("D3");
	DefGold->CollectionId = TEXT("C2");
	DefGold->Name = TEXT("gold");
	DefGold->Type = ETclViewerPropertyType::Int;
	DefGold->Value = TEXT("5");

	VP->ApplyRegistry({ Col1, Col2 }, { DefKills, DefTitle, DefGold });

	// The definition-by-name lookup ResetPropertyValues relies on resolves within the
	// right collection, case-insensitively.
	TestEqual(TEXT("find definition by name"), VP->FindDefinitionByName(TEXT("C1"), TEXT("KILLS")), DefKills);
	TestNull(TEXT("no such definition in other collection"), VP->FindDefinitionByName(TEXT("C2"), TEXT("kills")));

	// A viewer arrives with overrides in both collections. bIsMock skips the
	// server sync on local writes (no HTTP client in this test).
	UTclViewerData* Data = NewObject<UTclViewerData>(Outer);
	Data->Id = TEXT("viewer-1");
	Data->bIsMock = true;

	UTclViewerPropertyValue* SlimKills = NewObject<UTclViewerPropertyValue>(Outer);
	SlimKills->Value = TEXT("42");
	SlimKills->bIsDefault = false;
	UTclViewerPropertyValue* SlimTitle = NewObject<UTclViewerPropertyValue>(Outer);
	SlimTitle->Value = TEXT("Veteran");
	SlimTitle->bIsDefault = false;
	FTclViewerData_Properties WrapperC1;
	WrapperC1.Values.Add(TEXT("D1"), SlimKills);
	WrapperC1.Values.Add(TEXT("D2"), SlimTitle);
	Data->Properties.Add(TEXT("C1"), WrapperC1);

	UTclViewerPropertyValue* SlimGold = NewObject<UTclViewerPropertyValue>(Outer);
	SlimGold->Value = TEXT("99");
	SlimGold->bIsDefault = false;
	FTclViewerData_Properties WrapperC2;
	WrapperC2.Values.Add(TEXT("D3"), SlimGold);
	Data->Properties.Add(TEXT("C2"), WrapperC2);

	UTclViewer* Viewer = Tentacle->UpsertViewer(Data);
	if (!TestNotNull(TEXT("Viewer"), Viewer)) return false;

	// Sanity: overrides resolved.
	ETclFindPropertyResult FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("kills override"), Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult), static_cast<int64>(42));
	TestTrue(TEXT("kills found"), FindResult == ETclFindPropertyResult::Found);

	// A pending optimistic local write must not survive the reset either - the
	// server has deleted the definition's override, including the one this
	// write created.
	ETclSetPropertyResult SetResult = ETclSetPropertyResult::PropertyNotFound;
	Viewer->SetProperty_Int(TEXT("Outsiders"), TEXT("kills"), 100, SetResult);
	TestTrue(TEXT("local write succeeded"), SetResult == ETclSetPropertyResult::Success);

	// Count change events per collection from here on.
	int32 EventsC1 = 0;
	int32 EventsC2 = 0;
	Viewer->OnPropertyUpdated.Subscribe([&EventsC1, &EventsC2](const FTclViewerPropertyUpdatedData& EventData)
	{
		if (EventData.Collection && EventData.Collection->Id == TEXT("C1")) EventsC1++;
		if (EventData.Collection && EventData.Collection->Id == TEXT("C2")) EventsC2++;
	});

	// The reset (local-cache part; the HTTP round-trip is the server's side).
	Tentacle->ResetAllViewerPropertiesToDefaultForDefinition(TEXT("D1"));

	// The definition's entry reverted to its default.
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("kills reverted to default"), Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult), static_cast<int64>(0));
	TestTrue(TEXT("kills still found"), FindResult == ETclFindPropertyResult::Found);

	// The sibling definition in the same collection is untouched.
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("title untouched"), Viewer->GetProperty_String(TEXT("Outsiders"), TEXT("title"), FindResult), FString(TEXT("Veteran")));
	TestTrue(TEXT("title still found"), FindResult == ETclFindPropertyResult::Found);

	// The other collection is untouched.
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("gold untouched"), Viewer->GetProperty_Int(TEXT("Other"), TEXT("gold"), FindResult), static_cast<int64>(99));
	TestTrue(TEXT("gold still found"), FindResult == ETclFindPropertyResult::Found);

	// One event for the changed value (kills 100 -> 0), none for anything else.
	TestEqual(TEXT("C1 change events"), EventsC1, 1);
	TestEqual(TEXT("no C2 change events"), EventsC2, 0);

	// Idempotence: a later re-merge of the stored data must keep the default
	// and fire no further events (the reset rewrote the stored slim payload).
	Viewer->MergePropertiesFromData(Data);
	FindResult = ETclFindPropertyResult::PropertyNotFound;
	TestEqual(TEXT("kills stays default after re-merge"), Viewer->GetProperty_Int(TEXT("Outsiders"), TEXT("kills"), FindResult), static_cast<int64>(0));
	TestEqual(TEXT("no extra events after re-merge"), EventsC1, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
