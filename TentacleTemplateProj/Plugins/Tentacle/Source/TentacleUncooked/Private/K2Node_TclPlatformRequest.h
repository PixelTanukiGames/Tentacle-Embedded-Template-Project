// © 2026 Eldritch One. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node_BaseAsyncTask.h"
#include "K2Node_TclPlatformRequest.generated.h"

class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
class SGraphNode;
class UGraphNodeContextMenuContext;
class UToolMenu;

/** Whether a request node sends an optional field. */
enum class ETclRequestFieldState : uint8
{
	/** Left out of the request. */
	NotSent,
	/** Always sent. */
	Sent,
	/** Sent when the field's Send pin is true, decided while the game runs. */
	DecidedAtRuntime,
};

/**
 * The Blueprint node for a Twitch or Kick API request (each subclass of
 * UTclPlatformRequestAction): every field of the request is a pin on the node
 * itself, so a request needs no Make node.
 *
 * Features:
 * - A required field is always sent. A field that only wraps others (Twitch's
 *   `data` object on Ban User) is laid out as the fields it holds.
 * - An optional field is not sent until it is set: its pin reads "Not sent"
 *   until the user clicks that or wires the pin, and the stop button beside it
 *   leaves it out again. Typing a value into a pin never sends it by itself, so a
 *   value equal to the default (false, 0, an empty string) can still be sent.
 * - An optional field can be set to Decide at Runtime, which gives it a Send pin
 *   whose value, when the request is made, says whether the field goes out.
 * - Collapsing the node hides only the fields that are not sent, so the
 *   collapsed node shows exactly the request.
 * - Compiles to the request's own function through Make nodes that show exactly
 *   the fields sent, whose struct toggles then say what goes out.
 */
UCLASS()
class UK2Node_TclPlatformRequest : public UK2Node_BaseAsyncTask
{
	GENERATED_BODY()

public:
	UK2Node_TclPlatformRequest();

	/** Points the node at a request's factory function, as its menu entry does. */
	void InitializeFromFunction(const UFunction* Function);

	/** The struct the request takes, or null when it takes none. */
	UScriptStruct* GetRequestStruct() const;

	/** Whether a pin is one of the request's fields. */
	bool IsFieldPin(const UEdGraphPin* Pin) const;

	/** Whether a pin is an optional field, which is sent only once it is set. */
	bool IsOptionalFieldPin(const UEdGraphPin* Pin) const;

	/** Whether a field pin is sent, always or as decided at runtime: a required one always is. */
	bool IsFieldSent(const UEdGraphPin* Pin) const;

	/** Whether an optional field pin is not sent, always sent, or decided at runtime; a required one is always sent. */
	ETclRequestFieldState GetFieldState(const UEdGraphPin* Pin) const;

	/**
	 * Sets whether an optional field is sent, as one undoable edit. Not sending
	 * it breaks its wire; deciding at runtime gives it a Send pin.
	 */
	void SetFieldState(const UEdGraphPin* Pin, ETclRequestFieldState State);

	/** Starts or stops always sending an optional field. */
	void SetFieldSent(const UEdGraphPin* Pin, bool bSent);

	/** The pin for one of the request's fields, by its member's name, or a path into a wrapping struct (`Data.Reason`). */
	UEdGraphPin* FindFieldPin(FName FieldPath) const;

	/** The Send pin of a field decided at runtime, or null. */
	UEdGraphPin* FindSendPin(const UEdGraphPin* FieldPin) const;

	//~ UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;

	//~ UK2Node
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

private:
	/** The factory function's request parameter, or null. */
	FStructProperty* GetRequestParameter() const;

	/** The struct members from the request down to the field a pin stands for; empty for any other pin. */
	TArray<const FProperty*> GetFieldPath(const UEdGraphPin* Pin) const;

	/** A field's state by its path, as stored on the node. */
	ETclRequestFieldState GetFieldStateAt(const TArray<const FProperty*>& Path) const;

	/** Adds the pins for a struct's fields, and for the fields of any struct that only wraps others. */
	void AddFieldPins(const UScriptStruct* Struct, const uint8* Defaults, TArray<const FProperty*>& Path, const TMap<FString, FText>& FieldNames);

	/** Sets a field pin's flags from whether it is sent. */
	void ApplyFieldState(UEdGraphPin* Pin) const;

	/** Shows the node's collapse arrow while any field is not sent, and hides it otherwise. */
	void UpdateAdvancedPinDisplay();

	/**
	 * Builds one struct of the request in the compiled graph: a Make node showing
	 * the fields sent, the structs it wraps built the same way, and each field
	 * decided at runtime given its flag. Returns the pin carrying the struct.
	 */
	UEdGraphPin* BuildStruct(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, const UScriptStruct* Struct, TArray<const FProperty*>& Path);

	/** The optional fields the node always sends, by path (`bEmoteMode`, `Data.Reason`). */
	UPROPERTY()
	TArray<FName> SentFields;

	/** The optional fields whose Send pin decides, by path. */
	UPROPERTY()
	TArray<FName> RuntimeFields;
};
