// © 2025 Eldritch One. All rights reserved.

#include "Embedded/TclEmbeddedCore.h"
#include "Embedded/TclCoreLibrary.h"
#include "Utilities/TclCommon.h"

// Distinct file-scope names, for unity builds (see TclCoreLibrary.cpp).
namespace
{
	TclCore* RunningCore = nullptr;
	FTclEmbeddedOptions RunningCoreOptions;
	bool bWarnedAboutCoreOptions = false;
}

TclCore*
FTclEmbeddedCore::Acquire(const FTclEmbeddedOptions& Options, FString& OutError)
{
	const FTclCoreLibrary* Library = FTclCoreLibrary::Get(OutError);
	if (Library == nullptr)
		return nullptr;

	if (RunningCore != nullptr)
	{
		if (Options != RunningCoreOptions && !bWarnedAboutCoreOptions)
		{
			bWarnedAboutCoreOptions = true;
			tclWarn("Tentacle: the embedded core is already running with other Embedded settings; it keeps those until it restarts");
		}
		return RunningCore;
	}

	const FString ConfigJson = Options.ToConfigJson();
	const FTCHARToUTF8 Config(*ConfigJson);
	RunningCore = Library->CoreCreate(reinterpret_cast<const uint8_t*>(Config.Get()), static_cast<size_t>(Config.Length()));
	if (RunningCore == nullptr)
	{
		OutError = TEXT("the Tentacle core could not start (its log says why)");
		return nullptr;
	}

	RunningCoreOptions = Options;
	bWarnedAboutCoreOptions = false;
	const FString Where = Options.DataDirectory.IsEmpty()
		? FString(TEXT("keeping nothing between runs"))
		: FString::Printf(TEXT("data in %s"), *Options.DataDirectory);
	tclInfoF("Tentacle: started the embedded core %s (%s)", UTF8_TO_TCHAR(Library->Version()), *Where);
	return RunningCore;
}

void
FTclEmbeddedCore::Shutdown()
{
	if (RunningCore == nullptr)
		return;

	FString Error;
	if (const FTclCoreLibrary* Library = FTclCoreLibrary::Get(Error))
		Library->CoreDestroy(RunningCore);

	RunningCore = nullptr;
	tclInfo("Tentacle: stopped the embedded core");
}

bool
FTclEmbeddedCore::IsRunning()
{
	return RunningCore != nullptr;
}
