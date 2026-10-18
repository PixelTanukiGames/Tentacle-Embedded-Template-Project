// © 2025 Eldritch One. All rights reserved.

#include "TclSettings.h"
#include "Kismet/GameplayStatics.h"

UTclSettings*
UTclSettings::Load()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		auto* Loaded = Cast<UTclSettings>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
		if (IsValid(Loaded))
			return Loaded;
	}

	return NewObject<UTclSettings>();
}

void
UTclSettings::Save()
{
	UGameplayStatics::SaveGameToSlot(this, SlotName, UserIndex);
}
