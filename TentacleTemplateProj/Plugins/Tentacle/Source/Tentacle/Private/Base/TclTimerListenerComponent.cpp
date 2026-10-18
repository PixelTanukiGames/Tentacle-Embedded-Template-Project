// © 2025 Eldritch One. All rights reserved.

#include "Base/TclTimerListenerComponent.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Timers.h"
#include "Sdk/TclTimerEvent.h"
#include "Utilities/TclCommon.h"

void
UTclTimerListenerComponent::BeginPlay()
{
	Super::BeginPlay();

	auto* Tentacle = UTclCommon::GetActorOfClass<ATentacle>(this);
	if (!Tentacle || !Tentacle->Timers) return;

	Tentacle->Timers->OnTimerFired.AddDynamic(this, &UTclTimerListenerComponent::HandleTimerFired);
	BoundTentacle = Tentacle;
}

void
UTclTimerListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The actor bound to at BeginPlay rather than whichever one a lookup finds
	// now, which may already be gone or be another; a lookup only when none was.
	ATentacle* Tentacle = BoundTentacle.IsExplicitlyNull() ? UTclCommon::GetActorOfClass<ATentacle>(this) : BoundTentacle.Get();
	BoundTentacle.Reset();
	if (Tentacle && Tentacle->Timers)
	{
		Tentacle->Timers->OnTimerFired.RemoveDynamic(this, &UTclTimerListenerComponent::HandleTimerFired);
	}

	Super::EndPlay(EndPlayReason);
}

void
UTclTimerListenerComponent::HandleTimerFired(const UTclTimerEvent* Event)
{
	if (!Event) return;

	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	const bool bListening = TimerNames.ContainsByPredicate([&](const FString& Name)
	{
		return Name.Equals(Event->Name, SearchCase);
	});
	if (!bListening) return;

	OnFired.Broadcast(Event);
}
