// © 2026 Eldritch One. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "KismetNodes/SGraphNodeK2Default.h"
#include "Misc/EngineVersionComparison.h"

class UK2Node_TclPlatformRequest;

/**
 * The request node's widget: the engine's own node, with a button beside every
 * optional field. A field that is not sent shows "Not sent" in place of its
 * value, and clicking it sends the field; a field that is sent shows its value
 * and a stop button that leaves it out again. While collapsed, the arrow says
 * how many fields it hides.
 */
class SGraphNodeTclPlatformRequest : public SGraphNodeK2Default
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeTclPlatformRequest) {}
	SLATE_END_ARGS()

	/** Builds the widget for a request node. */
	void Construct(const FArguments& InArgs, UK2Node_TclPlatformRequest* InNode);

protected:
	//~ SGraphNode
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
#if !UE_VERSION_OLDER_THAN(5, 4, 0)
	// Before UE 5.4 the arrow cannot be replaced, and shows no count.
	virtual void CreateAdvancedViewArrow(TSharedPtr<SVerticalBox> MainBox) override;
#endif

private:
	/** The button beside an optional field's pin, which sends it or stops sending it. */
	TSharedRef<SWidget> MakeFieldButton(FName PinName);

	/** What the collapsed arrow says: how many fields it hides, none of them sent. */
	FText GetCollapsedSummary() const;
};
