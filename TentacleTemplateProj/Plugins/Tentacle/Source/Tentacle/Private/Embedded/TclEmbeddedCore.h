// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Utilities/TclEmbeddedOptions.h"

struct TclCore;

// Unity's TentacleEmbeddedCore. One core per process: it holds the platform
// connections and the database, so every Tentacle actor in the process (one per
// PIE world, say) opens its own connection to the same core. Everything here
// runs on the game thread.

/** The Tentacle core running inside this process, started by the first embedded connection. */
class FTclEmbeddedCore
{
public:
	/**
	 * The running core, started with Options if none is running yet. A core
	 * already running keeps the options it started with (a different set is
	 * logged once). Null when the core cannot start, with why in OutError.
	 */
	static TclCore* Acquire(const FTclEmbeddedOptions& Options, FString& OutError);

	/**
	 * Stop the core, closing every connection to it; those connections report
	 * closed. The next Acquire starts a new one. Called when the module shuts
	 * down and when a play-in-editor session ends.
	 */
	static void Shutdown();

	/** Whether a core is running. */
	static bool IsRunning();
};
