// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_CustomControls.h"
#include "Base/TclCustomControlComponents.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonWriter.h"
#include "Sdk/TclMsg_ServerToClient_CustomControlsControlEvent.h"
#include "Tests/TclTestDoubles.h"
#include "Tests/TclTestSupport.h"

namespace
{
	/** Deliver a value event for a control, the way one arrives from Tentacle. */
	void SendControlEvent(ATentacle* Tentacle, const FString& Kind, const FString& ControlId, const FString& ValueJson)
	{
		auto* Msg = TclTest::ParseMessage<UTclMsg_ServerToClient_CustomControlsControlEvent>(
			GetTransientPackage(), TclTest::ControlEventMessage(Kind, ControlId, ValueJson));
		Tentacle->CustomControls->HandleMsg_ControlEvent(Msg);
	}

	/** The control's wire copy: what Tentacle is sent for it, as text. */
	FString WireCopy(const UTclCustomControlComponent* Control)
	{
		FString Json;
		const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
		FJsonSerializer::Serialize(Control->SerializeToJson(), Writer);
		return Json;
	}

	/**
	 * Drive one typed control through a refused and an accepted value, for both
	 * value events, and check the refusal left the control and its wire copy as
	 * they were. ValueJson is what Tentacle sends; Typed is how the typed check
	 * should see it.
	 */
	template <typename TControl>
	void CheckTypedVeto(FAutomationTestBase& Test, const FString& Type, const FString& ValueJson, const FString& Typed)
	{
		UObject* Outer = GetTransientPackage();
		ATentacle* Tentacle = NewObject<ATentacle>(Outer);
		TControl* Control = NewObject<TControl>(Tentacle);
		Control->ControlId = TEXT("ctl-") + Type;
		Tentacle->CustomControls->RegisterCustomControl(Control);

		TclTest::FSentMessages Sent(Tentacle);

		for (const FString Kind : { FString(TEXT("ValueChange")), FString(TEXT("ValueCommit")) })
		{
			const FString What = Type + TEXT(" ") + Kind;

			// Refused by the typed check: the raw check ran first and passed, the
			// typed one saw the converted value, and nothing changed.
			Control->Probe.Calls.Reset();
			Control->Probe.bAcceptRaw = true;
			Control->Probe.bAcceptTyped = false;
			const FString Before = WireCopy(Control);
			SendControlEvent(Tentacle, Kind, Control->ControlId, ValueJson);

			Test.TestEqual(What + TEXT(": the raw check runs, then the typed one with the converted value"),
				FString::Join(Control->Probe.Calls, TEXT(" | ")),
				TEXT("raw:") + ValueJson.TrimChar(TEXT('"')) + TEXT(" | typed:") + Typed);
			Test.TestEqual(What + TEXT(": a refusal leaves the control and its wire copy as they were"), WireCopy(Control), Before);
			Test.TestEqual(What + TEXT(": a refusal sends nothing"), Sent.Num(), 0);

			// Refused by the raw check: the typed check is never asked.
			Control->Probe.Calls.Reset();
			Control->Probe.bAcceptRaw = false;
			Control->Probe.bAcceptTyped = true;
			SendControlEvent(Tentacle, Kind, Control->ControlId, ValueJson);
			Test.TestEqual(What + TEXT(": a raw refusal stops before the typed check"), Control->Probe.Calls.Num(), 1);
			Test.TestEqual(What + TEXT(": a raw refusal leaves the wire copy as it was"), WireCopy(Control), Before);

			// Accepted by both: the value lands.
			Control->Probe.Calls.Reset();
			Control->Probe.bAcceptRaw = true;
			Control->Probe.bAcceptTyped = true;
			SendControlEvent(Tentacle, Kind, Control->ControlId, ValueJson);
			Test.TestFalse(What + TEXT(": an accepted value changes the wire copy"), WireCopy(Control).Equals(Before, ESearchCase::CaseSensitive));

			// Put the control back so the next event kind starts from the old
			// value. Assigned directly: this control has no world to push from.
			Control->Value = GetDefault<TControl>()->Value;
			Sent.Reset();
		}
	}
}

/**
 * Every typed control asks its typed Should Receive Value, with the converted
 * value, after the raw check; a refusal from either leaves the control and the
 * copy Tentacle is sent exactly as they were.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomControlTypedVetoTest,
	"Tentacle.CustomControls.TypedVetoRefusalLeavesControlAndWireUnchanged",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomControlTypedVetoTest::RunTest(const FString& Parameters)
{
	CheckTypedVeto<UTclTestControl_Color>(*this, TEXT("Color"), TEXT("\"#FF8000\""), TEXT("FF8000FF"));
	CheckTypedVeto<UTclTestControl_Int>(*this, TEXT("Int"), TEXT("42"), TEXT("42"));
	CheckTypedVeto<UTclTestControl_Number>(*this, TEXT("Number"), TEXT("2.5"), TEXT("2.5"));
	CheckTypedVeto<UTclTestControl_Select>(*this, TEXT("Select"), TEXT("\"Option 2\""), TEXT("Option 2"));
	CheckTypedVeto<UTclTestControl_Slider>(*this, TEXT("Slider"), TEXT("75"), TEXT("75.0"));
	CheckTypedVeto<UTclTestControl_Switch>(*this, TEXT("Switch"), TEXT("true"), TEXT("true"));
	CheckTypedVeto<UTclTestControl_Text>(*this, TEXT("Text"), TEXT("\"hello\""), TEXT("hello"));
	CheckTypedVeto<UTclTestControl_Vector2>(*this, TEXT("Vector2"), TEXT("[1,2]"), TEXT("1,2"));
	CheckTypedVeto<UTclTestControl_Vector3>(*this, TEXT("Vector3"), TEXT("[1,2,3]"), TEXT("1,2,3"));
	CheckTypedVeto<UTclTestControl_Vector4>(*this, TEXT("Vector4"), TEXT("[1,2,3,4]"), TEXT("1,2,3,4"));
	return true;
}

/**
 * An accepted value still fires the value events with the value it replaced,
 * and a typed refusal fires neither.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCustomControlTypedVetoEventsTest,
	"Tentacle.CustomControls.TypedVetoRefusalFiresNoValueEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCustomControlTypedVetoEventsTest::RunTest(const FString& Parameters)
{
	UObject* Outer = GetTransientPackage();
	ATentacle* Tentacle = NewObject<ATentacle>(Outer);
	UTclTestControl_Int* Control = NewObject<UTclTestControl_Int>(Tentacle);
	Control->ControlId = TEXT("ctl-int");
	Control->Value = 7;
	Tentacle->CustomControls->RegisterCustomControl(Control);

	UTclTestRecorder* Recorder = NewObject<UTclTestRecorder>(Outer);
	Control->OnValueChangedFromApp.AddDynamic(Recorder, &UTclTestRecorder::OnIntValueChangedFromApp);
	Control->OnValueCommittedFromApp.AddDynamic(Recorder, &UTclTestRecorder::OnIntValueCommittedFromApp);

	Control->Probe.bAcceptTyped = false;
	SendControlEvent(Tentacle, TEXT("ValueChange"), TEXT("ctl-int"), TEXT("99"));
	SendControlEvent(Tentacle, TEXT("ValueCommit"), TEXT("ctl-int"), TEXT("99"));
	TestEqual(TEXT("a typed refusal fires no value event"), Recorder->Calls.Num(), 0);
	TestEqual(TEXT("and keeps the value"), Control->Value, 7);

	Control->Probe.bAcceptTyped = true;
	SendControlEvent(Tentacle, TEXT("ValueChange"), TEXT("ctl-int"), TEXT("99"));
	SendControlEvent(Tentacle, TEXT("ValueCommit"), TEXT("ctl-int"), TEXT("100"));
	TestEqual(TEXT("accepted values fire both events"), FString::Join(Recorder->Names(), TEXT(", ")), TEXT("OnValueChangedFromApp, OnValueCommittedFromApp"));
	TestEqual(TEXT("the change carries the value it replaced"), Recorder->Calls.Num() > 0 ? Recorder->Calls[0].Detail : FString(), FString(TEXT("99<-7")));
	TestEqual(TEXT("the value landed"), Control->Value, 100);

	return true;
}

/**
 * Find Custom Control By Id finds registered controls only.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclFindCustomControlByIdTest,
	"Tentacle.CustomControls.FindCustomControlByIdFindsRegisteredControls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclFindCustomControlByIdTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	UTentacle_CustomControls* Controls = Tentacle->CustomControls;

	UTclCustomControlComponent_Int* First = NewObject<UTclCustomControlComponent_Int>(Tentacle);
	First->ControlId = TEXT("ctl-a");
	UTclCustomControlComponent_Slider* Second = NewObject<UTclCustomControlComponent_Slider>(Tentacle);
	Second->ControlId = TEXT("ctl-b");
	Controls->RegisterCustomControl(First);
	Controls->RegisterCustomControl(Second);

	bool bFound = false;
	TestTrue(TEXT("finds the second control"), Controls->FindCustomControlById(TEXT("ctl-b"), bFound) == Second && bFound);
	TestTrue(TEXT("finds the first control"), Controls->FindCustomControlById(TEXT("ctl-a"), bFound) == First && bFound);

	TestNull(TEXT("an unknown id finds nothing"), Controls->FindCustomControlById(TEXT("ctl-nope"), bFound));
	TestFalse(TEXT("and says so"), bFound);

	Controls->UnregisterCustomControl(Second);
	TestNull(TEXT("an unregistered control is not found"), Controls->FindCustomControlById(TEXT("ctl-b"), bFound));
	TestFalse(TEXT("and says so"), bFound);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
