// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/Button.h"
#include "UI/TclSignInController.h"
#include "TclSignInButton.generated.h"

// Unity's SignInButton component and Godot's TclSignInButton. A UButton rather
// than a UUserWidget, so it sits in the UMG palette and takes the ordinary
// Button style and any content, which is what lets it fit a game's own design.
/**
 * A button that signs the player in to one platform, with no look of its own:
 * style it like any Button and put what you like inside it.
 *
 * A click does what the button's state says: Connect signs the player in and
 * opens the sign-in page, Cancel abandons a sign-in in progress, and Disconnect
 * signs the account out. The first Text Block inside it reads the label, unless
 * Set Label is off, and the button is disabled while it cannot be pressed. On
 * Sign In State Changed hands you the whole state, the status line and
 * Twitch's code included, to show wherever you like.
 *
 * Only Tentacle embedded in the game signs players in; with the Tentacle app the
 * button stays disabled. For a button of your own, use Create Sign In
 * Controller instead.
 */
UCLASS(meta=(DisplayName="Tentacle Sign In Button"))
class TENTACLE_API UTclSignInButton : public UButton
{
	GENERATED_BODY()

public:
	/** The platform the button signs in to. Read when the button is built. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	ETclStreamPlatform Platform = ETclStreamPlatform::Twitch;

	/** Open the sign-in page in the player's browser when signing in needs it. Off, open the state's Url yourself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	bool bOpenBrowser = true;

	/** Keep the first Text Block inside the button reading the label: Connect, Cancel, Disconnect, or Sign in again. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	bool bSetLabel = true;

	/** Disable the button while it cannot be pressed: with the Tentacle app, and while Tentacle starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	bool bDisableWhenUnavailable = true;

	/** Fires once as the button starts, and whenever what it shows changes, the countdown while signing in included. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle")
	FTclOnSignInStateChanged OnSignInStateChanged;

	/** What the button shows now. */
	UFUNCTION(BlueprintPure, Category="Tentacle")
	FTclSignInState GetSignInState() const;

	/** The controller behind the button, for its Sign In, Cancel, Disconnect, and Open Page; null before the button is built. */
	UFUNCTION(BlueprintPure, Category="Tentacle")
	UTclSignInController* GetSignInController() const { return Controller; }

	/** The world the button is in, found through its widget tree or, outside one, its outer. */
	virtual UWorld* GetWorld() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleStateChanged(const FTclSignInState& State);

	UPROPERTY(Transient)
	TObjectPtr<UTclSignInController> Controller;
};
