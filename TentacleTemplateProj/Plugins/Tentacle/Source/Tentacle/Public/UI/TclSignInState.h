// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Sdk/TclStreamPlatform.h"
#include "TclSignInState.generated.h"

class UTclPlatformConnection;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;

/** Where a sign-in button's platform stands, which decides what pressing it does. */
UENUM(BlueprintType)
enum class ETclSignInStage : uint8
{
	/** Signing in is not available here: Tentacle runs in the Tentacle app, or the level has no Tentacle actor. */
	Unavailable,
	/** Tentacle is starting and has not said how the platform stands yet. */
	Waiting,
	/** No account is signed in. Pressing signs the player in. */
	NotConnected,
	/** The player is signing in in their browser. Pressing cancels. */
	SigningIn,
	/** An account is signed in and the platform is connecting. Pressing disconnects. */
	Connecting,
	/** An account is signed in and connected. Pressing disconnects. */
	Connected,
	/** The account has to sign in again before it can connect. Pressing signs the player in. */
	NeedsReconnect,
};

// Unity's SignInState and Godot's TclSignInState, with the same rules, and the
// Connections widget's words (FTclConnectionsWidgetRow) for the status and
// detail, so a player reads the same sign-in on every engine.
/**
 * What a sign-in button shows for one platform: what its label says, a status
 * line and a detail line to show beside it, and whether it can be pressed.
 */
USTRUCT(BlueprintType)
struct TENTACLE_API FTclSignInState
{
	GENERATED_BODY()

	/** The platform this is about. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	ETclStreamPlatform Platform = ETclStreamPlatform::Twitch;

	/** Where the platform stands, which decides what pressing does. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	ETclSignInStage Stage = ETclSignInStage::Waiting;

	/** What the button says: Connect, Cancel, Disconnect, or Sign in again. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString Label;

	/** The platform's name: Twitch or Kick. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString Title;

	/** Where the platform stands, in a few words, such as "Connected as Streamer". */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString Status;

	/**
	 * A second line, or empty: Twitch's code and the time left while signing in,
	 * why the last sign-in failed, or that signing in again allows newer features.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString Detail;

	/** While signing in to Twitch, the code to show the player in case the page asks for it. Empty otherwise. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString UserCode;

	/** While signing in, the sign-in page. Empty otherwise. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString Url;

	/** The signed-in account's display name, or empty when none is signed in. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	FString AccountName;

	/** Whether pressing the button does anything: false while Unavailable or Waiting. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	bool bCanPress = false;

	/**
	 * True when the account signed in before an update asked the platform for
	 * more access: it keeps working, and signing in again turns on what needs it.
	 */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Connections")
	bool bMissingPermissions = false;

	/**
	 * The state for Platform, given why signing in is unavailable here (empty
	 * when it is available), its Connection (null before Tentacle has reported
	 * one), the sign-in Prompt in progress (null for none) with SecondsLeft on
	 * it, and the Result of the last sign-in (null for none).
	 */
	static FTclSignInState Describe(
		ETclStreamPlatform Platform,
		const FString& UnavailableNote,
		const UTclPlatformConnection* Connection,
		const UTclPlatformConnectionPrompt* Prompt,
		double SecondsLeft,
		const UTclPlatformConnectionResult* Result);

	bool operator==(const FTclSignInState& Other) const;
	bool operator!=(const FTclSignInState& Other) const { return !(*this == Other); }
};
