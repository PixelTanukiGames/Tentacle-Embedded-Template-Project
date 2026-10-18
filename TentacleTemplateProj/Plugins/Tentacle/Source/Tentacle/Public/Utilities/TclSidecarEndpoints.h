// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

/**
 * Where the running Tentacle sidecar can be reached.
 *
 * Both ports have well-known defaults, but the sidecar falls back to an
 * OS-assigned port when one is taken and publishes what it actually bound in a
 * small discovery file (`sidecar.json`) in its data folder. A plugin that
 * hardcodes the defaults silently stops working after such a fallback, so the
 * file is the primary source and the defaults are the last resort.
 *
 * Features:
 * - Reads the discovery file with per-field fallback: the sidecar omits the
 *   engine keys when its listener could not start, and the HTTP half is still
 *   usable in that state.
 * - Never fails: a missing, unreadable or malformed file resolves to the
 *   defaults, because Tentacle simply not running is the ordinary case.
 */
struct TENTACLE_API FTclSidecarEndpoints
{
	/** Default port of the sidecar's HTTP/tRPC API. */
	static constexpr int32 DefaultHttpPort = 47821;

	/** Default port of the sidecar's engine WebSocket listener. */
	static constexpr int32 DefaultEnginePort = 47822;

	/** Name of the discovery file the sidecar writes to its data folder. */
	inline static const FString DiscoveryFileName = TEXT("sidecar.json");

	/** Base URL of the tRPC API, e.g. "http://127.0.0.1:47821". */
	FString HttpBaseUrl = TEXT("http://127.0.0.1:47821");

	/** WebSocket URL of the engine listener, e.g. "ws://127.0.0.1:47822". */
	FString EngineSocketUrl = TEXT("ws://127.0.0.1:47822");

	/** True when these came out of the discovery file rather than the compiled-in defaults. */
	bool bFromDiscoveryFile = false;

	/**
	 * Tentacle's data folder, where the discovery file and the SQLite database
	 * live: the Tentacle folder under %APPDATA% on Windows, and under
	 * ~/Library/Application Support on Mac. Empty when it cannot be resolved.
	 */
	static FString
	DefaultDataDirectory();

	/**
	 * Resolve the endpoints, preferring what the sidecar published in
	 * DataDirectory (empty means the default folder). Always returns usable
	 * endpoints.
	 */
	static FTclSidecarEndpoints
	Resolve(const FString& DataDirectory = FString());

	/**
	 * Parse the discovery file's contents. Each field falls back to its default
	 * independently; text that is not a JSON object yields the defaults.
	 */
	static FTclSidecarEndpoints
	ParseDiscoveryJson(const FString& Json);
};
