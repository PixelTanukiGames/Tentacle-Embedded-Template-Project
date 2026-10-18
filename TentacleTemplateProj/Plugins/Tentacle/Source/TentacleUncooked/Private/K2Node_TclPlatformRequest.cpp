// © 2026 Eldritch One. All rights reserved.

#include "K2Node_TclPlatformRequest.h"

#include "Base/TclPlatformRequestAction.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Misc/EngineVersionComparison.h"
#include "ObjectTools.h"
#include "SGraphNodeTclPlatformRequest.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "UObject/StructOnScope.h"

#define LOCTEXT_NAMESPACE "K2Node_TclPlatformRequest"

namespace
{
	/** The metadata a request struct puts on each optional field's `bHas*` toggle. */
	const FName InlineEditConditionToggle(TEXT("InlineEditConditionToggle"));

	/** The metadata naming an optional field's toggle, which only an optional field carries. */
	const FName EditCondition(TEXT("EditCondition"));

	/** What a Send pin's name starts with, before its field pin's name. */
	const FString SendPinPrefix(TEXT("Send_"));

	/** Whether a request struct member is a field, and not one of the toggles saying which fields are sent. */
	bool IsFieldProperty(const FProperty* Property)
	{
		return Property->HasAnyPropertyFlags(CPF_BlueprintVisible) && !Property->HasMetaData(InlineEditConditionToggle);
	}

	/** Whether a field is optional, and so sent only once its toggle is set. */
	bool IsOptionalProperty(const FProperty* Property)
	{
		return Property->HasMetaData(EditCondition);
	}

	/**
	 * Whether a field is a required struct, which the node lays out as the fields
	 * it holds: Twitch's `data` object, which some requests wrap their fields in.
	 */
	bool IsWrappingStruct(const FProperty* Property)
	{
		return CastField<FStructProperty>(Property) != nullptr && IsFieldProperty(Property) && !IsOptionalProperty(Property);
	}

	/** A field's path as the node stores it: the members' names, joined with dots (`Data.Reason`). */
	FName PathKey(const TArray<const FProperty*>& Path)
	{
		TArray<FString> Names;
		for (const FProperty* Property : Path)
		{
			Names.Add(Property->GetName());
		}
		return FName(*FString::Join(Names, TEXT(".")));
	}

	/** A field's pin name: the request parameter's and the members', as a split pin is named (`Options_Data_Reason`). */
	FName PathPinName(const FStructProperty& Request, const TArray<const FProperty*>& Path)
	{
		FString Name = Request.GetName();
		for (const FProperty* Property : Path)
		{
			Name += TEXT("_") + Property->GetName();
		}
		return FName(*Name);
	}

	/** The name of a field's Send pin. */
	FName SendPinName(FName FieldPinName)
	{
		return FName(*(SendPinPrefix + FieldPinName.ToString()));
	}

	/** Every field a struct lays out, with the fields of the structs it wraps in their place. */
	void CollectFields(const UScriptStruct* Struct, TArray<const FProperty*>& Path, TArray<TArray<const FProperty*>>& Out)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			if (!IsFieldProperty(*It))
			{
				continue;
			}
			Path.Push(*It);
			if (IsWrappingStruct(*It))
			{
				CollectFields(CastField<FStructProperty>(*It)->Struct, Path, Out);
			}
			else
			{
				Out.Add(Path);
			}
			Path.Pop();
		}
	}
}

UK2Node_TclPlatformRequest::UK2Node_TclPlatformRequest()
{
	ProxyActivateFunctionName = GET_FUNCTION_NAME_CHECKED(UBlueprintAsyncActionBase, Activate);
}

void UK2Node_TclPlatformRequest::InitializeFromFunction(const UFunction* Function)
{
	const FObjectProperty* ReturnProperty = Function ? CastField<FObjectProperty>(Function->GetReturnProperty()) : nullptr;
	if (ReturnProperty == nullptr)
	{
		return;
	}
	ProxyFactoryFunctionName = Function->GetFName();
	ProxyFactoryClass = Function->GetOuterUClass();
	ProxyClass = ReturnProperty->PropertyClass;
}

FStructProperty* UK2Node_TclPlatformRequest::GetRequestParameter() const
{
	const UFunction* Function = GetFactoryFunction();
	if (Function == nullptr)
	{
		return nullptr;
	}
	for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		FStructProperty* Parameter = CastField<FStructProperty>(*It);
		const bool bIsInput = !It->HasAnyPropertyFlags(CPF_OutParm) || It->HasAnyPropertyFlags(CPF_ReferenceParm);
		if (Parameter != nullptr && bIsInput && !It->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			return Parameter;
		}
	}
	return nullptr;
}

UScriptStruct* UK2Node_TclPlatformRequest::GetRequestStruct() const
{
	const FStructProperty* Request = GetRequestParameter();
	return Request ? Request->Struct : nullptr;
}

TArray<const FProperty*> UK2Node_TclPlatformRequest::GetFieldPath(const UEdGraphPin* Pin) const
{
	TArray<const FProperty*> Path;
	if (Pin == nullptr || Pin->Direction != EGPD_Input || Pin->ParentPin != nullptr || Pin->GetOwningNodeUnchecked() != this)
	{
		return Path;
	}
	const FStructProperty* Request = GetRequestParameter();
	if (Request == nullptr)
	{
		return Path;
	}
	const FString Prefix = Request->GetName() + TEXT("_");
	const FString PinName = Pin->PinName.ToString();
	if (!PinName.StartsWith(Prefix, ESearchCase::CaseSensitive))
	{
		return Path;
	}

	// A request's members are named without underscores, so they split the path.
	TArray<FString> Names;
	PinName.RightChop(Prefix.Len()).ParseIntoArray(Names, TEXT("_"));
	const UStruct* Struct = Request->Struct;
	for (int32 Index = 0; Index < Names.Num(); ++Index)
	{
		const FProperty* Field = Struct->FindPropertyByName(FName(*Names[Index]));
		const bool bLast = Index == Names.Num() - 1;
		if (Field == nullptr || !IsFieldProperty(Field) || IsWrappingStruct(Field) == bLast)
		{
			return TArray<const FProperty*>();
		}
		Path.Add(Field);
		if (!bLast)
		{
			Struct = CastField<FStructProperty>(Field)->Struct;
		}
	}
	return Path;
}

bool UK2Node_TclPlatformRequest::IsFieldPin(const UEdGraphPin* Pin) const
{
	return GetFieldPath(Pin).Num() > 0;
}

bool UK2Node_TclPlatformRequest::IsOptionalFieldPin(const UEdGraphPin* Pin) const
{
	const TArray<const FProperty*> Path = GetFieldPath(Pin);
	return Path.Num() > 0 && IsOptionalProperty(Path.Last());
}

ETclRequestFieldState UK2Node_TclPlatformRequest::GetFieldStateAt(const TArray<const FProperty*>& Path) const
{
	if (!IsOptionalProperty(Path.Last()))
	{
		return ETclRequestFieldState::Sent;
	}
	const FName Key = PathKey(Path);
	if (RuntimeFields.Contains(Key))
	{
		return ETclRequestFieldState::DecidedAtRuntime;
	}
	return SentFields.Contains(Key) ? ETclRequestFieldState::Sent : ETclRequestFieldState::NotSent;
}

ETclRequestFieldState UK2Node_TclPlatformRequest::GetFieldState(const UEdGraphPin* Pin) const
{
	const TArray<const FProperty*> Path = GetFieldPath(Pin);
	return Path.Num() > 0 ? GetFieldStateAt(Path) : ETclRequestFieldState::NotSent;
}

bool UK2Node_TclPlatformRequest::IsFieldSent(const UEdGraphPin* Pin) const
{
	return IsFieldPin(Pin) && GetFieldState(Pin) != ETclRequestFieldState::NotSent;
}

UEdGraphPin* UK2Node_TclPlatformRequest::FindFieldPin(FName FieldPath) const
{
	const FStructProperty* Request = GetRequestParameter();
	if (Request == nullptr)
	{
		return nullptr;
	}
	TArray<FString> Names;
	FieldPath.ToString().ParseIntoArray(Names, TEXT("."));
	return FindPin(FName(*(Request->GetName() + TEXT("_") + FString::Join(Names, TEXT("_")))), EGPD_Input);
}

UEdGraphPin* UK2Node_TclPlatformRequest::FindSendPin(const UEdGraphPin* FieldPin) const
{
	return IsFieldPin(FieldPin) ? FindPin(SendPinName(FieldPin->PinName), EGPD_Input) : nullptr;
}

void UK2Node_TclPlatformRequest::ApplyFieldState(UEdGraphPin* Pin) const
{
	const bool bNotSent = IsOptionalFieldPin(Pin) && GetFieldState(Pin) == ETclRequestFieldState::NotSent;
	// A field that is not sent waits behind the collapse arrow, and its value is
	// not used, which also hides its value editor. A container's value is never
	// typed in, as on a Make node.
	Pin->bAdvancedView = bNotSent;
	Pin->bDefaultValueIsIgnored = bNotSent || Pin->PinType.IsContainer();
}

void UK2Node_TclPlatformRequest::UpdateAdvancedPinDisplay()
{
	const bool bAnyNotSent = Pins.ContainsByPredicate([](const UEdGraphPin* Pin)
	{
		return Pin->bAdvancedView && !Pin->bHidden;
	});
	if (!bAnyNotSent)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::NoPins;
	}
	else if (AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}
}

void UK2Node_TclPlatformRequest::AddFieldPins(const UScriptStruct* Struct, const uint8* Defaults, TArray<const FProperty*>& Path, const TMap<FString, FText>& FieldNames)
{
	const FStructProperty* Request = GetRequestParameter();
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Field = *It;
		if (!IsFieldProperty(Field))
		{
			continue;
		}
		Path.Push(Field);
		if (IsWrappingStruct(Field))
		{
			AddFieldPins(CastField<FStructProperty>(Field)->Struct, Field->ContainerPtrToValuePtr<uint8>(Defaults), Path, FieldNames);
			Path.Pop();
			continue;
		}

		UEdGraphPin* Pin = CreatePin(EGPD_Input, NAME_None, PathPinName(*Request, Path));
		if (!Schema->ConvertPropertyToPinType(Field, Pin->PinType))
		{
			RemovePin(Pin);
			Path.Pop();
			continue;
		}
		const FText* Name = FieldNames.Find(PathKey(Path).ToString());
		Pin->PinFriendlyName = Name ? *Name : Field->GetDisplayNameText();

		FString DefaultValue;
		if (FBlueprintEditorUtils::PropertyValueToString(Field, Defaults, DefaultValue)
			&& Schema->IsPinDefaultValid(Pin, DefaultValue, nullptr, FText::GetEmpty()).IsEmpty())
		{
			Schema->SetPinAutogeneratedDefaultValue(Pin, DefaultValue);
		}
		else
		{
			Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
		}
		ApplyFieldState(Pin);

		if (GetFieldStateAt(Path) == ETclRequestFieldState::DecidedAtRuntime)
		{
			UEdGraphPin* Send = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, SendPinName(Pin->PinName));
			Send->PinFriendlyName = FText::Format(LOCTEXT("SendPinName", "Send {0}"), Pin->PinFriendlyName);
			Schema->SetPinAutogeneratedDefaultValue(Send, TEXT("true"));
		}
		Path.Pop();
	}
}

void UK2Node_TclPlatformRequest::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const FStructProperty* Request = GetRequestParameter();
	UEdGraphPin* RequestPin = Request ? FindPin(Request->GetFName(), EGPD_Input) : nullptr;
	if (RequestPin != nullptr)
	{
		// The request's fields stand in for its struct, which the compiled
		// Blueprint still passes to the function.
		RequestPin->bHidden = true;

		// Each field is named as its member is; one a wrapped struct holds that
		// shares its name with another field takes the wrapper's name in front.
		TArray<const FProperty*> Path;
		TArray<TArray<const FProperty*>> Fields;
		CollectFields(Request->Struct, Path, Fields);
		TMap<FString, int32> Uses;
		for (const TArray<const FProperty*>& Field : Fields)
		{
			++Uses.FindOrAdd(Field.Last()->GetDisplayNameText().ToString());
		}
		TMap<FString, FText> FieldNames;
		for (const TArray<const FProperty*>& Field : Fields)
		{
			const FText Name = Field.Last()->GetDisplayNameText();
			FieldNames.Add(PathKey(Field).ToString(), Field.Num() > 1 && Uses[Name.ToString()] > 1
				? FText::Format(LOCTEXT("WrappedFieldName", "{0} {1}"), Field[Field.Num() - 2]->GetDisplayNameText(), Name)
				: Name);
		}

		FStructOnScope Defaults(Request->Struct);
		AddFieldPins(Request->Struct, Defaults.GetStructMemory(), Path, FieldNames);
	}
	UpdateAdvancedPinDisplay();
}

FText UK2Node_TclPlatformRequest::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
#if !UE_VERSION_OLDER_THAN(5, 5, 0)
	// From UE 5.5 an async node is titled with its function's own name
	// (UpdateChatSettings). A function call node, the Blueprint menu, and this
	// node before 5.5 use the friendly name (Update Chat Settings) while the
	// editor's Show Friendly Names setting is on, as it is by default.
	if (const UFunction* Function = GetFactoryFunction())
	{
		return ObjectTools::GetUserFacingFunctionName(Function, true);
	}
#endif
	return Super::GetNodeTitle(TitleType);
}

void UK2Node_TclPlatformRequest::SetFieldState(const UEdGraphPin* Pin, ETclRequestFieldState State)
{
	UEdGraphPin* FieldPin = Pin ? FindPin(Pin->PinName, EGPD_Input) : nullptr;
	const TArray<const FProperty*> Path = GetFieldPath(FieldPin);
	if (Path.Num() == 0 || !IsOptionalProperty(Path.Last()) || GetFieldStateAt(Path) == State)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetFieldStateTransaction", "Change Request Field"));
	Modify();
	const FName Key = PathKey(Path);
	SentFields.Remove(Key);
	RuntimeFields.Remove(Key);
	if (State == ETclRequestFieldState::Sent)
	{
		SentFields.Add(Key);
	}
	else if (State == ETclRequestFieldState::DecidedAtRuntime)
	{
		RuntimeFields.Add(Key);
	}

	// The value is kept for when the field is sent again, as the Details panel
	// keeps a property's value behind its unticked edit condition, but a wire
	// would send it, so the wire goes. So does a Send pin's, with its pin.
	if (State == ETclRequestFieldState::NotSent)
	{
		FieldPin->Modify();
		FieldPin->BreakAllPinLinks(true);
	}
	if (UEdGraphPin* Send = State != ETclRequestFieldState::DecidedAtRuntime ? FindSendPin(FieldPin) : nullptr)
	{
		Send->Modify();
		Send->BreakAllPinLinks(true);
	}

	// A Send pin may come or go; wires and values carry over by pin name.
	ReconstructNode();

	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
	if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}
}

void UK2Node_TclPlatformRequest::SetFieldSent(const UEdGraphPin* Pin, bool bSent)
{
	SetFieldState(Pin, bSent ? ETclRequestFieldState::Sent : ETclRequestFieldState::NotSent);
}

void UK2Node_TclPlatformRequest::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	// Wiring a field sends it: a wired value always reaches the request.
	if (Pin != nullptr && Pin->LinkedTo.Num() > 0 && IsOptionalFieldPin(Pin) && GetFieldState(Pin) == ETclRequestFieldState::NotSent)
	{
		Modify();
		SentFields.AddUnique(PathKey(GetFieldPath(Pin)));
		ApplyFieldState(Pin);
		UpdateAdvancedPinDisplay();
		if (UEdGraph* Graph = GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
	}
}

void UK2Node_TclPlatformRequest::GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const
{
	const TArray<const FProperty*> Path = GetFieldPath(&Pin);
	if (Path.Num() > 0)
	{
		GetSchema()->ConstructBasicPinTooltip(Pin, Path.Last()->GetToolTipText(), HoverTextOut);
		if (IsOptionalProperty(Path.Last()))
		{
			HoverTextOut += TEXT("\n\n");
			switch (GetFieldStateAt(Path))
			{
			case ETclRequestFieldState::NotSent:
				HoverTextOut += LOCTEXT("FieldNotSentTooltip", "Not sent. Click Not sent or wire the pin to send it.").ToString();
				break;
			case ETclRequestFieldState::Sent:
				HoverTextOut += LOCTEXT("FieldSentTooltip", "Sent with the request.").ToString();
				break;
			case ETclRequestFieldState::DecidedAtRuntime:
				HoverTextOut += LOCTEXT("FieldRuntimeTooltip", "Sent when its Send pin is true.").ToString();
				break;
			}
		}
		return;
	}

	const FString PinName = Pin.PinName.ToString();
	const UEdGraphPin* FieldPin = PinName.StartsWith(SendPinPrefix, ESearchCase::CaseSensitive)
		? FindPin(FName(*PinName.RightChop(SendPinPrefix.Len())), EGPD_Input)
		: nullptr;
	if (IsFieldPin(FieldPin))
	{
		GetSchema()->ConstructBasicPinTooltip(
			Pin,
			FText::Format(LOCTEXT("SendPinTooltip", "Whether {0} is sent, decided as the request is made."), FieldPin->GetDisplayName()),
			HoverTextOut);
		return;
	}
	Super::GetPinHoverText(Pin, HoverTextOut);
}

void UK2Node_TclPlatformRequest::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	if (Menu == nullptr || Context == nullptr || !IsOptionalFieldPin(Context->Pin))
	{
		return;
	}

	const ETclRequestFieldState Current = GetFieldState(Context->Pin);
	const FName PinName = Context->Pin->PinName;
	UK2Node_TclPlatformRequest* Node = const_cast<UK2Node_TclPlatformRequest*>(this);
	FToolMenuSection& Section = Menu->AddSection(TEXT("TclRequestField"), LOCTEXT("RequestFieldSection", "Request Field"));
	const auto AddState = [&Section, Node, PinName, Current](FName EntryName, const FText& Label, const FText& Tooltip, ETclRequestFieldState State)
	{
		Section.AddMenuEntry(
			EntryName,
			Label,
			Tooltip,
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateWeakLambda(Node, [Node, PinName, State]()
				{
					Node->SetFieldState(Node->FindPin(PinName, EGPD_Input), State);
				}),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([Current, State]() { return Current == State; })),
			EUserInterfaceActionType::RadioButton);
	};
	AddState(TEXT("TclAlwaysSendField"), LOCTEXT("AlwaysSendMenu", "Always Send"), LOCTEXT("AlwaysSendMenuTooltip", "Send this field with every request."), ETclRequestFieldState::Sent);
	AddState(TEXT("TclDontSendField"), LOCTEXT("DontSendMenu", "Don't Send"), LOCTEXT("DontSendMenuTooltip", "Leave this field out of the request."), ETclRequestFieldState::NotSent);
	AddState(
		TEXT("TclDecideFieldAtRuntime"),
		LOCTEXT("DecideAtRuntimeMenu", "Decide at Runtime"),
		LOCTEXT("DecideAtRuntimeMenuTooltip", "Add a Send pin that decides, as the request is made, whether this field is sent."),
		ETclRequestFieldState::DecidedAtRuntime);
}

TSharedPtr<SGraphNode> UK2Node_TclPlatformRequest::CreateVisualWidget()
{
	return SNew(SGraphNodeTclPlatformRequest, this);
}

UEdGraphPin* UK2Node_TclPlatformRequest::BuildStruct(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const UScriptStruct* Struct, TArray<const FProperty*>& Path)
{
	const FStructProperty* Request = GetRequestParameter();
	TArray<const FProperty*> Fields;
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		if (IsFieldProperty(*It))
		{
			Fields.Add(*It);
		}
	}

	// A Make node showing exactly the fields sent: it sets the toggle of every
	// optional field it shows, and no other, which is what the request sends.
	UK2Node_MakeStruct* Make = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
	Make->StructType = const_cast<UScriptStruct*>(Struct);
	Make->bMadeAfterOverridePinRemoval = true;
	for (const FProperty* Field : Fields)
	{
		Path.Push(Field);
		FOptionalPinFromProperty& Entry = Make->ShowPinForProperties.AddDefaulted_GetRef();
		Entry.PropertyName = Field->GetFName();
		Entry.bShowPin = IsWrappingStruct(Field) || GetFieldStateAt(Path) != ETclRequestFieldState::NotSent;
		Path.Pop();
	}
	Make->AllocateDefaultPins();

	TArray<TPair<const FProperty*, UEdGraphPin*>> DecidedAtRuntime;
	for (const FProperty* Field : Fields)
	{
		Path.Push(Field);
		UEdGraphPin* MakePin = Make->FindPin(Field->GetFName(), EGPD_Input);
		if (IsWrappingStruct(Field))
		{
			UEdGraphPin* Wrapped = BuildStruct(CompilerContext, SourceGraph, CastField<FStructProperty>(Field)->Struct, Path);
			if (MakePin != nullptr)
			{
				Wrapped->MakeLinkTo(MakePin);
			}
		}
		else if (const ETclRequestFieldState State = GetFieldStateAt(Path); State != ETclRequestFieldState::NotSent)
		{
			UEdGraphPin* FieldPin = FindPin(PathPinName(*Request, Path), EGPD_Input);
			if (FieldPin == nullptr || MakePin == nullptr)
			{
				CompilerContext.MessageLog.Error(
					*FText::Format(LOCTEXT("FieldMissingError", "@@ cannot send {0}: the request no longer has that field."), FText::FromName(PathKey(Path))).ToString(),
					this);
			}
			else
			{
				CompilerContext.MovePinLinksToIntermediate(*FieldPin, *MakePin);
				if (State == ETclRequestFieldState::DecidedAtRuntime)
				{
					DecidedAtRuntime.Emplace(Field, FindSendPin(FieldPin));
				}
			}
		}
		Path.Pop();
	}

	// A field decided at runtime is on the Make node, so its toggle is set; its
	// Send pin then sets the toggle again, as the request is made.
	UEdGraphPin* Value = Make->FindPinChecked(Struct->GetFName(), EGPD_Output);
	for (const TPair<const FProperty*, UEdGraphPin*>& Field : DecidedAtRuntime)
	{
		UK2Node_CallFunction* SetSent = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		SetSent->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UTclPlatformRequestAction, WithFieldSent), UTclPlatformRequestAction::StaticClass());
		SetSent->AllocateDefaultPins();
		UEdGraphPin* In = SetSent->FindPinChecked(TEXT("Request"), EGPD_Input);
		UEdGraphPin* Out = SetSent->FindPinChecked(TEXT("Result"), EGPD_Output);
		for (UEdGraphPin* Pin : { In, Out })
		{
			Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			Pin->PinType.PinSubCategoryObject = const_cast<UScriptStruct*>(Struct);
		}
		Value->MakeLinkTo(In);
		SetSent->FindPinChecked(TEXT("Toggle"), EGPD_Input)->DefaultValue = Field.Key->GetMetaData(EditCondition);
		if (UEdGraphPin* Send = Field.Value)
		{
			CompilerContext.MovePinLinksToIntermediate(*Send, *SetSent->FindPinChecked(TEXT("bSent"), EGPD_Input));
		}
		Value = Out;
	}
	return Value;
}

void UK2Node_TclPlatformRequest::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	const FStructProperty* Request = GetRequestParameter();
	UEdGraphPin* RequestPin = Request ? FindPin(Request->GetFName(), EGPD_Input) : nullptr;
	if (RequestPin != nullptr)
	{
		// A struct field split into its members goes back together first.
		ExpandSplitPins(CompilerContext, SourceGraph);

		TArray<const FProperty*> Path;
		BuildStruct(CompilerContext, SourceGraph, Request->Struct, Path)->MakeLinkTo(RequestPin);

		// The fields are on the Make nodes now. The base expansion hands every
		// remaining input pin to the factory function's parameter of that name,
		// which the fields and Send pins are not.
		TArray<UEdGraphPin*> FieldPins;
		for (UEdGraphPin* Pin : Pins)
		{
			if (IsFieldPin(Pin) || Pin->PinName.ToString().StartsWith(SendPinPrefix, ESearchCase::CaseSensitive))
			{
				FieldPins.Add(Pin);
			}
		}
		for (UEdGraphPin* Pin : FieldPins)
		{
			Pin->BreakAllPinLinks();
			RemovePin(Pin);
		}
	}

	Super::ExpandNode(CompilerContext, SourceGraph);
}

void UK2Node_TclPlatformRequest::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* NodeClass = GetClass();
	ActionRegistrar.RegisterClassFactoryActions<UTclPlatformRequestAction>(FBlueprintActionDatabaseRegistrar::FMakeFuncSpawnerDelegate::CreateLambda(
		[NodeClass](const UFunction* FactoryFunction) -> UBlueprintNodeSpawner*
		{
			UBlueprintNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(FactoryFunction);
			check(NodeSpawner != nullptr);
			NodeSpawner->NodeClass = NodeClass;

			const TWeakObjectPtr<UFunction> Function = MakeWeakObjectPtr(const_cast<UFunction*>(FactoryFunction));
			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda([Function](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/)
			{
				CastChecked<UK2Node_TclPlatformRequest>(NewNode)->InitializeFromFunction(Function.Get());
			});
			return NodeSpawner;
		}));
}

#undef LOCTEXT_NAMESPACE
