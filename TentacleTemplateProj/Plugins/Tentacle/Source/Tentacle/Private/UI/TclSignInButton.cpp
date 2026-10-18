// © 2025 Eldritch One. All rights reserved.

#include "UI/TclSignInButton.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

namespace
{
	/** The first Text Block in Widget or anywhere inside it, depth first, or null. */
	UTextBlock* FirstTextBlock(UWidget* Widget)
	{
		if (Widget == nullptr)
			return nullptr;
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
			return Text;
		if (const UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 Index = 0; Index < Panel->GetChildrenCount(); ++Index)
			{
				if (UTextBlock* Found = FirstTextBlock(Panel->GetChildAt(Index)))
					return Found;
			}
		}
		return nullptr;
	}
}

TSharedRef<SWidget>
UTclSignInButton::RebuildWidget()
{
	TSharedRef<SWidget> Built = Super::RebuildWidget();
	// In the designer it is only a button; it signs in once the game runs.
	if (!IsDesignTime())
	{
		OnClicked.AddUniqueDynamic(this, &UTclSignInButton::HandleClicked);
		if (Controller == nullptr)
			Controller = NewObject<UTclSignInController>(this);
		Controller->OnStateChanged.AddUniqueDynamic(this, &UTclSignInButton::HandleStateChanged);
		Controller->Start(this, Platform, bOpenBrowser);
	}
	return Built;
}

void
UTclSignInButton::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	if (Controller != nullptr)
		Controller->Stop();
}

#if WITH_EDITOR
const FText
UTclSignInButton::GetPaletteCategory()
{
	return NSLOCTEXT("Tentacle", "TentaclePaletteCategory", "Tentacle");
}
#endif

UWorld*
UTclSignInButton::GetWorld() const
{
	if (UWorld* World = Super::GetWorld())
		return World;
	// UE 5.1's widget finds a world only through the widget tree it sits in, so a
	// button made with NewObject outside one never found the Tentacle actor;
	// later engines go on to the outer, as here.
	const UObject* Outer = GetOuter();
	return Outer != nullptr && !HasAnyFlags(RF_ClassDefaultObject) ? Outer->GetWorld() : nullptr;
}

FTclSignInState
UTclSignInButton::GetSignInState() const
{
	return Controller != nullptr ? Controller->GetState() : FTclSignInState();
}

void
UTclSignInButton::HandleClicked()
{
	if (Controller != nullptr)
		Controller->Press();
}

void
UTclSignInButton::HandleStateChanged(const FTclSignInState& State)
{
	if (bSetLabel)
	{
		if (UTextBlock* Label = FirstTextBlock(GetContent()))
			Label->SetText(FText::FromString(State.Label));
	}
	if (bDisableWhenUnavailable)
		SetIsEnabled(State.bCanPress);
	OnSignInStateChanged.Broadcast(State);
}
