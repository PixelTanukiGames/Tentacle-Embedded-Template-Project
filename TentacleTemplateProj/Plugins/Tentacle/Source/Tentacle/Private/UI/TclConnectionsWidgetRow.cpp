// © 2025 Eldritch One. All rights reserved.

#include "UI/TclConnectionsWidgetRow.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionAccount.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"

FTclConnectionsWidgetRow
FTclConnectionsWidgetRow::Describe(
	ETclStreamPlatform Platform,
	const UTclPlatformConnection* Connection,
	const UTclPlatformConnectionPrompt* Prompt,
	double SecondsLeft,
	const UTclPlatformConnectionResult* Result,
	bool bOpenBrowserAutomatically)
{
	FTclConnectionsWidgetRow Row;
	Row.Title = PlatformName(Platform);

	if (Prompt != nullptr && SecondsLeft > 0.0)
	{
		Row.Status = TEXT("Finish signing in in your browser");
		const FString Left = FormatTimeLeft(SecondsLeft);
		Row.Detail = Prompt->UserCode.IsEmpty()
			? FString::Printf(TEXT("Expires in %s"), *Left)
			: FString::Printf(TEXT("Code %s · expires in %s"), *Prompt->UserCode, *Left);
		Row.PrimaryAction = ETclConnectionsWidgetAction::OpenPage;
		Row.PrimaryLabel = bOpenBrowserAutomatically ? TEXT("Open the page again") : TEXT("Open the sign-in page");
		Row.SecondaryAction = ETclConnectionsWidgetAction::Cancel;
		Row.SecondaryLabel = TEXT("Cancel");
		return Row;
	}

	Row.Detail = ResultDetail(Result);
	if (Connection == nullptr)
	{
		Row.Status = TEXT("Waiting for Tentacle");
		return Row;
	}

	const UTclPlatformConnectionAccount* Account = Connection->Account;
	if (Account != nullptr && Connection->bNeedsReconnect)
	{
		Row.Status = FString::Printf(TEXT("%s needs to sign in again"), *Account->DisplayName);
		Row.PrimaryAction = ETclConnectionsWidgetAction::Connect;
		Row.PrimaryLabel = TEXT("Sign in again");
	}
	else if (Account != nullptr)
	{
		Row.Status = Connection->Status == ETclPlatformConnectionStatus::Connected
			? FString::Printf(TEXT("Connected as %s"), *Account->DisplayName)
			: FString::Printf(TEXT("Connecting as %s..."), *Account->DisplayName);
		Row.PrimaryAction = ETclConnectionsWidgetAction::Disconnect;
		Row.PrimaryLabel = TEXT("Disconnect");
		// It signed in before this version asked for more: it still works, and
		// signing in again grants the rest.
		if (Connection->bMissingPermissions)
		{
			if (Row.Detail.IsEmpty())
				Row.Detail = TEXT("Sign in again to allow newer features");
			Row.SecondaryAction = ETclConnectionsWidgetAction::Connect;
			Row.SecondaryLabel = TEXT("Sign in again");
		}
	}
	else
	{
		Row.Status = TEXT("Not connected");
		Row.PrimaryAction = ETclConnectionsWidgetAction::Connect;
		Row.PrimaryLabel = TEXT("Connect");
	}
	return Row;
}

FString
FTclConnectionsWidgetRow::PlatformName(ETclStreamPlatform Platform)
{
	return Platform == ETclStreamPlatform::Kick ? TEXT("Kick") : TEXT("Twitch");
}

FString
FTclConnectionsWidgetRow::FormatTimeLeft(double Seconds)
{
	const int32 Whole = FMath::CeilToInt(Seconds);
	return FString::Printf(TEXT("%d:%02d"), Whole / 60, Whole % 60);
}

FString
FTclConnectionsWidgetRow::ResultDetail(const UTclPlatformConnectionResult* Result)
{
	if (Result == nullptr)
		return FString();

	switch (Result->Outcome)
	{
	case ETclPlatformConnectionOutcome::Denied:
		return TEXT("Sign-in was declined");
	case ETclPlatformConnectionOutcome::Expired:
		return TEXT("Sign-in ran out of time");
	case ETclPlatformConnectionOutcome::Error:
		return Result->Message.IsEmpty() ? FString(TEXT("Sign-in failed")) : FString::Printf(TEXT("Sign-in failed: %s"), *Result->Message);
	default:
		return FString();
	}
}
