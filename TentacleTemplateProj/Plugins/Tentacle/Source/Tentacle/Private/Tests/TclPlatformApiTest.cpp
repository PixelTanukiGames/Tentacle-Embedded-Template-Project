// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/PlatformApi/TclKickApiTypes.h"
#include "Sdk/PlatformApi/TclTwitchApiActions.h"
#include "Sdk/PlatformApi/TclTwitchApiTypes.h"
#include "TclTestDoubles.h"
#include "TclTestSupport.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"

namespace
{
	/** A JSON object's text, condensed, as the request nodes send it. */
	FString Condensed(const TSharedRef<FJsonObject>& Object)
	{
		FString Text;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
		FJsonSerializer::Serialize(Object, Writer);
		return Text;
	}

	/** One stream as Twitch's Get Streams answers with it. */
	const TCHAR* StreamJson =
		TEXT("{\"id\":\"1\",\"user_id\":\"2\",\"user_login\":\"streamer\",\"user_name\":\"Streamer\",\"game_id\":\"3\",")
		TEXT("\"game_name\":\"Game\",\"type\":\"live\",\"title\":\"Hello\",\"tags\":[\"English\",\"Chill\"],\"viewer_count\":12,")
		TEXT("\"started_at\":\"2026-01-01T00:00:00Z\",\"language\":\"en\",\"thumbnail_url\":\"https://example.test/{width}x{height}.jpg\",")
		TEXT("\"is_mature\":true}");

	/** What one request node saw of the transport: the call it made and how it was answered. */
	struct FTransportProbe
	{
		FString Procedure;
		bool bIsQuery = false;
		FString InputJson;
		int32 Calls = 0;

		/** The status to fail with, or 0 to succeed with Body. */
		int32 FailStatus = 0;
		FString Body;
	};

	/**
	 * A Tentacle actor in a bare world whose procedure calls go to a probe that
	 * answers at once, as the embedded session's carrier does.
	 */
	struct FNodeFixture
	{
		TclTest::FWorld World;
		ATentacle* Tentacle = nullptr;
		TStrongObjectPtr<UTclTestRecorder> Recorder;
		TSharedRef<FTransportProbe> Probe = MakeShared<FTransportProbe>();

		FNodeFixture()
		{
			Tentacle = World.Get()->SpawnActor<ATentacle>();
			UTentacleHttpClient* Http = NewObject<UTentacleHttpClient>(Tentacle);
			Tentacle->HttpClient = Http;
			TSharedRef<FTransportProbe> Seen = Probe;
			Http->SetProcedureTransport([Seen](const FString& Procedure, bool bIsQuery, const FString& InputJson, const UTentacleHttpClient::FOnSuccess& OnSuccess, const UTentacleHttpClient::FOnError& OnError)
			{
				Seen->Procedure = Procedure;
				Seen->bIsQuery = bIsQuery;
				Seen->InputJson = InputJson;
				++Seen->Calls;
				if (Seen->FailStatus != 0)
				{
					OnError.ExecuteIfBound(Seen->FailStatus, Seen->Body);
				}
				else
				{
					OnSuccess.ExecuteIfBound(Seen->Body);
				}
			});
			Recorder.Reset(NewObject<UTclTestRecorder>());
		}
	};
}

/**
 * A request sends only the optional fields that are set, under the platform's
 * own names, so a field a game never touched is not sent as a zero.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclPlatformApiRequestFieldsTest,
	"Tentacle.PlatformApi.RequestSendsOnlyTheFieldsItSets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclPlatformApiRequestFieldsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("an untouched request sends nothing"), Condensed(FTclTwitchApiGetStreamsInput().ToJson()), FString(TEXT("{}")));

	FTclTwitchApiGetStreamsInput Input;
	Input.bHasFirst = true;
	Input.First = 5;
	Input.bHasUserLogin = true;
	Input.UserLogin = { TEXT("one"), TEXT("two") };
	Input.Type = TEXT("live");
	TestEqual(TEXT("the set fields, by their JSON keys"), Condensed(Input.ToJson()), FString(TEXT("{\"user_login\":[\"one\",\"two\"],\"first\":5}")));

	FTclKickApiGetChannelRewardRedemptionsInput Redemptions;
	Redemptions.bHasStatus = true;
	Redemptions.Status = ETclKickApiRedemptionStatus::Accepted;
	TestEqual(TEXT("an enum goes as the platform's string"), Condensed(Redemptions.ToJson()), FString(TEXT("{\"status\":\"accepted\"}")));
	return true;
}

/**
 * An answer reads every field the platform sent, nested types and arrays
 * included, and its toggles say which optional fields were there.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclPlatformApiAnswerFieldsTest,
	"Tentacle.PlatformApi.AnswerReadsTheEndpointsFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclPlatformApiAnswerFieldsTest::RunTest(const FString& Parameters)
{
	const FTclTwitchApiGetStreamsOutput Output = FTclTwitchApiGetStreamsOutput::FromJson(
		TclTest::ParseObject(FString::Printf(TEXT("{\"data\":[%s],\"pagination\":{\"cursor\":\"next\"}}"), StreamJson)));
	if (!TestEqual(TEXT("one stream"), Output.Data.Num(), 1)) return false;
	const FTclTwitchApiStream& Stream = Output.Data[0];
	TestEqual(TEXT("its login"), Stream.UserLogin, FString(TEXT("streamer")));
	TestEqual(TEXT("its viewer count"), Stream.ViewerCount, 12);
	TestTrue(TEXT("a boolean"), Stream.bIsMature);
	TestEqual(TEXT("an array"), Stream.Tags, TArray<FString>{ TEXT("English"), TEXT("Chill") });
	TestTrue(TEXT("the cursor was sent"), Output.Pagination.bHasCursor);
	TestEqual(TEXT("and read"), Output.Pagination.Cursor, FString(TEXT("next")));

	const FTclTwitchApiGetStreamsOutput LastPage = FTclTwitchApiGetStreamsOutput::FromJson(TclTest::ParseObject(TEXT("{\"data\":[],\"pagination\":{}}")));
	TestFalse(TEXT("the last page has no cursor"), LastPage.Pagination.bHasCursor);

	// A container of containers, which a UPROPERTY cannot hold, keeps its JSON.
	const FTclTwitchApiCheermoteTier Tier = FTclTwitchApiCheermoteTier::FromJson(TclTest::ParseObject(
		TEXT("{\"min_bits\":100,\"id\":\"100\",\"color\":\"#9c3ee8\",\"images\":{\"dark\":{\"static\":{\"1\":\"a.png\"}}},")
		TEXT("\"can_cheer\":true,\"show_in_bits_card\":true}")));
	TestEqual(TEXT("a nested container's JSON"), Tier.Images, FString(TEXT("{\"dark\":{\"static\":{\"1\":\"a.png\"}}}")));
	TestEqual(TEXT("which writes back as JSON"), Condensed(Tier.ToJson()).Contains(TEXT("\"images\":{\"dark\":{\"static\":{\"1\":\"a.png\"}}}")), true);
	return true;
}

/**
 * A request node sends its endpoint's procedure with the request's fields and
 * fires On Success with the platform's answer.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclPlatformApiNodeSucceedsTest,
	"Tentacle.PlatformApi.NodeSendsItsProcedureAndFiresOnSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclPlatformApiNodeSucceedsTest::RunTest(const FString& Parameters)
{
	FNodeFixture Fixture;
	Fixture.Probe->Body = FString::Printf(TEXT("{\"result\":{\"data\":{\"data\":[%s],\"pagination\":{\"cursor\":\"abc\"}}}}"), StreamJson);

	FTclTwitchApiGetStreamsInput Input;
	Input.bHasFirst = true;
	Input.First = 5;
	UTclTwitch_GetStreams* Node = UTclTwitch_GetStreams::GetStreams(Fixture.Tentacle, Input);
	Node->OnSuccess.AddDynamic(Fixture.Recorder.Get(), &UTclTestRecorder::OnGetStreams);
	Node->OnFailure.AddDynamic(Fixture.Recorder.Get(), &UTclTestRecorder::OnPlatformRequestFailed);
	Node->Activate();

	TestEqual(TEXT("one call"), Fixture.Probe->Calls, 1);
	TestEqual(TEXT("to the endpoint's procedure"), Fixture.Probe->Procedure, FString(TEXT("twitch.getStreams")));
	TestTrue(TEXT("as a query"), Fixture.Probe->bIsQuery);
	TestEqual(TEXT("with the fields set"), Fixture.Probe->InputJson, FString(TEXT("{\"first\":5}")));
	TestEqual(TEXT("On Success fired, and only it"), Fixture.Recorder->Names(), TArray<FString>{ TEXT("OnGetStreams") });
	const FTclRecordedCall* Call = Fixture.Recorder->Find(TEXT("OnGetStreams"));
	if (TestNotNull(TEXT("the answer"), Call))
	{
		TestEqual(TEXT("with Twitch's answer, status 200, and no message"), Call->Detail, FString(TEXT("1|streamer|abc|200|")));
	}
	return true;
}

/**
 * A request whose endpoint answers with no body is a mutation, and fires the
 * plain On Success.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclPlatformApiNodeWithoutBodyTest,
	"Tentacle.PlatformApi.NodeWithoutAnAnswerFiresOnSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclPlatformApiNodeWithoutBodyTest::RunTest(const FString& Parameters)
{
	FNodeFixture Fixture;
	Fixture.Probe->Body = TEXT("{\"result\":{}}");

	FTclTwitchApiSendChatAnnouncementInput Input;
	Input.Message = TEXT("Hello chat");
	UTclTwitch_SendChatAnnouncement* Node = UTclTwitch_SendChatAnnouncement::SendChatAnnouncement(Fixture.Tentacle, Input);
	Node->OnSuccess.AddDynamic(Fixture.Recorder.Get(), &UTclTestRecorder::OnPlatformRequestSucceeded);
	Node->OnFailure.AddDynamic(Fixture.Recorder.Get(), &UTclTestRecorder::OnPlatformRequestFailed);
	Node->Activate();

	TestEqual(TEXT("to the endpoint's procedure"), Fixture.Probe->Procedure, FString(TEXT("twitch.sendChatAnnouncement")));
	TestFalse(TEXT("as a mutation"), Fixture.Probe->bIsQuery);
	TestTrue(TEXT("carrying the message"), Fixture.Probe->InputJson.Contains(TEXT("\"message\":\"Hello chat\"")));
	TestEqual(TEXT("On Success fired, and only it"), Fixture.Recorder->Names(), TArray<FString>{ TEXT("OnPlatformRequestSucceeded") });
	const FTclRecordedCall* Call = Fixture.Recorder->Find(TEXT("OnPlatformRequestSucceeded"));
	if (TestNotNull(TEXT("the success"), Call))
	{
		TestEqual(TEXT("with status 200 and no message"), Call->Detail, FString(TEXT("200|")));
	}
	return true;
}

/**
 * A request Tentacle refuses fires On Failure with its status and Tentacle's own
 * message, and a level without a Tentacle actor fails the same way with status 0.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclPlatformApiNodeFailsTest,
	"Tentacle.PlatformApi.NodeFiresOnFailureWithTheReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclPlatformApiNodeFailsTest::RunTest(const FString& Parameters)
{
	// Only the missing actor is logged here: a refusal is logged by the transport
	// that carried it, which this test stands in for.
	AddExpectedError(TEXT("No Tentacle actor in the level"), EAutomationExpectedErrorFlags::Contains, 1);

	{
		FNodeFixture Fixture;
		Fixture.Probe->FailStatus = 502;
		Fixture.Probe->Body = TEXT("{\"error\":{\"message\":\"Twitch refused the request\",\"code\":-32603,\"data\":{\"code\":\"BAD_GATEWAY\",\"httpStatus\":502}}}");

		UTclTwitch_GetStreams* Node = UTclTwitch_GetStreams::GetStreams(Fixture.Tentacle, FTclTwitchApiGetStreamsInput());
		Node->OnSuccess.AddDynamic(Fixture.Recorder.Get(), &UTclTestRecorder::OnGetStreams);
		Node->OnFailure.AddDynamic(Fixture.Recorder.Get(), &UTclTestRecorder::OnPlatformRequestFailed);
		Node->Activate();

		TestEqual(TEXT("an untouched request sends an empty object"), Fixture.Probe->InputJson, FString(TEXT("{}")));
		TestEqual(TEXT("On Failure fired, and only it"), Fixture.Recorder->Names(), TArray<FString>{ TEXT("OnPlatformRequestFailed") });
		const FTclRecordedCall* Call = Fixture.Recorder->Find(TEXT("OnPlatformRequestFailed"));
		if (TestNotNull(TEXT("the failure"), Call))
		{
			TestEqual(TEXT("with the status and Tentacle's message"), Call->Detail, FString(TEXT("502|Twitch refused the request")));
		}
	}

	{
		TclTest::FWorld Empty;
		TStrongObjectPtr<UTclTestRecorder> Recorder(NewObject<UTclTestRecorder>());
		UTclTwitch_GetStreams* Node = UTclTwitch_GetStreams::GetStreams(Empty.Get(), FTclTwitchApiGetStreamsInput());
		Node->OnFailure.AddDynamic(Recorder.Get(), &UTclTestRecorder::OnPlatformRequestFailed);
		Node->Activate();

		const FTclRecordedCall* Call = Recorder->Find(TEXT("OnPlatformRequestFailed"));
		if (TestNotNull(TEXT("no Tentacle actor fails the request"), Call))
		{
			TestTrue(TEXT("with status 0"), Call->Detail.StartsWith(TEXT("0|")));
		}
	}
	return true;
}

/**
 * Every request node shows On Failure's status code and message in Blueprint.
 * An async action node takes its data pins from the first delegate its class
 * declares and fills each exec pin's from the delegate behind it by name, so the
 * first delegate has to carry On Failure's parameters too, or Blueprint never
 * sees why a request failed.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclPlatformApiNodePinsTest,
	"Tentacle.PlatformApi.EveryNodeShowsWhyItFailedInBlueprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclPlatformApiNodePinsTest::RunTest(const FString& Parameters)
{
	const UFunction* FailureSignature = nullptr;
	if (const FMulticastDelegateProperty* OnFailure = CastField<FMulticastDelegateProperty>(
			UTclPlatformRequestAction::StaticClass()->FindPropertyByName(TEXT("OnFailure"))))
	{
		FailureSignature = OnFailure->SignatureFunction;
	}
	if (!TestNotNull(TEXT("On Failure"), FailureSignature))
	{
		return false;
	}

	TArray<UClass*> Nodes;
	GetDerivedClasses(UTclPlatformRequestAction::StaticClass(), Nodes);
	TestTrue(TEXT("every Twitch and Kick request has a node"), Nodes.Num() > 100);
	for (const UClass* Node : Nodes)
	{
		// The delegate K2Node_BaseAsyncTask reads the node's data pins from.
		const FMulticastDelegateProperty* First = nullptr;
		for (TFieldIterator<FProperty> It(Node); It && First == nullptr; ++It)
		{
			First = CastField<FMulticastDelegateProperty>(*It);
		}
		if (!TestNotNull(*FString::Printf(TEXT("%s has a delegate"), *Node->GetName()), First))
		{
			continue;
		}
		for (TFieldIterator<FProperty> Param(FailureSignature); Param && Param->HasAnyPropertyFlags(CPF_Parm); ++Param)
		{
			const FProperty* Pin = First->SignatureFunction->FindPropertyByName(Param->GetFName());
			TestTrue(
				*FString::Printf(TEXT("%s shows On Failure's %s"), *Node->GetName(), *Param->GetName()),
				Pin != nullptr && Pin->SameType(*Param));
		}
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
