// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Timers.h"
#include "Base/Tentacle.h"
#include "Base/TclDefinitionChangeInternal.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/TclMsg_ServerToClient_TimersDefinitions.h"
#include "Sdk/TclMsg_ServerToClient_TimersFired.h"
#include "Sdk/TclTimerEvent.h"
#include "Utilities/TclCommon.h"

namespace
{
	/**
	 * The id of the timer with this name, or empty after running OnComplete
	 * with why: Not Found when there is no such timer, Failed when the timers
	 * have not arrived from Tentacle, which says nothing about the name.
	 */
	FString ResolveTimerId(const UTentacle_Timers& Timers, const FString& TimerName, bool bCaseSensitive, const FTclOnDefinitionComplete& OnComplete)
	{
		if (!Timers.bAreDefinitionsLoaded)
		{
			tclWarnF("Tentacle Timers: the timers have not arrived from Tentacle yet, so '%s' cannot be found; is it connected?", *TimerName);
			FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::Failed);
			return FString();
		}

		bool bFound = false;
		const UTclTimerDefinition* Definition = Timers.FindDefinitionByName(TimerName, bFound, bCaseSensitive);
		if (!Definition)
		{
			FTclDefinitionChange::Finish(OnComplete, ETclDefinitionResult::NotFound);
			return FString();
		}
		return Definition->Id;
	}

	/** The HTTP client of the Tentacle actor that owns these timers, or null. */
	UTentacleHttpClient* TimersHttpClient(const UTentacle_Timers& Timers)
	{
		const ATentacle* Tentacle = Timers.GetOwner<ATentacle>();
		return Tentacle ? Tentacle->HttpClient : nullptr;
	}

	/** Call a mutation that acts on one timer, named by its id. */
	void DriveTimer(
		const UTentacle_Timers& Timers, FTclDefinitionChange::FCall Call, const TCHAR* Procedure,
		const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
	{
		const FString Id = ResolveTimerId(Timers, TimerName, bCaseSensitive, OnComplete);
		if (Id.IsEmpty())
			return;

		const TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
		Input->SetStringField(TEXT("id"), Id);
		FTclDefinitionChange::Change(TimersHttpClient(Timers), Call, Procedure, Input, OnComplete);
	}
}

void
UTentacle_Timers::CreateTimer(const FTclTimerSettings& Settings, const FTclOnDefinitionComplete& OnComplete)
{
	// The server requires a name on create, so an empty one is sent as it is
	// and refused there, rather than left out.
	const TSharedRef<FJsonObject> Input = Settings.ToInput();
	Input->SetStringField(TEXT("name"), Settings.Name);
	FTclDefinitionChange::Change(TimersHttpClient(*this), &UTentacleHttpClient::CreateTimer, TEXT("timers.create"), Input, OnComplete);
}

void
UTentacle_Timers::UpdateTimer(const FString& TimerName, const FTclTimerSettings& Settings, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	const FString Id = ResolveTimerId(*this, TimerName, bCaseSensitive, OnComplete);
	if (Id.IsEmpty())
		return;

	const TSharedRef<FJsonObject> Input = Settings.ToInput();
	Input->SetStringField(TEXT("id"), Id);
	FTclDefinitionChange::Change(TimersHttpClient(*this), &UTentacleHttpClient::UpdateTimer, TEXT("timers.update"), Input, OnComplete);
}

void
UTentacle_Timers::DeleteTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	DriveTimer(*this, &UTentacleHttpClient::DeleteTimer, TEXT("timers.delete"), TimerName, OnComplete, bCaseSensitive);
}

void
UTentacle_Timers::StartTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	DriveTimer(*this, &UTentacleHttpClient::StartTimer, TEXT("timers.start"), TimerName, OnComplete, bCaseSensitive);
}

void
UTentacle_Timers::PauseTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	DriveTimer(*this, &UTentacleHttpClient::PauseTimer, TEXT("timers.pause"), TimerName, OnComplete, bCaseSensitive);
}

void
UTentacle_Timers::ResetTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	DriveTimer(*this, &UTentacleHttpClient::ResetTimer, TEXT("timers.reset"), TimerName, OnComplete, bCaseSensitive);
}

void
UTentacle_Timers::FireTimer(const FString& TimerName, const FTclOnDefinitionComplete& OnComplete, bool bCaseSensitive)
{
	DriveTimer(*this, &UTentacleHttpClient::FireTimer, TEXT("timers.fire"), TimerName, OnComplete, bCaseSensitive);
}

void
UTentacle_Timers::HandleMsg_Fired(UTclMsg_ServerToClient_TimersFired* Msg)
{
	tclSoftAssert(Msg && Msg->Data);

	// Deliberately not de-duplicated: every fire is a fire.
	OnTimerFired.Broadcast(Msg->Data);

	// Iterate a copy: a handler may subscribe or unsubscribe while it runs, and
	// the subscriptions a dispatch runs are the ones present when it started.
	const TArray<FSubscription> Snapshot = Subscriptions;
	for (const FSubscription& Subscription : Snapshot)
	{
		if (Subscription.Matches(Msg->Data->Name))
			Subscription.Handler.ExecuteIfBound(Msg->Data);
	}

	// A handler whose object has been destroyed can never run again.
	Subscriptions.RemoveAll([](const FSubscription& Subscription) { return !Subscription.Handler.IsBound(); });
}

void
UTentacle_Timers::HandleMsg_Definitions(UTclMsg_ServerToClient_TimersDefinitions* Msg, const FString& DefinitionsJson)
{
	tclSoftAssert(Msg);

	// Tentacle sends the current definitions every time a session opens, so
	// ones identical to those held are not a change.
	if (bAreDefinitionsLoaded && DefinitionsJson == DefinitionsKey)
		return;

	Definitions = Msg->Definitions;
	Definitions.RemoveAll([](const UTclTimerDefinition* Definition) { return Definition == nullptr; });
	bAreDefinitionsLoaded = true;
	DefinitionsKey = DefinitionsJson;

	OnDefinitionsChanged.Broadcast();
}

UTclTimerDefinition*
UTentacle_Timers::FindDefinitionByName(const FString& Name, bool& Found, bool bCaseSensitive) const
{
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (UTclTimerDefinition* Definition : Definitions)
	{
		if (Definition && !Name.IsEmpty() && Definition->Name.Equals(Name, SearchCase))
		{
			Found = true;
			return Definition;
		}
	}
	Found = false;
	return nullptr;
}

void
UTentacle_Timers::Subscribe(const FString& TimerName, FTclTimerHandler Handler, bool bCaseSensitive)
{
	if (TimerName.IsEmpty() || !Handler.IsBound())
	{
		tclWarn("Tentacle Timers: Subscribe needs a timer name and a handler; nothing subscribed.");
		return;
	}

	// The same handler on a name its rule already matches would run twice for
	// every fire, so a repeat is a no-op.
	const bool bAlreadySubscribed = Subscriptions.ContainsByPredicate([&](const FSubscription& Existing)
	{
		return Existing.Handler == Handler && Existing.bCaseSensitive == bCaseSensitive && Existing.Matches(TimerName);
	});
	if (bAlreadySubscribed)
		return;

	FSubscription& Subscription = Subscriptions.AddDefaulted_GetRef();
	Subscription.TimerName = TimerName;
	Subscription.Handler = MoveTemp(Handler);
	Subscription.bCaseSensitive = bCaseSensitive;
}

void
UTentacle_Timers::Unsubscribe(const FString& TimerName, FTclTimerHandler Handler)
{
	Subscriptions.RemoveAll([&](const FSubscription& Subscription)
	{
		return Subscription.Handler == Handler && Subscription.Matches(TimerName);
	});
}

bool
UTentacle_Timers::FSubscription::Matches(const FString& Name) const
{
	return TimerName.Equals(Name, bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase);
}
