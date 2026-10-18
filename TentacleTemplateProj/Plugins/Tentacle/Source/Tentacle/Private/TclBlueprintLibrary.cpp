// © 2025 Eldritch One. All rights reserved.

#include "TclBlueprintLibrary.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Stream.h"
#include "Base/Tentacle_Twitch.h"
#include "Base/Tentacle_TwitchExtras.h"
#include "Base/Tentacle_Kick.h"
#include "Base/Tentacle_Obs.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Base/Tentacle_CustomEvents.h"
#include "Base/Tentacle_CustomControls.h"
#include "Base/Tentacle_Viewers.h"
#include "Base/Tentacle_Bot.h"
#include "Base/Tentacle_Timers.h"
#include "Base/Tentacle_Connections.h"
#include "Utilities/TclCommon.h"
#include "TentacleModule.h"
#include "Engine/Engine.h"

ATentacle*
UTclBlueprintLibrary::GetTentacle(const UObject* WorldContextObject)
{
	static bool ErrorShown = false;
	const auto Tentacle = UTclCommon::GetActorOfClass<ATentacle>(WorldContextObject);
	if (!IsValid(Tentacle) && !ErrorShown)
	{
		const FString Msg = TEXT("Tentacle actor not found. Have you added a Tentacle actor to the level?");
		UE_LOG(LogTentacle, Error, TEXT("%s"), *Msg);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, Msg);
			ErrorShown = true;
		}
	}
	return Tentacle;
}

UTentacle_Stream*
UTclBlueprintLibrary::GetTentacleStream(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Stream>() : nullptr;
}

UTentacle_Twitch*
UTclBlueprintLibrary::GetTentacleTwitch(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Twitch>() : nullptr;
}

UTentacle_TwitchExtras*
UTclBlueprintLibrary::GetTentacleTwitchExtras(const UObject* WorldContextObject)
{
	const auto Twitch = GetTentacleTwitch(WorldContextObject);
	return IsValid(Twitch) ? Twitch->Extras : nullptr;
}

UTentacle_Kick*
UTclBlueprintLibrary::GetTentacleKick(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Kick>() : nullptr;
}

UTentacle_Obs*
UTclBlueprintLibrary::GetTentacleObs(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Obs>() : nullptr;
}

UTentacle_ViewerProperties*
UTclBlueprintLibrary::GetTentacleViewerProperties(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_ViewerProperties>() : nullptr;
}

UTentacle_CustomEvents*
UTclBlueprintLibrary::GetTentacleCustomEvents(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_CustomEvents>() : nullptr;
}

UTentacle_CustomControls*
UTclBlueprintLibrary::GetTentacleCustomControls(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_CustomControls>() : nullptr;
}

UTentacle_Viewers*
UTclBlueprintLibrary::GetTentacleViewers(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Viewers>() : nullptr;
}

UTentacle_Bot*
UTclBlueprintLibrary::GetTentacleBot(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Bot>() : nullptr;
}

UTentacle_Timers*
UTclBlueprintLibrary::GetTentacleTimers(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Timers>() : nullptr;
}

UTentacle_Connections*
UTclBlueprintLibrary::GetTentacleConnections(const UObject* WorldContextObject)
{
	const auto Tentacle = GetTentacle(WorldContextObject);
	return IsValid(Tentacle) ? Tentacle->FindComponentByClass<UTentacle_Connections>() : nullptr;
}
