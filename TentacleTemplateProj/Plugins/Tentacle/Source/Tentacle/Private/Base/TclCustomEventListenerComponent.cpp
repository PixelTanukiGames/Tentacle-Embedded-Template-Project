// © 2025 Eldritch One. All rights reserved.

#include "Base/TclCustomEventListenerComponent.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_CustomEvents.h"
#include "Sdk/TclCustomEvent.h"
#include "Utilities/TclCommon.h"

void
UTclCustomEventListenerComponent::BeginPlay()
{
	Super::BeginPlay();

	auto* Tentacle = UTclCommon::GetActorOfClass<ATentacle>(this);
	if (!Tentacle || !Tentacle->CustomEvents) return;

	Tentacle->CustomEvents->OnCustomEvent.AddDynamic(this, &UTclCustomEventListenerComponent::HandleCustomEvent);
	BoundTentacle = Tentacle;
}

void
UTclCustomEventListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The actor bound to at BeginPlay rather than whichever one a lookup finds
	// now, which may already be gone or be another; a lookup only when none was.
	ATentacle* Tentacle = BoundTentacle.IsExplicitlyNull() ? UTclCommon::GetActorOfClass<ATentacle>(this) : BoundTentacle.Get();
	BoundTentacle.Reset();
	if (Tentacle && Tentacle->CustomEvents)
	{
		Tentacle->CustomEvents->OnCustomEvent.RemoveDynamic(this, &UTclCustomEventListenerComponent::HandleCustomEvent);
	}

	Super::EndPlay(EndPlayReason);
}

void
UTclCustomEventListenerComponent::HandleCustomEvent(const UTclCustomEvent* Event, UTclViewer* Viewer)
{
	if (!Event) return;

	// IgnoreCase is what TArray<FString>::Contains did here (FString's == ignores
	// case), so a listener without Case Sensitive set matches exactly as before.
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	const bool bListening = EventNames.ContainsByPredicate([&](const FString& Name)
	{
		return Name.Equals(Event->Name, SearchCase);
	});
	if (!bListening) return;

	OnTriggered.Broadcast(Event, Viewer);
}
