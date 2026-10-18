// © 2025 Eldritch One. All rights reserved.

#include "UI/TclConnectionsWidget.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"
#include "Styling/CoreStyle.h"
#include "UI/TclConnectionsWidgetRowView.h"
#include "Utilities/TclCommon.h"

void
UTclConnectionsWidgetRowView::HandlePrimaryClicked()
{
	if (Owner != nullptr)
		Owner->Press(this, true);
}

void
UTclConnectionsWidgetRowView::HandleSecondaryClicked()
{
	if (Owner != nullptr)
		Owner->Press(this, false);
}

UTclConnectionsWidget::UTclConnectionsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Platforms = { ETclStreamPlatform::Twitch, ETclStreamPlatform::Kick };
	OpenUrl = [](const FString& Url) { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); };
	Clock = []() { return FPlatformTime::Seconds(); };
}

TSharedRef<SWidget>
UTclConnectionsWidget::RebuildWidget()
{
	// A C++ widget has an empty tree until it builds one; a Widget Blueprint
	// deriving from this one may have built its own, which is left alone.
	if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
		BuildTree();
	return Super::RebuildWidget();
}

void
UTclConnectionsWidget::BuildTree()
{
	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Rows"));
	WidgetTree->RootWidget = Root;

	Note = MakeText(TEXT("Note"), MutedTextColor, FontSize);
	Note->SetVisibility(ESlateVisibility::Collapsed);
	Root->AddChildToVerticalBox(Note);

	for (const ETclStreamPlatform Platform : Platforms)
	{
		const FString Name = FTclConnectionsWidgetRow::PlatformName(Platform);
		UTclConnectionsWidgetRowView* Row = NewObject<UTclConnectionsWidgetRowView>(this);
		Row->Owner = this;
		Row->Platform = Platform;

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name + TEXT("Line")));

		UTextBlock* Title = MakeText(Name + TEXT("Title"), TextColor, FontSize, true);
		Title->SetText(FText::FromString(Name));
		Title->SetMinDesiredWidth(FontSize * 4.f);
		UHorizontalBoxSlot* TitleSlot = Line->AddChildToHorizontalBox(Title);
		TitleSlot->SetVerticalAlignment(VAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));

		// Wrapped, so a long status takes another line rather than running
		// under the buttons beside it.
		Row->Status = MakeText(Name + TEXT("Status"), TextColor, FontSize);
		Row->Status->SetAutoWrapText(true);
		UHorizontalBoxSlot* StatusSlot = Line->AddChildToHorizontalBox(Row->Status);
		StatusSlot->SetVerticalAlignment(VAlign_Center);
		StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		for (const bool bPrimary : { true, false })
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *(Name + (bPrimary ? TEXT("Primary") : TEXT("Secondary"))));
			Button->SetBackgroundColor(ButtonColor);
			UTextBlock* Label = MakeText(Name + (bPrimary ? TEXT("PrimaryLabel") : TEXT("SecondaryLabel")), ButtonTextColor, FontSize);
			Button->AddChild(Label);
			UHorizontalBoxSlot* ButtonSlot = Line->AddChildToHorizontalBox(Button);
			ButtonSlot->SetVerticalAlignment(VAlign_Center);
			ButtonSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
			if (bPrimary)
			{
				Row->Primary = Button;
				Row->PrimaryLabel = Label;
				Button->OnClicked.AddDynamic(Row, &UTclConnectionsWidgetRowView::HandlePrimaryClicked);
			}
			else
			{
				Row->Secondary = Button;
				Row->SecondaryLabel = Label;
				Button->OnClicked.AddDynamic(Row, &UTclConnectionsWidgetRowView::HandleSecondaryClicked);
			}
		}

		UVerticalBoxSlot* LineSlot = Root->AddChildToVerticalBox(Line);
		LineSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));

		Row->Detail = MakeText(Name + TEXT("Detail"), MutedTextColor, FMath::Max(1, FontSize - 3));
		Row->Detail->SetAutoWrapText(true);
		Root->AddChildToVerticalBox(Row->Detail);
		Rows.Add(Row);
	}
}

UTextBlock*
UTclConnectionsWidget::MakeText(const FString& Name, const FSlateColor& Color, int32 Size, bool bBold)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? TEXT("Bold") : TEXT("Regular"), Size));
	Text->SetColorAndOpacity(Color);
	return Text;
}

void
UTclConnectionsWidget::ShowNote(const FString& Text)
{
	Note->SetText(FText::FromString(Text));
	Note->SetVisibility(ESlateVisibility::HitTestInvisible);
	for (UTclConnectionsWidgetRowView* Row : Rows)
	{
		Row->Status->GetParent()->SetVisibility(ESlateVisibility::Collapsed);
		Row->Detail->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void
UTclConnectionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ATentacle* Tentacle = UTclCommon::GetActorOfClass<ATentacle>(this);
	if (Tentacle == nullptr)
	{
		ShowNote(TEXT("Add a Tentacle actor to the level to sign in here"));
		return;
	}
	if (!Tentacle->IsEmbedded())
	{
		ShowNote(TEXT("Twitch and Kick connect through the Tentacle app"));
		return;
	}

	Connections = Tentacle->Connections;
	Connections->OnConnectionChanged.AddUniqueDynamic(this, &UTclConnectionsWidget::HandleConnectionChanged);
	Connections->OnConnectPrompt.AddUniqueDynamic(this, &UTclConnectionsWidget::HandleConnectPrompt);
	Connections->OnConnectResult.AddUniqueDynamic(this, &UTclConnectionsWidget::HandleConnectResult);
	for (UTclConnectionsWidgetRowView* Row : Rows)
		Refresh(Row);
}

void
UTclConnectionsWidget::NativeDestruct()
{
	if (Connections != nullptr)
	{
		Connections->OnConnectionChanged.RemoveDynamic(this, &UTclConnectionsWidget::HandleConnectionChanged);
		Connections->OnConnectPrompt.RemoveDynamic(this, &UTclConnectionsWidget::HandleConnectPrompt);
		Connections->OnConnectResult.RemoveDynamic(this, &UTclConnectionsWidget::HandleConnectResult);
		Connections = nullptr;
	}
	Super::NativeDestruct();
}

void
UTclConnectionsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The countdown, while a row is signing in.
	for (UTclConnectionsWidgetRowView* Row : Rows)
	{
		if (Row->Prompt != nullptr)
			Refresh(Row);
	}
}

void
UTclConnectionsWidget::Refresh(UTclConnectionsWidgetRowView* Row)
{
	double SecondsLeft = 0.0;
	if (Row->Prompt != nullptr)
	{
		SecondsLeft = Row->PromptDeadline - Clock();
		// Ran out here; the core's result says so shortly.
		if (SecondsLeft <= 0.0)
			Row->Prompt = nullptr;
	}

	bool bFound = false;
	const UTclPlatformConnection* Connection = Connections != nullptr ? Connections->GetConnection(Row->Platform, bFound) : nullptr;
	const FTclConnectionsWidgetRow Described = FTclConnectionsWidgetRow::Describe(
		Row->Platform, Connection, Row->Prompt, SecondsLeft, Row->Result, bOpenBrowserAutomatically);

	Row->Status->SetText(FText::FromString(Described.Status));
	Row->Detail->SetText(FText::FromString(Described.Detail));
	Row->Detail->SetVisibility(Described.Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	Row->PrimaryAction = Described.PrimaryAction;
	Row->PrimaryLabel->SetText(FText::FromString(Described.PrimaryLabel));
	Row->Primary->SetVisibility(Described.PrimaryAction == ETclConnectionsWidgetAction::None ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	Row->SecondaryAction = Described.SecondaryAction;
	Row->SecondaryLabel->SetText(FText::FromString(Described.SecondaryLabel));
	Row->Secondary->SetVisibility(Described.SecondaryAction == ETclConnectionsWidgetAction::None ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

UTclConnectionsWidgetRowView*
UTclConnectionsWidget::RowFor(ETclStreamPlatform Platform) const
{
	for (UTclConnectionsWidgetRowView* Row : Rows)
	{
		if (Row->Platform == Platform)
			return Row;
	}
	return nullptr;
}

void
UTclConnectionsWidget::Press(UTclConnectionsWidgetRowView* Row, bool bPrimary)
{
	if (Connections == nullptr || Row == nullptr)
		return;

	switch (bPrimary ? Row->PrimaryAction : Row->SecondaryAction)
	{
	case ETclConnectionsWidgetAction::Connect:
		Row->Result = nullptr;
		Connections->Connect(Row->Platform);
		break;
	case ETclConnectionsWidgetAction::Disconnect:
		Connections->Disconnect(Row->Platform);
		break;
	case ETclConnectionsWidgetAction::OpenPage:
		if (Row->Prompt != nullptr)
			OpenUrl(Row->Prompt->Url);
		break;
	case ETclConnectionsWidgetAction::Cancel:
		Connections->CancelConnect(Row->Platform);
		break;
	default:
		break;
	}
	Refresh(Row);
}

void
UTclConnectionsWidget::HandleConnectionChanged(const UTclPlatformConnection* Connection)
{
	if (Connection == nullptr)
		return;
	if (UTclConnectionsWidgetRowView* Row = RowFor(Connection->Platform))
		Refresh(Row);
}

void
UTclConnectionsWidget::HandleConnectPrompt(const UTclPlatformConnectionPrompt* Prompt)
{
	UTclConnectionsWidgetRowView* Row = Prompt != nullptr ? RowFor(Prompt->Platform) : nullptr;
	if (Row == nullptr)
		return;

	Row->Prompt = const_cast<UTclPlatformConnectionPrompt*>(Prompt);
	Row->PromptDeadline = Clock() + Prompt->ExpiresInSeconds;
	Row->Result = nullptr;
	if (bOpenBrowserAutomatically)
		OpenUrl(Prompt->Url);
	Refresh(Row);
}

void
UTclConnectionsWidget::HandleConnectResult(const UTclPlatformConnectionResult* Result)
{
	UTclConnectionsWidgetRowView* Row = Result != nullptr ? RowFor(Result->Platform) : nullptr;
	if (Row == nullptr)
		return;

	Row->Prompt = nullptr;
	Row->Result = const_cast<UTclPlatformConnectionResult*>(Result);
	Refresh(Row);
}
