// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclEngineSocket.h"
#include "Embedded/TclCoreLibrary.h"
#include "Embedded/TclEmbeddedCore.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclEmbeddedOptions.h"

// Unity's TentacleEmbeddedTransport, without its reader thread: the connection
// is ticked on the game thread already, so the core is polled from there and
// every frame is handled where a game expects it, with nothing to marshal.

namespace
{
	/** Frames handled in one tick at most, so a burst cannot stall a frame; the rest wait for the next. */
	constexpr int32 MaxEmbeddedFramesPerTick = 512;

	/** A connection to the Tentacle core embedded in this process. */
	class FTclEmbeddedEngineSocket final : public ITclEngineSocket
	{
	public:
		explicit FTclEmbeddedEngineSocket(const FTclEmbeddedOptions& InOptions)
			: Options(InOptions)
		{
			Buffer.SetNumUninitialized(64 * 1024);
		}

		virtual ~FTclEmbeddedEngineSocket() override
		{
			Close();
		}

		virtual void Connect() override
		{
			FString Error;
			Library = FTclCoreLibrary::Get(Error);
			TclCore* Core = Library != nullptr ? FTclEmbeddedCore::Acquire(Options, Error) : nullptr;
			Connection = Core != nullptr ? Library->ConnectionOpen(Core) : nullptr;
			if (Connection != nullptr)
				FTclCoreLibrary::NoteConnectionOpened();
			if (Connection == nullptr && Error.IsEmpty())
				Error = TEXT("the Tentacle core refused the connection");

			// Reported on the next tick, as a socket reports, never from inside Connect().
			PendingError = Connection != nullptr ? FString() : Error;
			bPendingConnected = Connection != nullptr;
			bReportedClose = false;
		}

		virtual void Send(const FString& Json) override
		{
			if (Connection == nullptr)
				return;

			const FTCHARToUTF8 Frame(*Json);
			const int32 Result = Library->ConnectionSend(
				Connection, reinterpret_cast<const uint8_t*>(Frame.Get()), static_cast<size_t>(Frame.Length()));
			if (Result != TCL_OK)
				tclDebugF("Tentacle: the embedded core did not take a frame (%d)", Result);
		}

		virtual void Close() override
		{
			ClearCallbacks();
			PendingError.Empty();
			bPendingConnected = false;
			if (Connection != nullptr)
			{
				Library->ConnectionClose(Connection);
				Connection = nullptr;
				FTclCoreLibrary::NoteConnectionClosed();
			}
		}

		virtual void Tick() override
		{
			if (!PendingError.IsEmpty())
			{
				const FString Error = MoveTemp(PendingError);
				PendingError.Empty();
				if (OnConnectionError)
					OnConnectionError(Error);
				return;
			}

			if (Connection == nullptr)
				return;

			if (bPendingConnected)
			{
				bPendingConnected = false;
				if (OnConnected)
					OnConnected();
			}

			// Every callback may close this socket, so the connection is checked
			// again after each one.
			bool bDrained = false;
			for (int32 Frames = 0; Connection != nullptr && Frames < MaxEmbeddedFramesPerTick; ++Frames)
			{
				const int64 Length = Library->ConnectionPoll(Connection, Buffer.GetData(), static_cast<size_t>(Buffer.Num()));
				if (Length == 0)
				{
					bDrained = true;
					break;
				}
				if (Length < 0)
				{
					// The frame stays queued until a buffer that fits asks again.
					Buffer.SetNumUninitialized(static_cast<int32>(-Length));
					continue;
				}

				const FUTF8ToTCHAR Text(reinterpret_cast<const ANSICHAR*>(Buffer.GetData()), static_cast<int32>(Length));
				const FString Message(Text.Length(), Text.Get());
				if (OnMessage)
					OnMessage(Message);
			}

			// Frames the core queued before it closed are delivered first.
			if (bDrained && Connection != nullptr && !bReportedClose && Library->ConnectionIsClosed(Connection) != 0)
			{
				bReportedClose = true;
				if (OnClosed)
					OnClosed(1000, TEXT("the embedded core closed the connection"), true);
			}
		}

	private:
		FTclEmbeddedOptions Options;
		const FTclCoreLibrary* Library = nullptr;
		TclConnection* Connection = nullptr;
		TArray<uint8> Buffer;
		FString PendingError;
		bool bPendingConnected = false;
		bool bReportedClose = false;
	};
}

TSharedRef<ITclEngineSocket>
ITclEngineSocket::MakeEmbedded(const FTclEmbeddedOptions& Options)
{
	return MakeShared<FTclEmbeddedEngineSocket>(Options);
}
