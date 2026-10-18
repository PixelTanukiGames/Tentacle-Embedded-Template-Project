// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UObject/Object.h"
#include "UI/TclSignInState.h"
#include "TclSignInController.generated.h"

class UTentacle_Connections;
class UTclPlatformConnection;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTclOnSignInStateChanged, const FTclSignInState&, State);

// Godot's TclSignInController and tentacle-web's SignInController; Unity's
// SignInButton component is its own controller. The Tentacle Sign In Button
// runs one of these.
/**
 * The logic of a sign-in button for one platform, with no look of its own, for
 * a button of your own design: call Press from its On Clicked, and show the
 * label and status On State Changed hands you.
 *
 * Pressing does what the label says: Connect signs the player in and opens the
 * sign-in page, Cancel abandons a sign-in in progress, Disconnect signs the
 * account out, and Sign in again signs the player in again. It only opens the
 * page for a sign-in it started. Keep it in a variable for as long as the
 * button is on screen.
 */
UCLASS(BlueprintType)
class TENTACLE_API UTclSignInController : public UObject
{
	GENERATED_BODY()

public:
	UTclSignInController();

	/**
	 * A sign-in controller for Platform, which opens the sign-in page in the
	 * player's browser unless Open Browser is off. Its first state is worked out
	 * before it returns, so read Get State once after binding On State Changed.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections", meta=(WorldContext="WorldContextObject", DisplayName="Create Sign In Controller"))
	static UTclSignInController* CreateSignInController(UObject* WorldContextObject, ETclStreamPlatform Platform, bool bOpenBrowser = true);

	/** Fires whenever what the button should show changes, the countdown while signing in included. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Connections")
	FTclOnSignInStateChanged OnStateChanged;

	/** What the button should show now. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Connections")
	FTclSignInState GetState() const { return State; }

	/** Does what the label says: sign in, cancel, or disconnect. Does nothing while the state cannot be pressed. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void Press();

	/** Signs the player in, whatever the stage: for a second "Sign in again" button. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void SignIn();

	/** Abandons the sign-in in progress. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void Cancel();

	/** Signs the platform's account out. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void Disconnect();

	/** Opens the sign-in page in progress again, or for the first time when Open Browser is off. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections")
	void OpenPage();

	/** Starts following Platform from the Tentacle actor in WorldContextObject's world, replacing what it followed before. */
	void Start(const UObject* WorldContextObject, ETclStreamPlatform InPlatform, bool bInOpenBrowser);

	/** Stops following, until it is started again. */
	void Stop();

	/** Works out the state again, and fires On State Changed when it changed. */
	void Refresh();

	/** How a page is opened. Opens the player's browser unless replaced. */
	TFunction<void(const FString& Url)> OpenUrl;

	/** The clock a prompt's time left is measured on, in seconds. */
	TFunction<double()> Clock;

	virtual void BeginDestroy() override;

private:
	UFUNCTION()
	void HandleConnectionChanged(const UTclPlatformConnection* Connection);

	UFUNCTION()
	void HandleConnectPrompt(const UTclPlatformConnectionPrompt* Prompt);

	UFUNCTION()
	void HandleConnectResult(const UTclPlatformConnectionResult* Result);

	/** Keeps the countdown moving while a sign-in is in progress. */
	void UpdateTicker();

	UPROPERTY(Transient)
	TObjectPtr<UTentacle_Connections> Connections;

	/** The sign-in in progress, or null. Held here so it outlives the message that carried it. */
	UPROPERTY(Transient)
	TObjectPtr<UTclPlatformConnectionPrompt> Prompt;

	/** How the last sign-in ended, or null. */
	UPROPERTY(Transient)
	TObjectPtr<UTclPlatformConnectionResult> Result;

	FTclSignInState State;
	ETclStreamPlatform Platform = ETclStreamPlatform::Twitch;
	FString UnavailableNote;
	double PromptDeadline = 0.0;
	bool bOpenBrowser = true;
	bool bStartedHere = false;
	bool bHasState = false;
	FTSTicker::FDelegateHandle TickerHandle;
};
