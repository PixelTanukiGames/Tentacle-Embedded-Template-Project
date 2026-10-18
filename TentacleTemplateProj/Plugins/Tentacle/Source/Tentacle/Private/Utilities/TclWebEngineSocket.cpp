// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclEngineSocket.h"
#include "IWebSocket.h"
#include "WebSocketsModule.h"

namespace
{
	/** The WebSocket to the Tentacle app, over the engine's WebSockets module. */
	class FTclWebEngineSocket final : public ITclEngineSocket, public TSharedFromThis<FTclWebEngineSocket>
	{
	public:
		explicit FTclWebEngineSocket(const FString& InUrl)
			: Url(InUrl)
		{
		}

		virtual ~FTclWebEngineSocket() override
		{
			Close();
		}

		virtual void Connect() override
		{
			// The WebSockets module must be loaded before the first CreateWebSocket,
			// and loading it from here (rather than relying on a module dependency
			// having already done so) keeps the plugin usable from a commandlet or a
			// test.
			FWebSocketsModule::Get();
			Socket = FWebSocketsModule::Get().CreateWebSocket(Url, TEXT("ws"));
			if (!Socket.IsValid())
			{
				if (OnConnectionError)
					OnConnectionError(FString::Printf(TEXT("could not create a WebSocket for %s"), *Url));
				return;
			}

			// Weak bindings: a callback arriving after this socket was released finds
			// nothing to call, instead of a dangling pointer.
			Socket->OnConnected().AddSP(this, &FTclWebEngineSocket::HandleConnected);
			Socket->OnConnectionError().AddSP(this, &FTclWebEngineSocket::HandleConnectionError);
			Socket->OnClosed().AddSP(this, &FTclWebEngineSocket::HandleClosed);
			Socket->OnMessage().AddSP(this, &FTclWebEngineSocket::HandleMessage);
			Socket->Connect();
		}

		virtual void Send(const FString& Json) override
		{
			if (Socket.IsValid())
				Socket->Send(Json);
		}

		virtual void Close() override
		{
			ClearCallbacks();
			if (!Socket.IsValid())
				return;

			// Clear before closing: Close() can fire OnClosed synchronously, and that
			// callback must not report a socket being discarded.
			Socket->OnConnected().Clear();
			Socket->OnConnectionError().Clear();
			Socket->OnClosed().Clear();
			Socket->OnMessage().Clear();

			// Closed even while it is still connecting: a socket left to finish
			// connecting would sit open until Tentacle's handshake deadline.
			Socket->Close();

			Socket.Reset();
		}

	private:
		void HandleConnected()
		{
			if (OnConnected)
				OnConnected();
		}

		void HandleConnectionError(const FString& Error)
		{
			if (OnConnectionError)
				OnConnectionError(Error);
		}

		void HandleClosed(int32 StatusCode, const FString& Reason, bool bWasClean)
		{
			if (OnClosed)
				OnClosed(StatusCode, Reason, bWasClean);
		}

		void HandleMessage(const FString& Message)
		{
			if (OnMessage)
				OnMessage(Message);
		}

		FString Url;
		TSharedPtr<IWebSocket> Socket;
	};
}

TSharedRef<ITclEngineSocket>
ITclEngineSocket::MakeWebSocket(const FString& Url)
{
	return MakeShared<FTclWebEngineSocket>(Url);
}
