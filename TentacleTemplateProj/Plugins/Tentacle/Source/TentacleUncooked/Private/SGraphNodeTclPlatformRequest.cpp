// © 2026 Eldritch One. All rights reserved.

#include "SGraphNodeTclPlatformRequest.h"

#include "GraphEditor.h"
#include "GraphEditorSettings.h"
#include "K2Node_TclPlatformRequest.h"
#include "SGraphPanel.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SGraphNodeTclPlatformRequest"

void SGraphNodeTclPlatformRequest::Construct(const FArguments& InArgs, UK2Node_TclPlatformRequest* InNode)
{
	SGraphNodeK2Default::Construct(SGraphNodeK2Default::FArguments(), InNode);
}

void SGraphNodeTclPlatformRequest::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	const UK2Node_TclPlatformRequest* Node = Cast<UK2Node_TclPlatformRequest>(GraphNode);
	const UEdGraphPin* Pin = PinToAdd->GetPinObj();
	if (Node == nullptr || Pin == nullptr || !Node->IsOptionalFieldPin(Pin))
	{
		SGraphNodeK2Default::AddPin(PinToAdd);
		return;
	}

	// As SGraphNode::AddPin lays out an input pin, with the field's button beside it.
	PinToAdd->SetOwner(SharedThis(this));
	const TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			PinToAdd
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(2.f, 0.f, 0.f, 0.f)
		[
			MakeFieldButton(Pin->PinName)
		];
	if (Pin->bAdvancedView)
	{
		Row->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}
	LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(GetDefault<UGraphEditorSettings>()->GetInputPinPadding())
		[
			Row
		];
	InputPins.Add(PinToAdd);
}

#if !UE_VERSION_OLDER_THAN(5, 4, 0)
void SGraphNodeTclPlatformRequest::CreateAdvancedViewArrow(TSharedPtr<SVerticalBox> MainBox)
{
	// As SGraphNode::CreateAdvancedViewArrow, with the count beside the arrow.
	const bool bHidePins = OwnerGraphPanelPtr.IsValid() && OwnerGraphPanelPtr.Pin()->GetPinVisibility() != SGraphEditor::Pin_Show;
	if (bHidePins || GraphNode == nullptr || !MainBox.IsValid())
	{
		return;
	}
	MainBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.Padding(3.f, 0.f, 3.f, 3.f)
		[
			SNew(SCheckBox)
			.Visibility(this, &SGraphNodeTclPlatformRequest::AdvancedViewArrowVisibility)
			.OnCheckStateChanged(this, &SGraphNodeTclPlatformRequest::OnAdvancedViewChanged)
			.IsChecked(this, &SGraphNodeTclPlatformRequest::IsAdvancedViewChecked)
			.Cursor(EMouseCursor::Default)
			.Style(FAppStyle::Get(), "Graph.Node.AdvancedView")
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SImage)
						.Image(this, &SGraphNodeTclPlatformRequest::GetAdvancedViewArrow)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(4.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(this, &SGraphNodeTclPlatformRequest::GetCollapsedSummary)
						.Font(FCoreStyle::GetDefaultFontStyle("Italic", 8))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		];
}
#endif

FText SGraphNodeTclPlatformRequest::GetCollapsedSummary() const
{
	if (GraphNode == nullptr || GraphNode->AdvancedPinDisplay != ENodeAdvancedPins::Hidden)
	{
		return FText::GetEmpty();
	}
	int32 NotSent = 0;
	for (const UEdGraphPin* Pin : GraphNode->Pins)
	{
		NotSent += Pin->bAdvancedView && !Pin->bHidden ? 1 : 0;
	}
	return FText::Format(LOCTEXT("CollapsedSummary", "{0} more, not sent"), FText::AsNumber(NotSent));
}

TSharedRef<SWidget> SGraphNodeTclPlatformRequest::MakeFieldButton(FName PinName)
{
	const TWeakObjectPtr<UK2Node_TclPlatformRequest> WeakNode(Cast<UK2Node_TclPlatformRequest>(GraphNode));
	const auto IsSent = [WeakNode, PinName]()
	{
		const UK2Node_TclPlatformRequest* Node = WeakNode.Get();
		const UEdGraphPin* Pin = Node ? Node->FindPin(PinName, EGPD_Input) : nullptr;
		return Pin != nullptr && Node->IsFieldSent(Pin);
	};

	return SNew(SButton)
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		// A graph node shows the move cursor everywhere, which a button should not.
		.Cursor(EMouseCursor::Default)
		.ContentPadding(FMargin(1.f, 0.f))
		.VAlign(VAlign_Center)
		.ToolTipText_Lambda([IsSent]()
		{
			return IsSent()
				? LOCTEXT("StopSendingTooltip", "Stop sending this field")
				: LOCTEXT("SendTooltip", "Send this field with the request");
		})
		.OnClicked_Lambda([WeakNode, PinName, IsSent]()
		{
			if (UK2Node_TclPlatformRequest* Node = WeakNode.Get())
			{
				Node->SetFieldSent(Node->FindPin(PinName, EGPD_Input), !IsSent());
			}
			return FReply::Handled();
		})
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([IsSent]() { return IsSent() ? 1 : 0; })
			+ SWidgetSwitcher::Slot()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.35f))
				.Padding(FMargin(6.f, 1.f))
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NotSent", "Not sent"))
					.Font(FCoreStyle::GetDefaultFontStyle("Italic", 9))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
			+ SWidgetSwitcher::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.X"))
				.DesiredSizeOverride(FVector2D(12.f, 12.f))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

#undef LOCTEXT_NAMESPACE
