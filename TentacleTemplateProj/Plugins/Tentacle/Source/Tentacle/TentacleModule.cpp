// © 2024 Eldritch One. All rights reserved.

#include "TentacleModule.h"
#include "Embedded/TclCoreLibrary.h"
#include "Embedded/TclEmbeddedCore.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY(LogTentacle)

void FTentacleModule::StartupModule()
{
	UE_LOG(LogTentacle, Log, TEXT("StartupModule"));

#if WITH_EDITOR
	// A play-in-editor session's embedded core ends with it, as a game's ends
	// with the process, so the editor sitting idle holds no platform
	// connections and the next session starts from the settings it has then.
	EndPieHandle = FEditorDelegates::EndPIE.AddLambda([](const bool /*bIsSimulating*/)
	{
		FTclEmbeddedCore::Shutdown();
	});
#endif
}

void FTentacleModule::ShutdownModule()
{
	UE_LOG(LogTentacle, Log, TEXT("ShutdownModule"));

#if WITH_EDITOR
	FEditorDelegates::EndPIE.Remove(EndPieHandle);
#endif

	FTclEmbeddedCore::Shutdown();
	// With the core stopped, nothing runs code from its library any more: freed,
	// so the module leaves nothing of the core behind, and an editor that
	// unloads the plugin no longer keeps the file locked.
	FTclCoreLibrary::UnloadIfUnused();
}

IMPLEMENT_MODULE(FTentacleModule, Tentacle)
