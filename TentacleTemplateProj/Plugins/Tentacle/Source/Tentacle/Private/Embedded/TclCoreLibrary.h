// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include "tentacle_ffi.h"
THIRD_PARTY_INCLUDES_END

// The library is loaded at runtime rather than linked, so the plugin builds,
// packages, and runs in Paired mode on a machine that never built the core; only
// Embedded mode needs it, and it says so when it is missing. Every pointer is
// typed from the header's own declaration, which a test holds byte for byte to
// the core's (packages/app/tests/core-ffi-header.test.ts).

/**
 * The Tentacle core's library, loaded from the plugin's
 * Binaries/ThirdParty/TentacleCore/<platform> folder, with every function it
 * exports.
 */
struct FTclCoreLibrary
{
	decltype(&tcl_version) Version = nullptr;
	decltype(&tcl_core_create) CoreCreate = nullptr;
	decltype(&tcl_core_destroy) CoreDestroy = nullptr;
	decltype(&tcl_connection_open) ConnectionOpen = nullptr;
	decltype(&tcl_connection_send) ConnectionSend = nullptr;
	decltype(&tcl_connection_poll) ConnectionPoll = nullptr;
	decltype(&tcl_connection_wait) ConnectionWait = nullptr;
	decltype(&tcl_connection_is_closed) ConnectionIsClosed = nullptr;
	decltype(&tcl_connection_close) ConnectionClose = nullptr;

	/**
	 * The library, loaded the first time it is asked for. Null when it cannot
	 * be, with why in OutError; a failed load is not retried.
	 */
	static const FTclCoreLibrary* Get(FString& OutError);

	/** Where the library is expected for the platform this runs on, or empty where the core has no build. */
	static FString ExpectedPath();

	/** Counts a connection handle opened through the library, which it must outlive. */
	static void NoteConnectionOpened();

	/** Counts a connection handle closed through the library. */
	static void NoteConnectionClosed();

	/**
	 * Frees the library, once the core is stopped and no connection handle is
	 * open, so nothing can call into it afterwards; the next Get loads it
	 * again. With a handle still open it is kept, since closing that handle
	 * calls into it. Called at module shutdown.
	 */
	static void UnloadIfUnused();
};
