// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

struct FTclEmbeddedOptions;

/**
 * What an engine connection runs over: the WebSocket to the Tentacle app, or
 * a connection to the Tentacle core embedded in the game. FTclEngineConnection
 * does the handshake and the reconnects on top, the same for both.
 *
 * Features:
 * - Reports through its callbacks, always on the game thread, and never from
 *   inside Connect() for the embedded core (which reports on the next Tick).
 * - Close() drops the callbacks before closing, so closing reports nothing.
 * - Owned by a TSharedPtr; its owner holds a reference across Tick(), so a
 *   callback that releases the socket cannot free it mid-call.
 */
class ITclEngineSocket
{
public:
	virtual ~ITclEngineSocket() = default;

	/** Start opening the socket. The outcome arrives as OnConnected or OnConnectionError. */
	virtual void Connect() = 0;

	/** Send one JSON frame on an open socket. */
	virtual void Send(const FString& Json) = 0;

	/** Close the socket, reporting nothing. Safe to call more than once. */
	virtual void Close() = 0;

	/** Called every frame. A socket that delivers by polling does so here. */
	virtual void Tick() {}

	TFunction<void()> OnConnected;
	TFunction<void(const FString& Error)> OnConnectionError;
	TFunction<void(int32 StatusCode, const FString& Reason, bool bWasClean)> OnClosed;
	TFunction<void(const FString& Message)> OnMessage;

	/** A WebSocket to the Tentacle app's engine listener at Url. */
	static TSharedRef<ITclEngineSocket> MakeWebSocket(const FString& Url);

	/** A connection to the Tentacle core embedded in this process, started with Options if it is not running. */
	static TSharedRef<ITclEngineSocket> MakeEmbedded(const FTclEmbeddedOptions& Options);

protected:
	/** Drop every callback, so nothing more is reported. */
	void ClearCallbacks()
	{
		OnConnected = nullptr;
		OnConnectionError = nullptr;
		OnClosed = nullptr;
		OnMessage = nullptr;
	}
};
