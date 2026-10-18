// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_CustomControls.h"
#include "Base/TclCustomControlComponents.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** The option strings a control message carries, or an empty array. */
	TArray<FString> OptionsOf(const TSharedPtr<FJsonObject>& Control)
	{
		TArray<FString> Options;
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (Control.IsValid() && Control->TryGetArrayField(TEXT("options"), Values) && Values)
		{
			for (const TSharedPtr<FJsonValue>& Value : *Values)
			{
				FString Option;
				if (Value.IsValid() && Value->TryGetString(Option))
					Options.Add(Option);
			}
		}
		return Options;
	}
}

/**
 * On Initialize fires once the control has registered and its id exists, and a
 * value a handler assigns there directly still reaches Tentacle.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomControlOnInitializeTest,
	"Tentacle.CustomControls.OnInitializeFiresAfterRegistration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomControlOnInitializeTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	AActor* Host = World.Get()->SpawnActor<AActor>();
	if (!TestTrue(TEXT("actors spawned"), Tentacle && Host)) return false;

	UTclCustomControlComponent_Int* Control = NewObject<UTclCustomControlComponent_Int>(Host);
	Control->Value = 0;
	Control->RegisterComponent();

	bool bRegisteredAtInitialize = false;
	FString IdAtInitialize;
	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	Recorder->OnRecord = [&](const FTclRecordedCall&)
	{
		bRegisteredAtInitialize = Tentacle->CustomControls->CustomControls.Contains(Control);
		IdAtInitialize = Control->GetId();

		// Assigned directly, not through SetValue, so nothing pushes it.
		Control->Value = 42;
	};
	Control->OnInitialize.AddDynamic(Recorder, &UTclTestRecorder::OnInitialize);

	TclTest::FSentMessages Sent(Tentacle);
	Host->DispatchBeginPlay();

	TestEqual(TEXT("On Initialize fires once"), Recorder->Count(TEXT("OnInitialize")), 1);
	TestTrue(TEXT("the control is registered when On Initialize fires"), bRegisteredAtInitialize);
	TestFalse(TEXT("its id exists when On Initialize fires"), IdAtInitialize.IsEmpty());

	if (!TestEqual(TEXT("registration, then the state once more"), Sent.Num(), 2)) return false;
	TestEqual(TEXT("first the registration"), Sent.Type(0), FString(TEXT("CustomControls_RegisterControl")));
	TestEqual(TEXT("then the state"), Sent.Type(1), FString(TEXT("CustomControls_ControlState")));
	TestEqual(TEXT("the state is for this control"), TclTest::StringField(Sent.Control(1), TEXT("id")), IdAtInitialize);

	double PushedValue = 0.0;
	const TSharedPtr<FJsonObject> PushedControl = Sent.Control(1);
	TestTrue(TEXT("the state carries the value the handler assigned"),
		PushedControl.IsValid() && PushedControl->TryGetNumberField(TEXT("value"), PushedValue) && PushedValue == 42.0);

	Host->Destroy();
	return true;
}

/**
 * The documented pattern - fill a Select's options from On Initialize with Set
 * Options - still ends with Tentacle holding those options.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomControlOnInitializeSetOptionsTest,
	"Tentacle.CustomControls.OptionsSetInOnInitializeReachTentacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomControlOnInitializeSetOptionsTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	ATentacle* Tentacle = World.Get()->SpawnActor<ATentacle>();
	AActor* Host = World.Get()->SpawnActor<AActor>();
	if (!TestTrue(TEXT("actors spawned"), Tentacle && Host)) return false;

	UTclCustomControlComponent_Select* Control = NewObject<UTclCustomControlComponent_Select>(Host);
	Control->RegisterComponent();

	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	Recorder->OnRecord = [Control](const FTclRecordedCall&)
	{
		Control->SetOptions({ TEXT("Red"), TEXT("Green"), TEXT("Blue") });
	};
	Control->OnInitialize.AddDynamic(Recorder, &UTclTestRecorder::OnInitialize);

	TclTest::FSentMessages Sent(Tentacle);
	Host->DispatchBeginPlay();

	if (!TestTrue(TEXT("something was sent"), Sent.Num() > 0)) return false;
	TestEqual(TEXT("the control registers first"), Sent.Type(0), FString(TEXT("CustomControls_RegisterControl")));
	TestEqual(TEXT("the last state Tentacle holds carries the options"),
		FString::Join(OptionsOf(Sent.Control(Sent.Num() - 1)), TEXT(", ")), TEXT("Red, Green, Blue"));

	Host->Destroy();
	return true;
}

/**
 * On Initialize still fires when the level has no Tentacle actor, as it always
 * has.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomControlOnInitializeWithoutTentacleTest,
	"Tentacle.CustomControls.OnInitializeFiresWithoutATentacleActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomControlOnInitializeWithoutTentacleTest::RunTest(const FString& Parameters)
{
	TclTest::FWorld World;
	if (!TestNotNull(TEXT("test world"), World.Get())) return false;

	AActor* Host = World.Get()->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("actor spawned"), Host)) return false;

	UTclCustomControlComponent_Button* Control = NewObject<UTclCustomControlComponent_Button>(Host);
	Control->RegisterComponent();

	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(GetTransientPackage());
	Control->OnInitialize.AddDynamic(Recorder, &UTclTestRecorder::OnInitialize);

	Host->DispatchBeginPlay();
	TestEqual(TEXT("On Initialize fires without a Tentacle actor"), Recorder->Count(TEXT("OnInitialize")), 1);

	Host->Destroy();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
