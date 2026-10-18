// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_TwitchExtras.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Stream.h"

void
UTentacle_TwitchExtras::BeginIncomingRaid()
{
	bIsIncomingRaidOngoing = true;
}

void
UTentacle_TwitchExtras::EndIncomingRaid()
{
	bIsIncomingRaidOngoing = false;
	OnIncomingRaidEnded.Broadcast();

	// The deprecated Stream component keeps its own raid flag and event for the
	// graphs still bound to them; ending the raid here ends it there too.
	auto* Tentacle = GetTypedOuter<ATentacle>();
	if (Tentacle && Tentacle->Stream)
		Tentacle->Stream->MirrorRaidEnded();
}
