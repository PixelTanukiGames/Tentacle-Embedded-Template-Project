// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/Package.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Obs.h"
#include "Tests/TclTestSupport.h"

/**
 * The Blueprint Call node sends an Obs_Call with the request data parsed from
 * its JSON text, and leaves requestData out entirely when the JSON is empty:
 * Tentacle's schema makes the field optional but not nullable, so an explicit
 * null would be dropped before it reached OBS.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclObsCallRequestTest,
	"Tentacle.Obs.CallRequestSendsTheRequestAndItsData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclObsCallRequestTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	TclTest::FSentMessages Sent(Tentacle);

	Tentacle->Obs->CallRequest(TEXT("SetInputMute"), TEXT("{\"inputName\":\"Mic\",\"inputMuted\":true}"));
	if (!TestEqual(TEXT("one message sent"), Sent.Num(), 1)) return false;
	TestEqual(TEXT("an OBS call"), Sent.Type(0), FString(TEXT("Obs_Call")));

	const TSharedPtr<FJsonObject> Data = Sent.Data(0);
	TestEqual(TEXT("request type"), TclTest::StringField(Data, TEXT("requestType")), FString(TEXT("SetInputMute")));
	const TSharedPtr<FJsonObject> RequestData = TclTest::ObjectField(Data, TEXT("requestData"));
	if (!TestNotNull(TEXT("request data is an object"), RequestData.Get())) return false;
	TestEqual(TEXT("request data: a string field"), TclTest::StringField(RequestData, TEXT("inputName")), FString(TEXT("Mic")));
	bool bMuted = false;
	TestTrue(TEXT("request data: a bool field"), RequestData->TryGetBoolField(TEXT("inputMuted"), bMuted) && bMuted);

	// Empty (or blank) JSON is a request with no fields: requestData is absent, not null.
	Tentacle->Obs->CallRequest(TEXT("GetVersion"), FString());
	Tentacle->Obs->CallRequest(TEXT("GetStats"), TEXT("   "));
	if (!TestEqual(TEXT("both field-less requests sent"), Sent.Num(), 3)) return false;
	TestEqual(TEXT("empty JSON: request type"), TclTest::StringField(Sent.Data(1), TEXT("requestType")), FString(TEXT("GetVersion")));
	TestFalse(TEXT("empty JSON: requestData left out"), Sent.Data(1)->HasField(TEXT("requestData")));
	TestFalse(TEXT("blank JSON: requestData left out"), Sent.Data(2)->HasField(TEXT("requestData")));

	return true;
}

/**
 * JSON that is not an object is refused with a warning and never sent, and so
 * is a call with no request type: a malformed request can only be rejected on
 * the far side of the socket, where the game never hears about it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclObsCallRequestRefusalTest,
	"Tentacle.Obs.CallRequestRefusesJsonThatIsNotAnObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclObsCallRequestRefusalTest::RunTest(const FString& Parameters)
{
	ATentacle* Tentacle = NewObject<ATentacle>(GetTransientPackage());
	TclTest::FSentMessages Sent(Tentacle);

	Tentacle->Obs->CallRequest(TEXT("SetInputMute"), TEXT("{\"inputName\":"));
	Tentacle->Obs->CallRequest(TEXT("SetInputMute"), TEXT("not json at all"));
	Tentacle->Obs->CallRequest(TEXT("SetInputMute"), TEXT("[1, 2, 3]"));
	Tentacle->Obs->CallRequest(TEXT("SetInputMute"), TEXT("42"));
	Tentacle->Obs->CallRequest(TEXT("SetInputMute"), TEXT("null"));
	Tentacle->Obs->CallRequest(FString(), TEXT("{}"));

	TestEqual(TEXT("nothing sent"), Sent.Num(), 0);

	// And the component still sends a good request afterwards.
	Tentacle->Obs->CallRequest(TEXT("GetVersion"), TEXT("{}"));
	TestEqual(TEXT("a good request still goes out"), Sent.Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
