// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_CustomEvents.h"
#include "Base/Tentacle.h"
#include "Sdk/TclMsg_ServerToClient_CustomEventsDefinitions.h"
#include "Sdk/TclMsg_ServerToClient_CustomEventsEvent.h"
#include "Sdk/TclCustomEvent.h"
#include "Sdk/TclViewerData.h"
#include "Utilities/TclCommon.h"

void
UTentacle_CustomEvents::HandleMsg_Event(UTclMsg_ServerToClient_CustomEventsEvent* Msg)
{
	tclSoftAssert(Msg && Msg->Data);

	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	UTclViewer* Viewer = nullptr;
	if (Msg->Data->Viewer)
	{
		Viewer = Tentacle->UpsertViewer(Msg->Data->Viewer);
	}

	OnCustomEvent.Broadcast(Msg->Data, Viewer);

	// Iterate a copy: a handler may subscribe or unsubscribe while it runs, and
	// the subscriptions a dispatch runs are the ones present when it started.
	const TArray<FSubscription> Snapshot = Subscriptions;
	for (const FSubscription& Subscription : Snapshot)
	{
		if (Subscription.Matches(Msg->Data->Name))
			Subscription.Handler.ExecuteIfBound(Msg->Data, Viewer);
	}

	// A handler whose object has been destroyed can never run again.
	Subscriptions.RemoveAll([](const FSubscription& Subscription) { return !Subscription.Handler.IsBound(); });
}

void
UTentacle_CustomEvents::HandleMsg_Definitions(UTclMsg_ServerToClient_CustomEventsDefinitions* Msg, const FString& DefinitionsJson)
{
	tclSoftAssert(Msg);

	// Tentacle sends the current definitions every time a session opens, so
	// ones identical to those held are not a change.
	if (bAreDefinitionsLoaded && DefinitionsJson == DefinitionsKey)
		return;

	Definitions = Msg->Definitions;
	Definitions.RemoveAll([](const UTclCustomEventDefinition* Definition) { return Definition == nullptr; });
	bAreDefinitionsLoaded = true;
	DefinitionsKey = DefinitionsJson;

	OnDefinitionsChanged.Broadcast();
}

UTclCustomEventDefinition*
UTentacle_CustomEvents::FindDefinitionByName(const FString& Name, bool& Found, bool bCaseSensitive) const
{
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (UTclCustomEventDefinition* Definition : Definitions)
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
UTentacle_CustomEvents::Subscribe(const FString& EventName, FTclCustomEventHandler Handler, bool bCaseSensitive)
{
	if (EventName.IsEmpty() || !Handler.IsBound())
	{
		tclWarn("Tentacle Custom Events: Subscribe needs an event name and a handler; nothing subscribed.");
		return;
	}

	// The same handler on a name its rule already matches would run twice for
	// every event, so a repeat is a no-op.
	const bool bAlreadySubscribed = Subscriptions.ContainsByPredicate([&](const FSubscription& Existing)
	{
		return Existing.Handler == Handler && Existing.bCaseSensitive == bCaseSensitive && Existing.Matches(EventName);
	});
	if (bAlreadySubscribed)
		return;

	FSubscription& Subscription = Subscriptions.AddDefaulted_GetRef();
	Subscription.EventName = EventName;
	Subscription.Handler = MoveTemp(Handler);
	Subscription.bCaseSensitive = bCaseSensitive;
}

void
UTentacle_CustomEvents::Unsubscribe(const FString& EventName, FTclCustomEventHandler Handler)
{
	Subscriptions.RemoveAll([&](const FSubscription& Subscription)
	{
		return Subscription.Handler == Handler && Subscription.Matches(EventName);
	});
}

bool
UTentacle_CustomEvents::FSubscription::Matches(const FString& Name) const
{
	return EventName.Equals(Name, bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase);
}
