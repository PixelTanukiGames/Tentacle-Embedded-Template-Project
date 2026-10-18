// © 2026 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/TclPlatformRequestAction.h"
#include "Base/Tentacle.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "K2Node_AsyncAction.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_TclPlatformRequest.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Sdk/PlatformApi/TclTwitchApiActions.h"
#include "Sdk/PlatformApi/TclTwitchApiTypes.h"
#include "Sdk/TclHttpClient.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"

namespace
{
	/** The custom event each test Blueprint sends its request from. */
	const FName SendEventName(TEXT("SendRequest"));

	/** What a Blueprint's request reached the transport with. */
	struct FTransportProbe
	{
		FString Procedure;
		FString InputJson;
		int32 Calls = 0;
	};

	/**
	 * A bare game world holding a Tentacle actor whose procedure calls go to a
	 * probe that answers at once. Nothing begins play, so nothing dials out.
	 */
	struct FRequestWorld
	{
		UWorld* World = nullptr;
		ATentacle* Tentacle = nullptr;
		TSharedRef<FTransportProbe> Probe = MakeShared<FTransportProbe>();

		FRequestWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (GEngine)
			{
				GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			}
			Tentacle = World->SpawnActor<ATentacle>();
			UTentacleHttpClient* Http = NewObject<UTentacleHttpClient>(Tentacle);
			Tentacle->HttpClient = Http;
			const TSharedRef<FTransportProbe> Seen = Probe;
			Http->SetProcedureTransport([Seen](const FString& Procedure, bool, const FString& InputJson, const UTentacleHttpClient::FOnSuccess& OnSuccess, const UTentacleHttpClient::FOnError&)
			{
				Seen->Procedure = Procedure;
				Seen->InputJson = InputJson;
				++Seen->Calls;
				OnSuccess.ExecuteIfBound(TEXT("{\"result\":{\"data\":{\"data\":[]}}}"));
			});
		}

		~FRequestWorld()
		{
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
			World->DestroyWorld(false);
		}

		FRequestWorld(const FRequestWorld&) = delete;
		FRequestWorld& operator=(const FRequestWorld&) = delete;

		/** Spawns an actor of a compiled Blueprint here and runs its send event. */
		void Send(UBlueprint* Blueprint) const
		{
			AActor* Actor = World->SpawnActor<AActor>(Blueprint->GeneratedClass);
			// An actor runs Blueprint only once its world has begun play, which a
			// test world never does.
			FEditorScriptExecutionGuard ScriptGuard;
			Actor->ProcessEvent(Actor->FindFunctionChecked(SendEventName), nullptr);
		}
	};

	/** An actor Blueprint with a custom event, SendRequest, to send a request from. */
	struct FRequestBlueprint
	{
		TStrongObjectPtr<UBlueprint> Blueprint;
		UEdGraph* Graph = nullptr;
		UK2Node_CustomEvent* SendEvent = nullptr;

		FRequestBlueprint()
		{
			Blueprint.Reset(FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(),
				GetTransientPackage(),
				MakeUniqueObjectName(GetTransientPackage(), UBlueprint::StaticClass(), TEXT("TclRequestNodeTest")),
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass()));
			Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint.Get());

			FGraphNodeCreator<UK2Node_CustomEvent> EventCreator(*Graph);
			SendEvent = EventCreator.CreateNode();
			SendEvent->CustomFunctionName = SendEventName;
			EventCreator.Finalize();
		}

		/** Places a request node for a factory function, run by the send event. */
		UK2Node_TclPlatformRequest* AddRequestNode(UClass* Action, FName FactoryFunction) const
		{
			FGraphNodeCreator<UK2Node_TclPlatformRequest> Creator(*Graph);
			UK2Node_TclPlatformRequest* Node = Creator.CreateNode();
			Node->InitializeFromFunction(Action->FindFunctionByName(FactoryFunction));
			Creator.Finalize();
			Link(SendEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), Node->FindPinChecked(UEdGraphSchema_K2::PN_Execute));
			return Node;
		}

		/** Places a node returning a literal integer, to wire into a field. */
		UEdGraphPin* AddLiteralInt(int32 Value) const
		{
			FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
			UK2Node_CallFunction* Literal = Creator.CreateNode();
			Literal->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralInt), UKismetSystemLibrary::StaticClass());
			Creator.Finalize();
			GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Literal->FindPinChecked(TEXT("Value")), FString::FromInt(Value));
			return Literal->GetReturnValuePin();
		}

		/** Places a node returning a literal boolean, to wire into a field or a Send pin. */
		UEdGraphPin* AddLiteralBool(bool bValue) const
		{
			FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
			UK2Node_CallFunction* Literal = Creator.CreateNode();
			Literal->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralBool), UKismetSystemLibrary::StaticClass());
			Creator.Finalize();
			GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Literal->FindPinChecked(TEXT("Value")), bValue ? TEXT("true") : TEXT("false"));
			return Literal->GetReturnValuePin();
		}

		/** Connects two pins as dragging a wire between them does. */
		static bool Link(UEdGraphPin* From, UEdGraphPin* To)
		{
			return GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(From, To);
		}

		/** Compiles the Blueprint, returning the compiler's messages. */
		TSharedRef<FCompilerResultsLog> Compile() const
		{
			TSharedRef<FCompilerResultsLog> Results = MakeShared<FCompilerResultsLog>();
			Results->bSilentMode = true;
			FKismetEditorUtilities::CompileBlueprint(Blueprint.Get(), EBlueprintCompileOptions::SkipGarbageCollection, &Results.Get());
			return Results;
		}
	};

	/** The compiler's errors, one a line, for a failed test's message. */
	FString ErrorsOf(const FCompilerResultsLog& Results)
	{
		FString Errors;
		for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
		{
			if (Message->GetSeverity() == EMessageSeverity::Error)
			{
				Errors += Message->ToText().ToString() + TEXT("\n");
			}
		}
		return Errors;
	}

	/**
	 * Points the engine's generic async node at a factory function, as its menu
	 * entry does. Through reflection, because the members are protected and
	 * InitializeProxyFromFunction only arrived after UE 5.1.
	 */
	void PointAsyncActionAt(UK2Node_AsyncAction* Node, const UFunction* Function)
	{
		const UClass* Base = UK2Node_BaseAsyncTask::StaticClass();
		CastFieldChecked<FNameProperty>(Base->FindPropertyByName(TEXT("ProxyFactoryFunctionName")))->SetPropertyValue_InContainer(Node, Function->GetFName());
		CastFieldChecked<FObjectPropertyBase>(Base->FindPropertyByName(TEXT("ProxyFactoryClass")))->SetObjectPropertyValue_InContainer(Node, Function->GetOuterUClass());
		CastFieldChecked<FObjectPropertyBase>(Base->FindPropertyByName(TEXT("ProxyClass")))->SetObjectPropertyValue_InContainer(Node, CastFieldChecked<FObjectProperty>(Function->GetReturnProperty())->PropertyClass);
	}

	/** The pin for an UpdateChatSettings field. */
	UEdGraphPin* Field(const UK2Node_TclPlatformRequest* Node, const TCHAR* Name)
	{
		return Node->FindFieldPin(FName(Name));
	}
}

/**
 * A request node lays every field of the request out as a pin of its own, with
 * the struct the function takes hidden. An optional field starts not sent:
 * behind the collapse arrow, with its value unused. A required one is always sent.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeFieldsTest,
	"Tentacle.RequestNode.LaysTheFieldsOutOnTheNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeFieldsTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;
	const UK2Node_TclPlatformRequest* Settings = Test.AddRequestNode(UTclTwitch_UpdateChatSettings::StaticClass(), TEXT("UpdateChatSettings"));

	const UEdGraphPin* Options = Settings->FindPin(TEXT("Options"), EGPD_Input);
	if (TestNotNull(TEXT("the request struct's pin"), Options))
	{
		TestTrue(TEXT("is hidden"), Options->bHidden);
	}

	const UEdGraphPin* EmoteMode = Field(Settings, TEXT("bEmoteMode"));
	if (TestNotNull(TEXT("a field's pin"), EmoteMode))
	{
		TestEqual(TEXT("named for the field"), EmoteMode->PinFriendlyName.ToString(), FString(TEXT("Emote Mode")));
		TestEqual(TEXT("of the field's type"), EmoteMode->PinType.PinCategory, UEdGraphSchema_K2::PC_Boolean);
		TestTrue(TEXT("an optional field"), Settings->IsOptionalFieldPin(EmoteMode));
		TestFalse(TEXT("starts not sent"), Settings->IsFieldSent(EmoteMode));
		TestTrue(TEXT("behind the collapse arrow"), EmoteMode->bAdvancedView);
		TestTrue(TEXT("with its value unused"), EmoteMode->bDefaultValueIsIgnored);
	}
	TestNull(TEXT("no pin for a field's toggle"), Field(Settings, TEXT("bHasEmoteMode")));
	TestTrue(TEXT("the node collapses what it does not send"), Settings->AdvancedPinDisplay != ENodeAdvancedPins::NoPins);

	const UK2Node_TclPlatformRequest* Announcement = Test.AddRequestNode(UTclTwitch_SendChatAnnouncement::StaticClass(), TEXT("SendChatAnnouncement"));
	const UEdGraphPin* Message = Field(Announcement, TEXT("Message"));
	if (TestNotNull(TEXT("a required field's pin"), Message))
	{
		TestFalse(TEXT("is not optional"), Announcement->IsOptionalFieldPin(Message));
		TestTrue(TEXT("is always sent"), Announcement->IsFieldSent(Message));
		TestFalse(TEXT("and always shown"), Message->bAdvancedView);
		TestFalse(TEXT("with its value used"), Message->bDefaultValueIsIgnored);
	}

	// Ban User wraps its fields in Twitch's data object, which the node lays out
	// as the fields it holds.
	const UK2Node_TclPlatformRequest* Ban = Test.AddRequestNode(UTclTwitch_BanUser::StaticClass(), TEXT("BanUser"));
	TestNull(TEXT("no pin for a wrapping struct"), Ban->FindPin(TEXT("Options_Data"), EGPD_Input));
	const UEdGraphPin* UserId = Field(Ban, TEXT("Data.UserId"));
	if (TestNotNull(TEXT("a wrapped field's pin"), UserId))
	{
		TestEqual(TEXT("named for the field alone"), UserId->PinFriendlyName.ToString(), FString(TEXT("User Id")));
		TestTrue(TEXT("a required wrapped field is sent"), Ban->IsFieldSent(UserId));
	}
	const UEdGraphPin* Reason = Field(Ban, TEXT("Data.Reason"));
	if (TestNotNull(TEXT("an optional wrapped field's pin"), Reason))
	{
		TestTrue(TEXT("is optional"), Ban->IsOptionalFieldPin(Reason));
		TestFalse(TEXT("and starts not sent"), Ban->IsFieldSent(Reason));
	}
	TestNotNull(TEXT("beside the request's own fields"), Field(Ban, TEXT("BroadcasterId")));
	return true;
}

/**
 * A field set to Decide at Runtime gets a Send pin, true until told otherwise,
 * right after its own; any other setting takes the Send pin away, wire and all.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeRuntimeFieldTest,
	"Tentacle.RequestNode.DecidesFieldsAtRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeRuntimeFieldTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;
	UK2Node_TclPlatformRequest* Node = Test.AddRequestNode(UTclTwitch_UpdateChatSettings::StaticClass(), TEXT("UpdateChatSettings"));

	TestNull(TEXT("a field starts with no Send pin"), Node->FindSendPin(Field(Node, TEXT("bEmoteMode"))));
	Node->SetFieldState(Field(Node, TEXT("bEmoteMode")), ETclRequestFieldState::DecidedAtRuntime);
	const UEdGraphPin* EmoteMode = Field(Node, TEXT("bEmoteMode"));
	TestTrue(TEXT("it is decided at runtime"), Node->GetFieldState(EmoteMode) == ETclRequestFieldState::DecidedAtRuntime);
	TestFalse(TEXT("shown, with its value in use"), EmoteMode->bAdvancedView || EmoteMode->bDefaultValueIsIgnored);
	UEdGraphPin* Send = Node->FindSendPin(EmoteMode);
	if (!TestNotNull(TEXT("with a Send pin"), Send))
	{
		return false;
	}
	TestEqual(TEXT("named for the field"), Send->PinFriendlyName.ToString(), FString(TEXT("Send Emote Mode")));
	TestEqual(TEXT("a boolean"), Send->PinType.PinCategory, UEdGraphSchema_K2::PC_Boolean);
	TestEqual(TEXT("true by default"), Send->DefaultValue, FString(TEXT("true")));
	TestEqual(TEXT("right after the field"), Node->Pins.IndexOfByKey(Send), Node->Pins.IndexOfByKey(EmoteMode) + 1);

	TestTrue(TEXT("a Send pin takes a wire"), FRequestBlueprint::Link(Test.AddLiteralBool(false), Send));
	Node->SetFieldState(Field(Node, TEXT("bEmoteMode")), ETclRequestFieldState::Sent);
	TestTrue(TEXT("always sent"), Node->GetFieldState(Field(Node, TEXT("bEmoteMode"))) == ETclRequestFieldState::Sent);
	TestNull(TEXT("with no Send pin"), Node->FindSendPin(Field(Node, TEXT("bEmoteMode"))));
	TestFalse(TEXT("leaving no orphaned pin"), Node->Pins.ContainsByPredicate([](const UEdGraphPin* Pin) { return Pin->bOrphanedPin; }));

	Node->SetFieldState(Field(Node, TEXT("bEmoteMode")), ETclRequestFieldState::DecidedAtRuntime);
	Node->ReconstructNode();
	TestNotNull(TEXT("a Send pin stays when the node is rebuilt"), Node->FindSendPin(Field(Node, TEXT("bEmoteMode"))));
	Node->SetFieldState(Field(Node, TEXT("bEmoteMode")), ETclRequestFieldState::NotSent);
	TestNull(TEXT("and goes with Don't Send"), Node->FindSendPin(Field(Node, TEXT("bEmoteMode"))));
	return true;
}

/**
 * Sending an optional field brings its pin out from behind the collapse arrow
 * with its value in use, wiring a field sends it, and stopping a field leaves it
 * out again and breaks its wire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeSendFieldTest,
	"Tentacle.RequestNode.SetsAndUnsetsFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeSendFieldTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;
	UK2Node_TclPlatformRequest* Node = Test.AddRequestNode(UTclTwitch_UpdateChatSettings::StaticClass(), TEXT("UpdateChatSettings"));

	Node->SetFieldSent(Field(Node, TEXT("bEmoteMode")), true);
	const UEdGraphPin* EmoteMode = Field(Node, TEXT("bEmoteMode"));
	TestTrue(TEXT("a field set to send is sent"), Node->IsFieldSent(EmoteMode));
	TestFalse(TEXT("out from behind the collapse arrow"), EmoteMode->bAdvancedView);
	TestFalse(TEXT("with its value in use"), EmoteMode->bDefaultValueIsIgnored);

	UEdGraphPin* WaitTime = Field(Node, TEXT("SlowModeWaitTime"));
	TestTrue(TEXT("a wire connects"), FRequestBlueprint::Link(Test.AddLiteralInt(30), WaitTime));
	TestTrue(TEXT("and wiring a field sends it"), Node->IsFieldSent(WaitTime));

	// Changing a field rebuilds the node's pins, so a pin is found again after.
	Node->SetFieldSent(WaitTime, false);
	WaitTime = Field(Node, TEXT("SlowModeWaitTime"));
	TestFalse(TEXT("a field stopped is not sent"), Node->IsFieldSent(WaitTime));
	TestEqual(TEXT("and its wire is gone"), WaitTime->LinkedTo.Num(), 0);
	TestTrue(TEXT("and it is back behind the collapse arrow"), WaitTime->bAdvancedView);

	const UEdGraphPin* Message = Field(Test.AddRequestNode(UTclTwitch_SendChatAnnouncement::StaticClass(), TEXT("SendChatAnnouncement")), TEXT("Message"));
	UK2Node_TclPlatformRequest* Announcement = CastChecked<UK2Node_TclPlatformRequest>(Message->GetOwningNode());
	Announcement->SetFieldSent(Message, false);
	TestTrue(TEXT("a required field cannot be stopped"), Announcement->IsFieldSent(Message));

	Node->SetFieldSent(Field(Node, TEXT("bSubscriberMode")), true);
	Node->ReconstructNode();
	TestTrue(TEXT("a sent field stays sent when the node is rebuilt"), Node->IsFieldSent(Field(Node, TEXT("bSubscriberMode"))));
	TestFalse(TEXT("as does one not sent"), Node->IsFieldSent(Field(Node, TEXT("bFollowerMode"))));
	return true;
}

/**
 * A compiled request sends exactly the fields set on the node: a typed value, a
 * value equal to the field's default, and a wired one, but not a field whose
 * value was typed in and never sent, nor one that was sent and then stopped.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeSendsTest,
	"Tentacle.RequestNode.SendsExactlyTheFieldsSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeSendsTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;
	UK2Node_TclPlatformRequest* Node = Test.AddRequestNode(UTclTwitch_UpdateChatSettings::StaticClass(), TEXT("UpdateChatSettings"));
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	Node->SetFieldSent(Field(Node, TEXT("bEmoteMode")), true);
	Schema->TrySetDefaultValue(*Field(Node, TEXT("bEmoteMode")), TEXT("true"));

	// false is the field's default, and sending it turns follower mode off.
	Node->SetFieldSent(Field(Node, TEXT("bFollowerMode")), true);

	FRequestBlueprint::Link(Test.AddLiteralInt(30), Field(Node, TEXT("SlowModeWaitTime")));

	// Typed in, never sent.
	Field(Node, TEXT("bSubscriberMode"))->DefaultValue = TEXT("true");

	// Sent, then stopped.
	Node->SetFieldSent(Field(Node, TEXT("bUniqueChatMode")), true);
	Schema->TrySetDefaultValue(*Field(Node, TEXT("bUniqueChatMode")), TEXT("true"));
	Node->SetFieldSent(Field(Node, TEXT("bUniqueChatMode")), false);

	const TSharedRef<FCompilerResultsLog> Results = Test.Compile();
	if (!TestEqual(*FString::Printf(TEXT("the Blueprint compiles\n%s"), *ErrorsOf(*Results)), Results->NumErrors, 0))
	{
		return false;
	}

	const FRequestWorld World;
	World.Send(Test.Blueprint.Get());
	TestEqual(TEXT("one request"), World.Probe->Calls, 1);
	TestEqual(TEXT("to the endpoint's procedure"), World.Probe->Procedure, FString(TEXT("twitch.updateChatSettings")));
	TestEqual(TEXT("with exactly the fields set"), World.Probe->InputJson, FString(TEXT("{\"emote_mode\":true,\"follower_mode\":false,\"slow_mode_wait_time\":30}")));
	return true;
}

/**
 * A field decided at runtime is sent only when its Send pin is true as the
 * request is made: wired to false, it is left out; left at true, it is sent.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeSendsRuntimeTest,
	"Tentacle.RequestNode.SendsWhatItsSendPinsDecide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeSendsRuntimeTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;
	UK2Node_TclPlatformRequest* Node = Test.AddRequestNode(UTclTwitch_UpdateChatSettings::StaticClass(), TEXT("UpdateChatSettings"));
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	Node->SetFieldState(Field(Node, TEXT("bEmoteMode")), ETclRequestFieldState::DecidedAtRuntime);
	Schema->TrySetDefaultValue(*Field(Node, TEXT("bEmoteMode")), TEXT("true"));
	FRequestBlueprint::Link(Test.AddLiteralBool(false), Node->FindSendPin(Field(Node, TEXT("bEmoteMode"))));

	Node->SetFieldState(Field(Node, TEXT("bFollowerMode")), ETclRequestFieldState::DecidedAtRuntime);
	Schema->TrySetDefaultValue(*Field(Node, TEXT("bFollowerMode")), TEXT("true"));

	const TSharedRef<FCompilerResultsLog> Results = Test.Compile();
	if (!TestEqual(*FString::Printf(TEXT("the Blueprint compiles\n%s"), *ErrorsOf(*Results)), Results->NumErrors, 0))
	{
		return false;
	}

	const FRequestWorld World;
	World.Send(Test.Blueprint.Get());
	TestEqual(TEXT("only the field its Send pin sends"), World.Probe->InputJson, FString(TEXT("{\"follower_mode\":true}")));
	return true;
}

/**
 * A request that wraps its fields in Twitch's data object sends them inside it,
 * each as the node says: required, sent, or decided at runtime.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeWrappedTest,
	"Tentacle.RequestNode.SendsWrappedFieldsInTheirObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeWrappedTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;
	UK2Node_TclPlatformRequest* Node = Test.AddRequestNode(UTclTwitch_BanUser::StaticClass(), TEXT("BanUser"));
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	Schema->TrySetDefaultValue(*Field(Node, TEXT("Data.UserId")), TEXT("42"));
	Node->SetFieldSent(Field(Node, TEXT("Data.Reason")), true);
	Schema->TrySetDefaultValue(*Field(Node, TEXT("Data.Reason")), TEXT("spam"));
	Node->SetFieldState(Field(Node, TEXT("Data.Duration")), ETclRequestFieldState::DecidedAtRuntime);
	Schema->TrySetDefaultValue(*Field(Node, TEXT("Data.Duration")), TEXT("600"));
	FRequestBlueprint::Link(Test.AddLiteralBool(false), Node->FindSendPin(Field(Node, TEXT("Data.Duration"))));

	const TSharedRef<FCompilerResultsLog> Results = Test.Compile();
	if (!TestEqual(*FString::Printf(TEXT("the Blueprint compiles\n%s"), *ErrorsOf(*Results)), Results->NumErrors, 0))
	{
		return false;
	}

	const FRequestWorld World;
	World.Send(Test.Blueprint.Get());
	TestEqual(TEXT("to the endpoint's procedure"), World.Probe->Procedure, FString(TEXT("twitch.banUser")));
	TestEqual(TEXT("the wrapped fields set, inside their object"), World.Probe->InputJson, FString(TEXT("{\"data\":{\"user_id\":\"42\",\"reason\":\"spam\"}}")));
	return true;
}

/**
 * The request struct's own Make node, which C++ users and the struct-taking
 * function rely on, sends the optional fields whose pins it shows and no others:
 * the engine sets each shown field's toggle.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestStructMakeNodeTest,
	"Tentacle.RequestNode.StructMakeNodeSendsItsShownPins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestStructMakeNodeTest::RunTest(const FString& Parameters)
{
	FRequestBlueprint Test;

	FGraphNodeCreator<UK2Node_AsyncAction> ActionCreator(*Test.Graph);
	UK2Node_AsyncAction* Action = ActionCreator.CreateNode();
	PointAsyncActionAt(Action, UTclTwitch_UpdateChatSettings::StaticClass()->FindFunctionByName(TEXT("UpdateChatSettings")));
	ActionCreator.Finalize();
	FRequestBlueprint::Link(Test.SendEvent->FindPinChecked(UEdGraphSchema_K2::PN_Then), Action->FindPinChecked(UEdGraphSchema_K2::PN_Execute));

	FGraphNodeCreator<UK2Node_MakeStruct> MakeCreator(*Test.Graph);
	UK2Node_MakeStruct* Make = MakeCreator.CreateNode();
	Make->StructType = FTclTwitchApiUpdateChatSettingsInput::StaticStruct();
	Make->bMadeAfterOverridePinRemoval = true;
	FOptionalPinFromProperty& Shown = Make->ShowPinForProperties.AddDefaulted_GetRef();
	Shown.PropertyName = GET_MEMBER_NAME_CHECKED(FTclTwitchApiUpdateChatSettingsInput, SlowModeWaitTime);
	Shown.bShowPin = true;
	MakeCreator.Finalize();

	GetDefault<UEdGraphSchema_K2>()->TrySetDefaultValue(*Make->FindPinChecked(GET_MEMBER_NAME_CHECKED(FTclTwitchApiUpdateChatSettingsInput, SlowModeWaitTime)), TEXT("10"));
	FRequestBlueprint::Link(Make->FindPinChecked(FTclTwitchApiUpdateChatSettingsInput::StaticStruct()->GetFName(), EGPD_Output), Action->FindPinChecked(TEXT("Options")));

	const TSharedRef<FCompilerResultsLog> Results = Test.Compile();
	if (!TestEqual(*FString::Printf(TEXT("the Blueprint compiles\n%s"), *ErrorsOf(*Results)), Results->NumErrors, 0))
	{
		return false;
	}

	const FRequestWorld World;
	World.Send(Test.Blueprint.Get());
	TestEqual(TEXT("only the field the Make node shows"), World.Probe->InputJson, FString(TEXT("{\"slow_mode_wait_time\":10}")));
	return true;
}

/**
 * Every Twitch and Kick request uses the request node: its class carries
 * HasDedicatedAsyncNode, which keeps the engine's generic async node from
 * offering it a second time in the Blueprint menu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclRequestNodeEveryRequestTest,
	"Tentacle.RequestNode.EveryRequestUsesTheRequestNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclRequestNodeEveryRequestTest::RunTest(const FString& Parameters)
{
	TArray<UClass*> Actions;
	GetDerivedClasses(UTclPlatformRequestAction::StaticClass(), Actions);
	TestTrue(TEXT("every Twitch and Kick request has a node"), Actions.Num() > 100);
	for (const UClass* Action : Actions)
	{
		TestTrue(*FString::Printf(TEXT("%s has its dedicated node"), *Action->GetName()), Action->HasMetaData(TEXT("HasDedicatedAsyncNode")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
