// © 2025 Eldritch One. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Sdk/TclStreamPlatform.h"
#include "TclSignInAction.generated.h"

class UTentacle_Connections;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;

// One signature for all three pins, because an async node's data pins come
// from its first delegate's signature: Url and User Code are On Prompt's,
// Message is On Failed's.
/**
 * Called as a Sign In node moves on. Url is the sign-in page and User Code the
 * code Twitch's page asks for, on On Prompt. Message says why signing in did
 * not connect, on On Failed.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTclSignInEvent, const FString&, Url, const FString&, UserCode, const FString&, Message);

/**
 * Signs the player in to Twitch or Kick in one node, as the Connections
 * widget's button does: it asks Tentacle to connect the platform, opens the
 * sign-in page in the player's browser, and ends at On Connected or On Failed.
 * On Prompt fires as the page opens, with the code to show for Twitch.
 *
 * Only Tentacle embedded in the game signs players in; with the Tentacle app,
 * the node fails at once. Signing in to the same platform again ends this node
 * as cancelled. For finer control, use the Tentacle Connections component's
 * Connect and its events, which this is built on.
 */
UCLASS()
class TENTACLE_API UTclSignInAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Fires when the sign-in page is ready, and opened unless Open Browser is off: show User Code for Twitch. */
	UPROPERTY(BlueprintAssignable)
	FTclSignInEvent OnPrompt;

	/** Fires when the player is signed in and the platform connects. */
	UPROPERTY(BlueprintAssignable)
	FTclSignInEvent OnConnected;

	/** Fires when signing in ends without connecting, with a Message saying why. */
	UPROPERTY(BlueprintAssignable)
	FTclSignInEvent OnFailed;

	/**
	 * Sign the player in to Platform: Tentacle connects it, and the sign-in page
	 * opens in the player's browser unless Open Browser is off.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Connections", meta=(BlueprintInternalUseOnly="true", WorldContext="WorldContextObject", DisplayName="Sign In"))
	static UTclSignInAction* SignIn(UObject* WorldContextObject, ETclStreamPlatform Platform, bool bOpenBrowser = true);

	virtual void Activate() override;

	/** Opens a sign-in page in the player's browser; a test replaces it. */
	TFunction<void(const FString& Url)> OpenUrl;

private:
	UFUNCTION()
	void HandlePrompt(const UTclPlatformConnectionPrompt* Prompt);

	UFUNCTION()
	void HandleResult(const UTclPlatformConnectionResult* Result);

	/** Fires On Failed with Message and lets the node go. */
	void Fail(const FString& Message);

	/** Stops listening and lets the node go. */
	void Finish();

	TWeakObjectPtr<UObject> WorldContext;
	TWeakObjectPtr<UTentacle_Connections> Connections;
	ETclStreamPlatform Platform = ETclStreamPlatform::Twitch;
	bool bOpenBrowser = true;
	bool bFinished = false;
};
