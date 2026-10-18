// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclEngineConnection.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclEmbeddedOptions.h"
#include "Utilities/TclEngineSocket.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// The app's SESSION_PASSWORD_REFUSED (packages/common/src/json/engine-session-json.ts).
const TCHAR* const FTclEngineConnection::PasswordRefusedReason = TEXT("connection password missing or wrong");

FTclEngineConnection::FTclEngineConnection()
{
}

FTclEngineConnection::~FTclEngineConnection()
{
	Stop();
}

void
FTclEngineConnection::Start(const FString& InUrl, const FTclEngineIdentity& InIdentity)
{
	Stop();

	Url = InUrl;
	MakeSocket = [InUrl]() { return ITclEngineSocket::MakeWebSocket(InUrl); };
	Begin(InIdentity);
}

void
FTclEngineConnection::StartEmbedded(const FTclEmbeddedOptions& Options, const FTclEngineIdentity& InIdentity)
{
	Stop();

	Url = TEXT("the embedded Tentacle core");
	MakeSocket = [Options]() { return ITclEngineSocket::MakeEmbedded(Options); };
	Begin(InIdentity);
}

void
FTclEngineConnection::Begin(const FTclEngineIdentity& InIdentity)
{
	FString Problem;
	if (!InIdentity.IsValid(Problem))
	{
		tclErrorF("Tentacle: not connecting, the session identity is invalid: %s", *Problem);
		return;
	}

	Identity = InIdentity;
	bStopping = false;
	bReportedUnreachable = false;
	bReportedPasswordRefused = false;
	Attempt = 0;

	Connect();
}

void
FTclEngineConnection::Stop()
{
	bStopping = true;
	ReleaseSocket();

	const bool bWasOpen = State == EState::Ready;
	State = EState::Idle;
	SessionId.Empty();

	if (bWasOpen)
	{
		tclInfoF("Tentacle: disconnected from %s", *Url);
		OnSessionClosed.ExecuteIfBound();
	}
}

void
FTclEngineConnection::Tick(float DeltaSeconds)
{
	if (State == EState::WaitingToReconnect)
	{
		// The socket that dropped is released here, outside its own callbacks, so
		// it is never asked to destroy itself while it is still dispatching.
		ReleaseSocket();

		if (FPlatformTime::Seconds() >= ReconnectAt)
			Connect();
		return;
	}

	// A socket that delivers by polling does so here. Held for the call, so a
	// handler that disconnects cannot free the socket mid-tick.
	if (const TSharedPtr<ITclEngineSocket> Ticking = Socket)
		Ticking->Tick();
}

bool
FTclEngineConnection::Send(const FString& JsonString)
{
	if (State != EState::Ready || !Socket.IsValid())
	{
		tclDebug("Tentacle: dropping a message, the session is not open");
		return false;
	}

	Socket->Send(JsonString);
	return true;
}

void
FTclEngineConnection::Connect()
{
	if (bStopping || !MakeSocket)
		return;

	ReleaseSocket();
	State = EState::Connecting;
	Socket = MakeSocket();

	// Weak captures: a callback arriving after the owner released this object
	// finds nothing to call, instead of a dangling pointer.
	const TWeakPtr<FTclEngineConnection> Weak = AsShared();
	Socket->OnConnected = [Weak]()
	{
		if (const TSharedPtr<FTclEngineConnection> Self = Weak.Pin())
			Self->HandleConnected();
	};
	Socket->OnConnectionError = [Weak](const FString& Error)
	{
		if (const TSharedPtr<FTclEngineConnection> Self = Weak.Pin())
			Self->HandleConnectionError(Error);
	};
	Socket->OnClosed = [Weak](int32 StatusCode, const FString& Reason, bool bWasClean)
	{
		if (const TSharedPtr<FTclEngineConnection> Self = Weak.Pin())
			Self->HandleClosed(StatusCode, Reason, bWasClean);
	};
	Socket->OnMessage = [Weak](const FString& Message)
	{
		if (const TSharedPtr<FTclEngineConnection> Self = Weak.Pin())
			Self->HandleMessage(Message);
	};

	tclDebugF("Tentacle: connecting to %s (attempt %d)", *Url, Attempt + 1);
	Socket->Connect();
}

void
FTclEngineConnection::HandleConnected()
{
	if (State != EState::Connecting || !Socket.IsValid())
		return;

	// The server closes any socket whose first frame is not a valid hello, and
	// admits nothing before it, so the hello goes out before anything else.
	State = EState::Handshaking;
	Socket->Send(Identity.ToHelloMessage());
}

void
FTclEngineConnection::HandleConnectionError(const FString& Error)
{
	if (bStopping)
		return;

	// Tentacle not running is the ordinary case, so it is reported once per
	// outage rather than on every retry.
	if (!bReportedUnreachable)
	{
		bReportedUnreachable = true;
		tclWarnF("Tentacle: could not reach %s (%s). Is Tentacle running? Retrying in the background", *Url, *Error);
	}
	else
	{
		tclDebugF("Tentacle: still cannot reach %s (%s)", *Url, *Error);
	}

	ScheduleReconnect(TEXT("connection error"));
}

void
FTclEngineConnection::HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
{
	if (bStopping)
		return;

	if (State == EState::Handshaking && Reason == PasswordRefusedReason)
	{
		// The user's to fix, on one side or the other, so it is said once, plainly,
		// and the connection keeps trying: clearing or matching the password in
		// Tentacle lets the next attempt in.
		if (!bReportedPasswordRefused)
		{
			bReportedPasswordRefused = true;
			tclErrorF("Tentacle: %s has a connection password set, and %s. Set Connection Password on the Tentacle actor "
				"to the one under Settings > System > Connection Password in Tentacle. Retrying in the background",
				*Url, Identity.Password.IsEmpty() ? TEXT("this game sent none") : TEXT("the one this game sent is wrong"));
		}
	}
	else if (State == EState::Handshaking)
	{
		// A close during the handshake is the server refusing the hello, which
		// only an invalid identity or a full session registry produces; say so
		// rather than retrying silently.
		tclWarnF("Tentacle: %s closed the session during the handshake (%d %s)", *Url, StatusCode, *Reason);
	}
	else
	{
		tclInfoF("Tentacle: the connection closed (%d %s)", StatusCode, *Reason);
	}

	ScheduleReconnect(TEXT("closed"));
}

void
FTclEngineConnection::HandleMessage(const FString& Message)
{
	switch (State)
	{
	case EState::Handshaking:
		HandleWelcome(Message);
		return;

	case EState::Ready:
		OnMessageReceived.ExecuteIfBound(Message);
		return;

	default:
		tclDebug("Tentacle: ignoring a message received outside a session");
		return;
	}
}

void
FTclEngineConnection::HandleWelcome(const FString& Message)
{
	TSharedPtr<FJsonObject> Root;
	const auto Reader = TJsonReaderFactory<>::Create(Message);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		tclWarn("Tentacle: ignoring an unparseable frame while waiting for the welcome");
		return;
	}

	FString Type;
	if (!Root->TryGetStringField(TEXT("type"), Type) || Type != TEXT("Session_Welcome"))
	{
		tclWarnF("Tentacle: expected Session_Welcome, ignoring a '%s' frame", *Type);
		return;
	}

	const TSharedPtr<FJsonObject>* Data = nullptr;
	FString NewSessionId;
	if (!Root->TryGetObjectField(TEXT("data"), Data) || !(*Data)->TryGetStringField(TEXT("sessionId"), NewSessionId) || NewSessionId.IsEmpty())
	{
		tclWarn("Tentacle: the welcome carried no session id, ignoring it");
		return;
	}

	SessionId = NewSessionId;
	State = EState::Ready;
	Attempt = 0;
	bReportedUnreachable = false;
	bReportedPasswordRefused = false;

	tclInfoF("Tentacle: connected to %s (session %s)", *Url, *SessionId);
	OnSessionOpened.ExecuteIfBound(SessionId);
}

void
FTclEngineConnection::ScheduleReconnect(const FString& Why)
{
	if (bStopping)
		return;

	const bool bWasOpen = State == EState::Ready;
	State = EState::WaitingToReconnect;
	SessionId.Empty();

	Attempt += 1;
	const double Delay = ReconnectPolicy.DelayForAttempt(Attempt, FMath::FRand());
	ReconnectAt = FPlatformTime::Seconds() + Delay;
	tclDebugF("Tentacle: reconnecting in %.1fs (%s)", Delay, *Why);

	if (bWasOpen)
	{
		tclInfoF("Tentacle: disconnected from %s, reconnecting", *Url);
		OnSessionClosed.ExecuteIfBound();
	}
}

void
FTclEngineConnection::ReleaseSocket()
{
	if (!Socket.IsValid())
		return;

	Socket->Close();
	Socket.Reset();
}
