// © 2025 Eldritch One. All rights reserved.

#include "Base/TclSignInAction.h"

#include "Base/Tentacle.h"
#include "Base/Tentacle_Connections.h"
#include "HAL/PlatformProcess.h"
#include "Sdk/TclPlatformConnectionPrompt.h"
#include "Sdk/TclPlatformConnectionResult.h"
#include "TentacleModule.h"
#include "UI/TclConnectionsWidgetRow.h"
#include "Utilities/TclCommon.h"

namespace
{
	// Tentacle drops an attempt that a new Connect replaces without reporting
	// it, so the node waiting on it would wait forever: each platform's current
	// Sign In node is remembered, and a new one ends it.
	TMap<ETclStreamPlatform, TWeakObjectPtr<UTclSignInAction>>& CurrentSignIns()
	{
		static TMap<ETclStreamPlatform, TWeakObjectPtr<UTclSignInAction>> Current;
		return Current;
	}

	/** Why a sign-in did not connect, in the words the Connections widget uses. */
	FString FailureMessage(const UTclPlatformConnectionResult* Result)
	{
		const FString Detail = FTclConnectionsWidgetRow::ResultDetail(Result);
		return Detail.IsEmpty() ? FString(TEXT("Sign-in was cancelled")) : Detail;
	}
}

UTclSignInAction*
UTclSignInAction::SignIn(UObject* WorldContextObject, ETclStreamPlatform Platform, bool bOpenBrowser)
{
	UTclSignInAction* Action = NewObject<UTclSignInAction>();
	Action->WorldContext = WorldContextObject;
	Action->Platform = Platform;
	Action->bOpenBrowser = bOpenBrowser;
	Action->OpenUrl = [](const FString& Url) { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); };
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void
UTclSignInAction::Activate()
{
	ATentacle* Tentacle = WorldContext.IsValid() ? UTclCommon::GetActorOfClass<ATentacle>(WorldContext.Get()) : nullptr;
	if (!IsValid(Tentacle) || !IsValid(Tentacle->Connections))
	{
		const FString Message = TEXT("No Tentacle actor in the level. Have you added a Tentacle actor?");
		UE_LOG(LogTentacle, Warning, TEXT("Tentacle: Sign In failed: %s"), *Message);
		Fail(Message);
		return;
	}
	if (!Tentacle->IsEmbedded())
	{
		Fail(TEXT("Twitch and Kick connect through the Tentacle app"));
		return;
	}

	if (const TWeakObjectPtr<UTclSignInAction>* Earlier = CurrentSignIns().Find(Platform))
	{
		if (UTclSignInAction* Replaced = Earlier->Get(); Replaced != nullptr && Replaced != this)
		{
			Replaced->Fail(TEXT("Sign-in was cancelled"));
		}
	}
	CurrentSignIns().Add(Platform, this);

	Connections = Tentacle->Connections;
	Connections->OnConnectPrompt.AddDynamic(this, &UTclSignInAction::HandlePrompt);
	Connections->OnConnectResult.AddDynamic(this, &UTclSignInAction::HandleResult);
	Connections->Connect(Platform);
}

void
UTclSignInAction::HandlePrompt(const UTclPlatformConnectionPrompt* Prompt)
{
	if (bFinished || Prompt == nullptr || Prompt->Platform != Platform)
	{
		return;
	}
	if (bOpenBrowser && OpenUrl)
	{
		OpenUrl(Prompt->Url);
	}
	OnPrompt.Broadcast(Prompt->Url, Prompt->UserCode, FString());
}

void
UTclSignInAction::HandleResult(const UTclPlatformConnectionResult* Result)
{
	if (bFinished || Result == nullptr || Result->Platform != Platform)
	{
		return;
	}
	if (Result->Outcome == ETclPlatformConnectionOutcome::Connected)
	{
		Finish();
		OnConnected.Broadcast(FString(), FString(), FString());
		return;
	}
	Fail(FailureMessage(Result));
}

void
UTclSignInAction::Fail(const FString& Message)
{
	if (bFinished)
	{
		return;
	}
	Finish();
	OnFailed.Broadcast(FString(), FString(), Message);
}

void
UTclSignInAction::Finish()
{
	bFinished = true;
	if (UTentacle_Connections* Listening = Connections.Get())
	{
		Listening->OnConnectPrompt.RemoveDynamic(this, &UTclSignInAction::HandlePrompt);
		Listening->OnConnectResult.RemoveDynamic(this, &UTclSignInAction::HandleResult);
	}
	if (const TWeakObjectPtr<UTclSignInAction>* Current = CurrentSignIns().Find(Platform); Current != nullptr && Current->Get() == this)
	{
		CurrentSignIns().Remove(Platform);
	}
	SetReadyToDestroy();
}
