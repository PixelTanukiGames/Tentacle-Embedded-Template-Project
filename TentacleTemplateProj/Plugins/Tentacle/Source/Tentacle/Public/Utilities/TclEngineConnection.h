// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Utilities/TclEngineIdentity.h"
#include "Utilities/TclReconnectPolicy.h"

class ITclEngineSocket;
struct FTclEmbeddedOptions;

DECLARE_DELEGATE_OneParam(FTclEngineConnectionMessage, const FString& /*JsonMessage*/);
DECLARE_DELEGATE_OneParam(FTclEngineSessionOpened, const FString& /*SessionId*/);
DECLARE_DELEGATE(FTclEngineSessionClosed);

/**
 * The plugin's connection to Tentacle: a WebSocket to the Tentacle app
 * (Start), or a connection to the Tentacle core running inside the game
 * (StartEmbedded). Both speak the same messages, so nothing above the
 * connection changes with the mode.
 *
 * Tentacle hosts the engine listener and this plugin dials in, so several
 * games - an editor PIE session next to a packaged build - can be connected at
 * once and nothing in the game ever opens a listening socket (no firewall
 * prompt, no leaked PIE ports). Uses the built-in UE WebSockets module.
 *
 * Wire shape:
 * - The first frame sent is a `Session_Hello` carrying the engine identity.
 * - Tentacle answers with `Session_Welcome`, which hands back the session id.
 *   Only then is the session open; nothing is sent or surfaced before it.
 * - After that the connection speaks the `TentacleMsg_*` vocabulary in both
 *   directions.
 *
 * Features:
 * - Reconnects with backoff whenever the socket drops or cannot be opened, so a
 *   game started before Tentacle simply connects when Tentacle comes up.
 * - Must be owned by a TSharedPtr and ticked every frame: socket callbacks bind
 *   weakly to it, so a callback arriving after the owner let go is a no-op.
 * - A message before the welcome, or a malformed one at any time, is logged and
 *   ignored; it never disturbs the session.
 */
class TENTACLE_API FTclEngineConnection : public TSharedFromThis<FTclEngineConnection>
{
public:
	FTclEngineConnection();
	~FTclEngineConnection();

	/** Open the connection to Url, introducing this process as Identity. Reconnects until Stop(). */
	void
	Start(const FString& Url, const FTclEngineIdentity& InIdentity);

	/**
	 * Open a connection to the Tentacle core running inside this process,
	 * starting it with Options if it is not running yet, and introducing this
	 * process as Identity. Reconnects until Stop(), as Start() does: a core
	 * that is missing or stopped is tried again with the same backoff.
	 */
	void
	StartEmbedded(const FTclEmbeddedOptions& Options, const FTclEngineIdentity& InIdentity);

	/** Close the connection and stop reconnecting. Safe to call more than once. */
	void
	Stop();

	/** Must be called every frame: drives the reconnect timer and socket teardown. */
	void
	Tick(float DeltaSeconds);

	/**
	 * Send one JSON message to Tentacle. Returns false, and sends nothing, while
	 * the session is not open - a control registered before the welcome is
	 * re-sent when Tentacle asks for the controls on connect.
	 */
	bool
	Send(const FString& JsonString);

	/** True once Tentacle has welcomed this session. */
	bool
	IsSessionOpen() const { return State == EState::Ready; }

	/** The id Tentacle assigned to this session; empty while not open. */
	const FString&
	GetSessionId() const { return SessionId; }

	/**
	 * The reason Tentacle closes a session with when its user set a connection
	 * password and the hello carried none, or a wrong one.
	 */
	static const TCHAR* const PasswordRefusedReason;

	/** Backoff between reconnect attempts. */
	FTclReconnectPolicy ReconnectPolicy;

	/** Called for every message received on an open session. */
	FTclEngineConnectionMessage OnMessageReceived;

	/** Called when Tentacle welcomes the session. */
	FTclEngineSessionOpened OnSessionOpened;

	/** Called when an open session closes, for whatever reason. */
	FTclEngineSessionClosed OnSessionClosed;

private:
	enum class EState : uint8
	{
		Idle,
		Connecting,
		Handshaking,
		Ready,
		WaitingToReconnect,
	};

	void
	Connect();

	void
	HandleConnected();

	void
	HandleConnectionError(const FString& Error);

	void
	HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean);

	void
	HandleMessage(const FString& Message);

	void
	HandleWelcome(const FString& Message);

	/** Mark the session closed (if it was open) and arm the reconnect timer. */
	void
	ScheduleReconnect(const FString& Why);

	/** Drop the socket, reporting nothing. */
	void
	ReleaseSocket();

	/** Shared by Start and StartEmbedded once they know how to make a socket. */
	void
	Begin(const FTclEngineIdentity& InIdentity);

	/** Where the connection goes, for the log: the app's URL, or the embedded core. */
	FString Url;
	FTclEngineIdentity Identity;

	/** Makes a fresh socket for each attempt. */
	TFunction<TSharedRef<ITclEngineSocket>()> MakeSocket;
	TSharedPtr<ITclEngineSocket> Socket;
	EState State = EState::Idle;
	FString SessionId;

	/** Reconnects since the last successful welcome. */
	int32 Attempt = 0;

	/** Platform time the next connect attempt is due. */
	double ReconnectAt = 0.0;

	/** Set by Stop(); suppresses reconnects from callbacks that fire during teardown. */
	bool bStopping = false;

	/** Whether the "cannot reach Tentacle" warning has been logged for this outage. */
	bool bReportedUnreachable = false;

	/** Whether the "Tentacle wants its connection password" error has been logged since the last session. */
	bool bReportedPasswordRefused = false;
};
