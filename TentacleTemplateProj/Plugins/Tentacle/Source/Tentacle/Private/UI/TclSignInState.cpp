// © 2025 Eldritch One. All rights reserved.

#include "UI/TclSignInState.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionAccount.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "UI/TclConnectionsWidgetRow.h"

FTclSignInState
FTclSignInState::Describe(
	ETclStreamPlatform Platform,
	const FString& UnavailableNote,
	const UTclPlatformConnection* Connection,
	const UTclPlatformConnectionPrompt* Prompt,
	double SecondsLeft,
	const UTclPlatformConnectionResult* Result)
{
	FTclSignInState State;
	State.Platform = Platform;
	State.Title = FTclConnectionsWidgetRow::PlatformName(Platform);
	State.Label = TEXT("Connect");

	if (!UnavailableNote.IsEmpty())
	{
		State.Stage = ETclSignInStage::Unavailable;
		State.Status = UnavailableNote;
		return State;
	}

	// The widget's words for where the platform stands; a button has one action
	// per stage where a row has two buttons.
	const FTclConnectionsWidgetRow Row = FTclConnectionsWidgetRow::Describe(Platform, Connection, Prompt, SecondsLeft, Result, true);
	State.Status = Row.Status;
	State.Detail = Row.Detail;

	if (Prompt != nullptr && SecondsLeft > 0.0)
	{
		State.Stage = ETclSignInStage::SigningIn;
		State.Label = TEXT("Cancel");
		State.UserCode = Prompt->UserCode;
		State.Url = Prompt->Url;
		State.bCanPress = true;
		return State;
	}

	if (Connection == nullptr)
	{
		State.Stage = ETclSignInStage::Waiting;
		return State;
	}

	State.bCanPress = true;
	const UTclPlatformConnectionAccount* Account = Connection->Account;
	if (Account != nullptr)
		State.AccountName = Account->DisplayName;

	if (Account != nullptr && Connection->bNeedsReconnect)
	{
		State.Stage = ETclSignInStage::NeedsReconnect;
		State.Label = TEXT("Sign in again");
	}
	else if (Account != nullptr)
	{
		State.Stage = Connection->Status == ETclPlatformConnectionStatus::Connected ? ETclSignInStage::Connected : ETclSignInStage::Connecting;
		State.Label = TEXT("Disconnect");
		State.bMissingPermissions = Connection->bMissingPermissions;
	}
	else
	{
		State.Stage = ETclSignInStage::NotConnected;
	}
	return State;
}

bool
FTclSignInState::operator==(const FTclSignInState& Other) const
{
	return Platform == Other.Platform
		&& Stage == Other.Stage
		&& Label == Other.Label
		&& Title == Other.Title
		&& Status == Other.Status
		&& Detail == Other.Detail
		&& UserCode == Other.UserCode
		&& Url == Other.Url
		&& AccountName == Other.AccountName
		&& bCanPress == Other.bCanPress
		&& bMissingPermissions == Other.bMissingPermissions;
}
