// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle.h"
#include "Base/Tentacle_Stream.h"
#include "Base/Tentacle_Twitch.h"
#include "Base/Tentacle_Kick.h"
#include "Base/Tentacle_Obs.h"
#include "Base/Tentacle_ViewerProperties.h"
#include "Base/Tentacle_CustomEvents.h"
#include "Base/Tentacle_CustomControls.h"
#include "Base/Tentacle_Viewers.h"
#include "Base/Tentacle_Bot.h"
#include "Base/Tentacle_Timers.h"
#include "Base/Tentacle_Connections.h"
#include "Embedded/TclConnectionProcedures.h"
#include "Misc/CommandLine.h"
#include "TclSettings.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/TclViewerData.h"
#include "Sdk/TclMsg_ServerToClient.h"
#include "Sdk/TclMsg_ServerToClient_BotCommands.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsPrompt.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsResult.h"
#include "Sdk/TclMsg_ServerToClient_ConnectionsStatus.h"
#include "Sdk/TclMsg_ServerToClient_RpcResponse.h"
#include "Sdk/TclMsg_ServerToClient_CustomControlsInitialize.h"
#include "Sdk/TclMsg_ServerToClient_CustomControlsControlEvent.h"
#include "Sdk/TclMsg_ServerToClient_CustomEventsDefinitions.h"
#include "Sdk/TclMsg_ServerToClient_CustomEventsEvent.h"
#include "Sdk/TclMsg_ServerToClient_StreamChatJoin.h"
#include "Sdk/TclMsg_ServerToClient_StreamEvent.h"
#include "Sdk/TclMsg_ServerToClient_TimersDefinitions.h"
#include "Sdk/TclMsg_ServerToClient_TimersFired.h"
#include "Utilities/TclCommon.h"
#include "Utilities/TclConfigFiles.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
// TCondensedJsonPrintPolicy lives here, and JsonWriter.h does not pull it in on
// any engine version. Without this the file only compiles where something else
// in the PCH graph happens to include it - which is why UE 5.4+ built fine and
// 5.1 failed with C7568/C3203 (MSVC's C++20 mode reports an undeclared template
// as an "assumed function template" rather than an unknown identifier).
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	/**
	 * A message as condensed JSON. Tentacle sends the same text for the same
	 * content, so two snapshots are the same snapshot exactly when this is.
	 */
	FString TclSnapshotKey(const TSharedPtr<FJsonObject>& Json)
	{
		FString Text;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Text);
		FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);
		return Text;
	}
}

ATentacle::ATentacle()
{
	PrimaryActorTick.bCanEverTick = true;

	Twitch = CreateDefaultSubobject<UTentacle_Twitch>(TEXT("Twitch"));
	Kick = CreateDefaultSubobject<UTentacle_Kick>(TEXT("Kick"));
	Stream = CreateDefaultSubobject<UTentacle_Stream>(TEXT("Stream"));
	Obs =CreateDefaultSubobject<UTentacle_Obs>(TEXT("Obs"));
	ViewerProperties = CreateDefaultSubobject<UTentacle_ViewerProperties>(TEXT("ViewerProperties"));
	CustomEvents = CreateDefaultSubobject<UTentacle_CustomEvents>(TEXT("CustomEvents"));
	CustomControls = CreateDefaultSubobject<UTentacle_CustomControls>(TEXT("CustomControls"));
	Viewers = CreateDefaultSubobject<UTentacle_Viewers>(TEXT("Viewers"));
	Bot = CreateDefaultSubobject<UTentacle_Bot>(TEXT("Bot"));
	Timers = CreateDefaultSubobject<UTentacle_Timers>(TEXT("Timers"));
	Connections = CreateDefaultSubobject<UTentacle_Connections>(TEXT("Connections"));
}

void
ATentacle::BeginPlay()
{
	Super::BeginPlay();

	// Load settings
	Settings = UTclSettings::Load();

	// Where Tentacle is. It is a local app on well-known loopback ports, but it
	// publishes the ports it actually bound in a discovery file, which wins over
	// the defaults so a fallback port on the app side still connects.
	Endpoints = FTclSidecarEndpoints::Resolve(DataDirectoryOverride);
	if (IsEmbedded())
	{
		tclInfo("Tentacle: embedded in the game");
	}
	else
	{
		tclInfoF("Tentacle: sidecar at %s / %s (%s)",
			*Endpoints.HttpBaseUrl, *Endpoints.EngineSocketUrl,
			Endpoints.bFromDiscoveryFile ? TEXT("from sidecar.json") : TEXT("defaults"));
	}

	// Init HTTP client. No access token: Tentacle trusts loopback callers, and
	// asks them for its connection password only once its user sets one.
	HttpClient = NewObject<UTentacleHttpClient>(this);
	HttpClient->Configure(Endpoints.HttpBaseUrl, TEXT(""));
	HttpClient->SetConnectionPassword(ConnectionPassword);
	RouteProcedures(IsEmbedded());

	// Initialize subsystems
	InitializeSubsystems();
	PostInitializeSubsystems();

	// Dial in to Tentacle's engine listener, unless the game wants to decide
	// when that happens.
	if (bAutoConnect)
	{
		StartEngineConnection();
	}

	bIsReady = true;
	tclInfo("Tentacle: Ready");
}

void
ATentacle::Connect()
{
	if (Connection.IsValid())
		return;

	// A callback that runs as the actor ends (a failed procedure's completion,
	// say) would otherwise open a session nothing ticks or closes any more.
	if (bEndedPlay)
	{
		tclWarn("Tentacle: not connecting; this Tentacle actor has ended play");
		return;
	}

	StartEngineConnection();
}

void
ATentacle::Disconnect()
{
	if (!Connection.IsValid())
		return;

	Connection->Stop();
	Connection.Reset();

	// Stop() reports the close only when a session was actually open, so
	// disconnecting mid-reconnect would otherwise leave the status where it
	// was. Setting it here covers that; SetConnectionStatus ignores a no-op.
	SessionId.Empty();
	SetConnectionStatus(ETclConnectionStatus::Disconnected);
}

void
ATentacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Connection.IsValid())
		Connection->Tick(DeltaTime);
}

void
ATentacle::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndedPlay = true;
	bIsReady = false;

	if (Connection.IsValid())
		Connection->Stop();

	Connection.Reset();

	if (Procedures.IsValid())
	{
		Procedures->FailAll(TEXT("Tentacle stopped"));
		Procedures.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void
ATentacle::BeginDestroy()
{
	// An actor can be destroyed without EndPlay (its world torn down, then
	// garbage collected). Its connection would then stop in the destructor,
	// after the members its callbacks use, and report the closed session into
	// a half-destroyed actor. Stopped here instead, with nothing bound to call
	// back: nothing in the game should hear from an actor being destroyed.
	if (Connection.IsValid())
	{
		Connection->OnMessageReceived.Unbind();
		Connection->OnSessionOpened.Unbind();
		Connection->OnSessionClosed.Unbind();
		Connection->Stop();
		Connection.Reset();
	}
	Procedures.Reset();

	Super::BeginDestroy();
}

void
ATentacle::InitializeSubsystems()
{
	// Subsystems can override Initialize() for setup that doesn't depend on other subsystems
}

void
ATentacle::PostInitializeSubsystems()
{
	ViewerProperties->PostInitialize();
}

void
ATentacle::RouteProcedures(bool bEmbedded)
{
	if (Procedures.IsValid())
	{
		Procedures->FailAll(TEXT("Tentacle changed where it connects"));
		Procedures.Reset();
	}

	if (HttpClient == nullptr)
		return;

	if (!bEmbedded)
	{
		HttpClient->SetProcedureTransport(nullptr);
		return;
	}

	// The embedded core has no HTTP server: its procedures travel over the
	// connection, so everything that calls the HTTP client works unchanged.
	Procedures = MakeShared<FTclConnectionProcedures>([this](const FString& Frame)
	{
		return Connection.IsValid() && Connection->Send(Frame);
	});
	HttpClient->SetProcedureTransport(
		[WeakProcedures = TWeakPtr<FTclConnectionProcedures>(Procedures)](
			const FString& Procedure, bool bIsQuery, const FString& InputJson,
			const UTentacleHttpClient::FOnSuccess& OnSuccess, const UTentacleHttpClient::FOnError& OnError)
		{
			if (const TSharedPtr<FTclConnectionProcedures> Carrier = WeakProcedures.Pin())
				Carrier->Call(Procedure, bIsQuery, InputJson, OnSuccess, OnError);
			else
				OnError.ExecuteIfBound(0, TEXT("Tentacle is not connected"));
		});
}

void
ATentacle::StartEngineConnection()
{
	// The modes are read here rather than once at BeginPlay, so a game that
	// changes one before calling Connect moves the session and the procedures
	// together.
	const bool bEmbedded = IsEmbedded();
	RouteProcedures(bEmbedded);

	// Read here too, so a password the game sets before calling Connect is the
	// one its calls send.
	if (HttpClient != nullptr)
		HttpClient->SetConnectionPassword(ConnectionPassword);

	Connection = MakeShared<FTclEngineConnection>();

	// The actor owns the connection and stops it in EndPlay, so `this` outlives
	// every callback these can fire.
	Connection->OnMessageReceived.BindLambda([this](const FString& Msg)
	{
		HandleWebSocketMessage(Msg);
	});

	Connection->OnSessionOpened.BindLambda([this](const FString& NewSessionId)
	{
		SessionId = NewSessionId;
		SetConnectionStatus(ETclConnectionStatus::Connected);

		// The registry may have changed while no session was open, or never
		// loaded: a game started before Tentacle, or the embedded core, whose
		// procedures only travel once a session is open.
		ViewerProperties->ReloadRegistry(nullptr);
	});

	Connection->OnSessionClosed.BindLambda([this]()
	{
		SessionId.Empty();
		SetConnectionStatus(ETclConnectionStatus::Disconnected);

		if (Procedures.IsValid())
			Procedures->FailAll(TEXT("the session with Tentacle closed"));
	});

	if (bEmbedded)
		Connection->StartEmbedded(BuildEmbeddedOptions(), BuildIdentity());
	else
		Connection->Start(Endpoints.EngineSocketUrl, BuildIdentity());
}

FTclEmbeddedOptions
ATentacle::BuildEmbeddedOptions() const
{
	FTclEmbeddedOptions Options;
	Options.DataDirectory = FPaths::ConvertRelativePathToFull(EmbeddedDataDirectory.IsEmpty()
		? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Tentacle"))
		: EmbeddedDataDirectory);
	IFileManager::Get().MakeDirectory(*Options.DataDirectory, true);

	Options.TwitchClientId = TwitchClientId;
	Options.ConfigFiles = FTclConfigFiles::Read(FTclConfigFiles::Resolve(ConfigDirectory));
	return Options;
}

FTclEngineIdentity
ATentacle::BuildIdentity() const
{
	FTclEngineIdentity Identity;
	Identity.Engine = TEXT("unreal");
	Identity.EngineVersion = FEngineVersion::Current().ToString(EVersionComponent::Patch);

	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Tentacle")))
		Identity.PluginVersion = Plugin->GetDescriptor().VersionName;

	// Unique per play session, which is what keeps an editor PIE session and a
	// packaged build of the same project apart in Tentacle.
	Identity.InstanceId = FGuid::NewGuid().ToString(EGuidFormats::Digits);

	Identity.Label = SessionLabel.IsEmpty()
		? FString(FApp::GetProjectName()) + (GIsEditor ? TEXT(" (Editor)") : TEXT(""))
		: SessionLabel;

	Identity.Password = ConnectionPassword;

	return Identity;
}

bool
ATentacle::IsEmbedded() const
{
	return TclTentacleMode::ResolvesToEmbedded(EditorMode, BuildMode, GIsEditor, FCommandLine::Get());
}

bool
ATentacle::IsConnected() const
{
	return ConnectionStatus == ETclConnectionStatus::Connected;
}

void
ATentacle::SetConnectionStatus(ETclConnectionStatus NewStatus)
{
	if (ConnectionStatus == NewStatus)
		return;

	ConnectionStatus = NewStatus;
	OnConnectionStatusChanged.Broadcast(ConnectionStatus);
}

void
ATentacle::HandleWebSocketMessage(const FString& JsonMessage)
{
	TSharedPtr<FJsonObject> JsonObj;
	const auto Reader = TJsonReaderFactory<>::Create(JsonMessage);

	if (!FJsonSerializer::Deserialize(Reader, JsonObj) || !JsonObj.IsValid())
	{
		const FString Preview = JsonMessage.Left(200);
		tclWarnF("Tentacle: Failed to parse WebSocket message as JSON: %s", *Preview);
		return;
	}

	FString MsgType;
	if (JsonObj->TryGetStringField(TEXT("type"), MsgType))
	{
		tclDebugF("Tentacle: Received message type: %s", *MsgType);
	}

	HandleMsg(JsonObj);
}

void
ATentacle::HandleMsg(const TSharedPtr<FJsonObject>& MsgJson)
{
	FString MsgType;
	if (!MsgJson->TryGetStringField(TEXT("type"), MsgType))
	{
		tclWarn("Tentacle: Message missing 'type' field");
		return;
	}

	auto* Msg = UTclMsg_ServerToClient::Parse(this, MsgJson);
	if (!Msg)
	{
		tclWarnF("Tentacle: Failed to parse message of type '%s'", *MsgType);
		return;
	}

	if (auto* RpcResponse = Cast<UTclMsg_ServerToClient_RpcResponse>(Msg))
	{
		// Only Embedded mode makes calls over the connection; anything else
		// answering one is ignored.
		if (Procedures.IsValid())
			Procedures->HandleResponse(RpcResponse->Id, RpcResponse->Status, RpcResponse->Body);
	}
	else if (auto* ConnectionsStatus = Cast<UTclMsg_ServerToClient_ConnectionsStatus>(Msg))
	{
		Connections->HandleMsg_Status(ConnectionsStatus);
	}
	else if (auto* ConnectionsPrompt = Cast<UTclMsg_ServerToClient_ConnectionsPrompt>(Msg))
	{
		Connections->HandleMsg_Prompt(ConnectionsPrompt);
	}
	else if (auto* ConnectionsResult = Cast<UTclMsg_ServerToClient_ConnectionsResult>(Msg))
	{
		Connections->HandleMsg_Result(ConnectionsResult);
	}
	else if (auto* ControlEvent = Cast<UTclMsg_ServerToClient_CustomControlsControlEvent>(Msg))
	{
		CustomControls->HandleMsg_ControlEvent(ControlEvent);
	}
	else if (auto* ControlInit = Cast<UTclMsg_ServerToClient_CustomControlsInitialize>(Msg))
	{
		CustomControls->HandleMsg_Initialize(ControlInit);
	}
	else if (auto* CustomEvent = Cast<UTclMsg_ServerToClient_CustomEventsEvent>(Msg))
	{
		CustomEvents->HandleMsg_Event(CustomEvent);
	}
	else if (auto* Definitions = Cast<UTclMsg_ServerToClient_CustomEventsDefinitions>(Msg))
	{
		CustomEvents->HandleMsg_Definitions(Definitions, TclSnapshotKey(MsgJson));
	}
	else if (auto* BotCommands = Cast<UTclMsg_ServerToClient_BotCommands>(Msg))
	{
		Bot->HandleMsg_Commands(BotCommands, TclSnapshotKey(MsgJson));
	}
	else if (auto* TimerFired = Cast<UTclMsg_ServerToClient_TimersFired>(Msg))
	{
		Timers->HandleMsg_Fired(TimerFired);
	}
	else if (auto* TimerDefinitions = Cast<UTclMsg_ServerToClient_TimersDefinitions>(Msg))
	{
		Timers->HandleMsg_Definitions(TimerDefinitions, TclSnapshotKey(MsgJson));
	}
	else if (auto* ChatJoin = Cast<UTclMsg_ServerToClient_StreamChatJoin>(Msg))
	{
		HandleChatJoin(ChatJoin);
	}
	else if (auto* StreamEvent = Cast<UTclMsg_ServerToClient_StreamEvent>(Msg))
	{
		HandleStreamEvent(StreamEvent);
	}
	else
	{
		tclDebugF("Tentacle: Unhandled message type '%s'", *MsgType);
	}
}

// --- Stream ---

void
ATentacle::HandleStreamEvent(UTclMsg_ServerToClient_StreamEvent* Msg)
{
	tclSoftAssert(Msg && Msg->Event);

	UTclStreamEventBase* Event = Msg->Event;

	// A duplicate never reaches a handler, on any surface.
	if (!StreamEventDedup.TryAdd(Event->Id))
		return;

	// Upserted first, so every surface hands over the same cached viewer.
	UTclViewer* Viewer = Msg->Viewer ? UpsertViewer(Msg->Viewer) : nullptr;

	OnStreamEvent.Broadcast(Event, Viewer);

	// The platform component fires the kind's own delegate and this actor's
	// cross-platform view after it.
	if (auto* TwitchEvent = Cast<UTclStreamEvent_Twitch_Base>(Event))
	{
		if (Twitch)
			Twitch->DispatchEvent(TwitchEvent, Viewer);
	}
	else if (auto* KickEvent = Cast<UTclStreamEvent_Kick_Base>(Event))
	{
		if (Kick)
			Kick->DispatchEvent(KickEvent, Viewer);
	}

	// The deprecated Stream component fires last, in its original order.
	if (Stream)
		Stream->DispatchDeprecatedEvent(Event, Viewer);
}

void
ATentacle::HandleChatJoin(UTclMsg_ServerToClient_StreamChatJoin* Msg)
{
	tclSoftAssert(Msg);

	UTclViewer* Viewer = Msg->Viewer ? UpsertViewer(Msg->Viewer) : nullptr;

	// A join is Tentacle's own event, not a platform's, so it fires here alone;
	// the viewer's data says which platform they joined on.
	if (Msg->Viewer)
		OnChatJoin.Broadcast(Viewer);

	if (Stream)
		Stream->DispatchDeprecatedChatJoin(Msg->Viewer, Viewer);
}

void
ATentacle::DispatchChatMessage(UTclStreamEvent_Anywhere_ChatMessage* ChatMsg, UTclViewer* Viewer)
{
	tclSoftAssert(ChatMsg);

	if (!ChatMsg->Command.IsEmpty())
		OnChatCommand.Broadcast(ChatMsg, Viewer);
	else
		OnChatMessage.Broadcast(ChatMsg, Viewer);

	if (!ChatMsg->Emojis.IsEmpty())
		OnChatMessageWithEmojis.Broadcast(ChatMsg, Viewer);

	if (!ChatMsg->Emotes.IsEmpty())
		OnChatMessageWithEmotes.Broadcast(ChatMsg, Viewer);
}

// --- Chat send ---

namespace
{
	/**
	 * The wire value for a send target, or an empty string when every connected
	 * platform is targeted (the field is then omitted from the message).
	 */
	FString
	ChatSendTargetToWire(ETclChatSendTarget Target)
	{
		switch (Target)
		{
		case ETclChatSendTarget::Twitch: return TEXT("twitch");
		case ETclChatSendTarget::Kick:   return TEXT("kick");
		default:                         return FString();
		}
	}
}

int32
ATentacle::GetChatMessageMaxLength()
{
	return ChatMessageMaxLength;
}

void
ATentacle::SendChatMessage(const FString& Message, bool bIsBot)
{
	SendChatMessageToTarget(Message, ETclChatSendTarget::AllConnected, bIsBot);
}

void
ATentacle::SendChatMessageToViewer(UTclViewer* Viewer, const FString& Message, bool bIsBot)
{
	SendChatMessageToViewerOnTarget(Viewer, Message, ETclChatSendTarget::AllConnected, bIsBot);
}

void
ATentacle::SendChatMessageToTarget(const FString& Message, ETclChatSendTarget Target, bool bIsBot)
{
	SendChatMessageInternal(Message, Target, bIsBot, FString());
}

void
ATentacle::SendChatMessageToViewerOnTarget(UTclViewer* Viewer, const FString& Message, ETclChatSendTarget Target, bool bIsBot)
{
	if (Viewer == nullptr)
	{
		tclWarn("Tentacle: Send Chat Message To Viewer needs a viewer; nothing sent.");
		return;
	}

	SendChatMessageInternal(Message, Target, bIsBot, Viewer->GetViewerId());
}

FString
ATentacle::BuildChatMessagePayload(const FString& Message, ETclChatSendTarget Target, bool bIsBot, const FString& ViewerId)
{
	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetStringField(TEXT("message"), Message);
	DataObj->SetBoolField(TEXT("isBot"), bIsBot);

	// Absent fields carry meaning on the wire: no platform means every connected
	// platform, and no viewerId means the message mentions nobody.
	const FString Platform = ChatSendTargetToWire(Target);
	if (!Platform.IsEmpty())
	{
		DataObj->SetStringField(TEXT("platform"), Platform);
	}
	if (!ViewerId.IsEmpty())
	{
		DataObj->SetStringField(TEXT("viewerId"), ViewerId);
	}

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("Stream_SendChatMessage"));
	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);

	return JsonString;
}

void
ATentacle::SendChatMessageInternal(const FString& Message, ETclChatSendTarget Target, bool bIsBot, const FString& ViewerId)
{
	// Validate here rather than letting Tentacle reject the message: a warning
	// in the Unreal log is visible to whoever wrote the Blueprint, a rejection
	// on the far side of the socket is not.
	if (Message.IsEmpty())
	{
		tclWarn("Tentacle: Send Chat Message needs a message; nothing sent.");
		return;
	}
	if (Message.Len() > ChatMessageMaxLength)
	{
		tclWarnF("Tentacle: Send Chat Message is %d characters, over the %d limit; nothing sent.", Message.Len(), ChatMessageMaxLength);
		return;
	}

	SendMsg(BuildChatMessagePayload(Message, Target, bIsBot, ViewerId));
}

void
ATentacle::SendMsg(const FString& JsonString)
{
	OnSendMsg.Broadcast(JsonString);

	if (Connection.IsValid())
		Connection->Send(JsonString);
}

// --- Viewer Management ---
//
// The cache lives on the Viewers component; these forward to it so the actor's
// Blueprint nodes and C++ entry points keep working exactly as they always have.

UTclViewer*
ATentacle::UpsertViewer(const TSharedPtr<FJsonObject>& ViewerJson)
{
	return Viewers ? Viewers->UpsertViewer(ViewerJson) : nullptr;
}

UTclViewer*
ATentacle::UpsertViewer(UTclViewerData* ViewerData)
{
	return Viewers ? Viewers->UpsertViewer(ViewerData) : nullptr;
}

UTclViewer*
ATentacle::FindViewerById(const FString& ViewerId, bool& Found) const
{
	if (!Viewers)
	{
		Found = false;
		return nullptr;
	}

	return Viewers->FindViewerById(ViewerId, Found);
}

void
ATentacle::DestroyViewer(const FString& ViewerId)
{
	if (Viewers)
		Viewers->DestroyViewer(ViewerId);
}

void
ATentacle::DestroyAllViewers()
{
	if (Viewers)
		Viewers->DestroyAllViewers();
}

void
ATentacle::RemergeAllViewerProperties()
{
	if (!Viewers)
		return;

	for (UTclViewer* Viewer : Viewers->GetAllViewers())
	{
		if (Viewer->Data)
			Viewer->MergePropertiesFromData(Viewer->Data);
	}
}

void
ATentacle::ResetAllViewerPropertiesToDefaults(const FString& CollectionId)
{
	if (!Viewers)
		return;

	for (UTclViewer* Viewer : Viewers->GetAllViewers())
		Viewer->ResetPropertiesToDefaultsForCollection(CollectionId);
}

void
ATentacle::ResetAllViewerPropertiesToDefaultForDefinition(const FString& DefinitionId)
{
	if (!Viewers)
		return;

	for (UTclViewer* Viewer : Viewers->GetAllViewers())
		Viewer->ResetPropertyToDefaultForDefinition(DefinitionId);
}
