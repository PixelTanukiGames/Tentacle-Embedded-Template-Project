// © 2025 Eldritch One. All rights reserved.

#include "UI/TclSignInController.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Sdk/TclPlatformConnection.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"
#include "UObject/Package.h"
#include "Utilities/TclCommon.h"

UTclSignInController::UTclSignInController()
{
	OpenUrl = [](const FString& Url) { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); };
	Clock = []() { return FPlatformTime::Seconds(); };
}

UTclSignInController*
UTclSignInController::CreateSignInController(UObject* WorldContextObject, ETclStreamPlatform Platform, bool bOpenBrowser)
{
	UTclSignInController* Controller = NewObject<UTclSignInController>(WorldContextObject != nullptr ? WorldContextObject : GetTransientPackage());
	Controller->Start(WorldContextObject, Platform, bOpenBrowser);
	return Controller;
}

void
UTclSignInController::Start(const UObject* WorldContextObject, ETclStreamPlatform InPlatform, bool bInOpenBrowser)
{
	Stop();
	Platform = InPlatform;
	bOpenBrowser = bInOpenBrowser;

	ATentacle* Tentacle = WorldContextObject != nullptr ? UTclCommon::GetActorOfClass<ATentacle>(WorldContextObject) : nullptr;
	if (Tentacle == nullptr)
	{
		UnavailableNote = TEXT("Add a Tentacle actor to the level to sign in here");
	}
	else if (!Tentacle->IsEmbedded())
	{
		UnavailableNote = TEXT("Twitch and Kick connect through the Tentacle app");
	}
	else
	{
		UnavailableNote.Reset();
		Connections = Tentacle->Connections;
		Connections->OnConnectionChanged.AddUniqueDynamic(this, &UTclSignInController::HandleConnectionChanged);
		Connections->OnConnectPrompt.AddUniqueDynamic(this, &UTclSignInController::HandleConnectPrompt);
		Connections->OnConnectResult.AddUniqueDynamic(this, &UTclSignInController::HandleConnectResult);
	}
	Refresh();
}

void
UTclSignInController::Stop()
{
	if (Connections != nullptr)
	{
		Connections->OnConnectionChanged.RemoveDynamic(this, &UTclSignInController::HandleConnectionChanged);
		Connections->OnConnectPrompt.RemoveDynamic(this, &UTclSignInController::HandleConnectPrompt);
		Connections->OnConnectResult.RemoveDynamic(this, &UTclSignInController::HandleConnectResult);
		Connections = nullptr;
	}
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	// A sign-in that ends while nothing is listening would otherwise read as
	// still in progress when it starts again.
	Prompt = nullptr;
	Result = nullptr;
	bStartedHere = false;
	bHasState = false;
}

void
UTclSignInController::BeginDestroy()
{
	Stop();
	Super::BeginDestroy();
}

void
UTclSignInController::Refresh()
{
	double SecondsLeft = 0.0;
	if (Prompt != nullptr)
	{
		SecondsLeft = PromptDeadline - Clock();
		// Ran out here; the core's result says so shortly.
		if (SecondsLeft <= 0.0)
			Prompt = nullptr;
	}

	bool bFound = false;
	const UTclPlatformConnection* Connection = Connections != nullptr ? Connections->GetConnection(Platform, bFound) : nullptr;
	const FTclSignInState Next = FTclSignInState::Describe(Platform, UnavailableNote, Connection, Prompt, SecondsLeft, Result);
	UpdateTicker();

	if (bHasState && Next == State)
		return;
	State = Next;
	bHasState = true;
	OnStateChanged.Broadcast(State);
}

void
UTclSignInController::UpdateTicker()
{
	if (Prompt == nullptr || TickerHandle.IsValid())
		return;
	// Removed by returning false once the sign-in is over, rather than from
	// inside its own tick.
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float)
		{
			Refresh();
			if (Prompt != nullptr)
				return true;
			TickerHandle.Reset();
			return false;
		}),
		0.25f);
}

void
UTclSignInController::Press()
{
	switch (State.Stage)
	{
	case ETclSignInStage::NotConnected:
	case ETclSignInStage::NeedsReconnect:
		SignIn();
		break;
	case ETclSignInStage::SigningIn:
		Cancel();
		break;
	case ETclSignInStage::Connecting:
	case ETclSignInStage::Connected:
		Disconnect();
		break;
	default:
		break;
	}
}

void
UTclSignInController::SignIn()
{
	if (Connections == nullptr)
		return;
	Result = nullptr;
	bStartedHere = true;
	Connections->Connect(Platform);
	Refresh();
}

void
UTclSignInController::Cancel()
{
	if (Connections == nullptr)
		return;
	Connections->CancelConnect(Platform);
	Refresh();
}

void
UTclSignInController::Disconnect()
{
	if (Connections == nullptr)
		return;
	Connections->Disconnect(Platform);
	Refresh();
}

void
UTclSignInController::OpenPage()
{
	if (Prompt != nullptr && OpenUrl)
		OpenUrl(Prompt->Url);
}

void
UTclSignInController::HandleConnectionChanged(const UTclPlatformConnection* Connection)
{
	if (Connection != nullptr && Connection->Platform == Platform)
		Refresh();
}

void
UTclSignInController::HandleConnectPrompt(const UTclPlatformConnectionPrompt* InPrompt)
{
	if (InPrompt == nullptr || InPrompt->Platform != Platform)
		return;

	// A sign-in someone else started, the game's Sign In node say, shows here
	// too, but only the one that asked opens the page.
	Prompt = const_cast<UTclPlatformConnectionPrompt*>(InPrompt);
	PromptDeadline = Clock() + InPrompt->ExpiresInSeconds;
	Result = nullptr;
	if (bStartedHere && bOpenBrowser && OpenUrl)
		OpenUrl(InPrompt->Url);
	Refresh();
}

void
UTclSignInController::HandleConnectResult(const UTclPlatformConnectionResult* InResult)
{
	if (InResult == nullptr || InResult->Platform != Platform)
		return;

	Prompt = nullptr;
	Result = const_cast<UTclPlatformConnectionResult*>(InResult);
	bStartedHere = false;
	Refresh();
}
