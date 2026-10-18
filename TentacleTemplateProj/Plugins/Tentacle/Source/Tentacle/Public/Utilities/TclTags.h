// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "TclTags.generated.h"

UENUM(BlueprintType)
enum class ETclTag : uint8
{
	Tentacle,
	Subsystem_Api,
	Subsystem_ViewerProperties,
	Subsystem_Auth,
	Subsystem_CustomControls,
	Subsystem_CustomEvents,
	Subsystem_Obs,
	Subsystem_Stream,
};

UCLASS(BlueprintType)
class TENTACLE_API UTclTags : public UObject
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure, Category="Tentacle")
	static FName
	GetTentacleTag(ETclTag Tag)
	{
		switch (Tag)
		{
			case ETclTag::Tentacle: return FName("Tcl.Tentacle");
			case ETclTag::Subsystem_Api: return FName("Tcl.Tentacle_Api");
			case ETclTag::Subsystem_ViewerProperties: return FName("Tcl.Tentacle_ViewerProperties");
			case ETclTag::Subsystem_Auth: return FName("Tcl.Tentacle_Auth");
			case ETclTag::Subsystem_CustomControls: return FName("Tcl.Tentacle_CustomControls");
			case ETclTag::Subsystem_CustomEvents: return FName("Tcl.Tentacle_CustomEvents");
			case ETclTag::Subsystem_Obs: return FName("Tcl.Tentacle_Obs");
			case ETclTag::Subsystem_Stream: return FName("Tcl.Tentacle_Stream");
			default: return FName("");
		}
	}
};
