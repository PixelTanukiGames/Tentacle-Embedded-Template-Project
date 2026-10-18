// © 2024 Eldritch One. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTentacle, Log, All)

class FTentacleModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FTentacleModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FTentacleModule>("Tentacle");
	}

	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("Tentacle");
	}

private:
	/** The end-of-PIE hook that stops the embedded core; editor builds only. */
	FDelegateHandle EndPieHandle;
};
