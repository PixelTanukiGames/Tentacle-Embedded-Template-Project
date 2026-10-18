// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utilities/TclConnectionStatus.h"
#include "Utilities/TclEmbeddedOptions.h"
#include "Utilities/TclEngineConnection.h"
#include "Utilities/TclSidecarEndpoints.h"
#include "Utilities/TclTentacleMode.h"
#include "Utilities/TclDeduplicator.h"
#include "Enums/TclChatSendTarget.h"
#include "Base/TclViewer.h"
#include "Sdk/TclStreamEventBase.h"
#include "Sdk/TclStreamEvent_Anywhere_Ban.h"
#include "Sdk/TclStreamEvent_Anywhere_ChannelUpdate.h"
#include "Sdk/TclStreamEvent_Anywhere_ChatMessage.h"
#include "Sdk/TclStreamEvent_Anywhere_Donation.h"
#include "Sdk/TclStreamEvent_Anywhere_Follow.h"
#include "Sdk/TclStreamEvent_Anywhere_Redemption.h"
#include "Sdk/TclStreamEvent_Anywhere_Subscription.h"
#include "Sdk/TclStreamEvent_Anywhere_SubscriptionGift.h"
#include "Sdk/TclStreamEvent_Anywhere_SubscriptionRenewal.h"
#include "Tentacle.generated.h"

class UTentacle_Stream;
class UTentacle_Twitch;
class UTentacle_Kick;
class UTentacle_Obs;
class UTentacle_ViewerProperties;
class UTentacle_CustomEvents;
class UTentacle_CustomControls;
class UTentacle_Viewers;
class UTentacle_Bot;
class UTentacle_Timers;
class UTentacle_Connections;
class UTclSettings;
class FTclConnectionProcedures;
class UTentacleHttpClient;
class UTclMsg_ServerToClient_StreamChatJoin;
class UTclMsg_ServerToClient_StreamEvent;

/**
 * Core Tentacle actor. Owns all subsystem components, manages viewer state,
 * holds the connection to the Tentacle app, and routes messages.
 *
 * Stream events and chat that work across platforms live on the actor itself
 * (On Follow, On Chat Message, Send Chat Message); each platform's own live on
 * its Twitch and Kick components.
 *
 * Tentacle hosts the engine WebSocket listener and this actor dials in, so
 * several games (an editor PIE session next to a packaged build) can be
 * connected at once and the game never opens a listening socket.
 */
UCLASS()
class TENTACLE_API ATentacle : public AActor
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewerAdded, const UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConnectionStatusChanged, ETclConnectionStatus, ConnectionStatus);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSendMsg, const FString& /*JsonString*/);

	// Stream, across platforms. Events that identify a viewer pass that viewer
	// alongside the event; events that describe the channel pass the event only.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStreamEvent, const UTclStreamEventBase*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatJoin, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatCommand, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessage, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmojis, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmotes, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBan, const UTclStreamEvent_Anywhere_Ban*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelUpdate, const UTclStreamEvent_Anywhere_ChannelUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDonation, const UTclStreamEvent_Anywhere_Donation*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFollow, const UTclStreamEvent_Anywhere_Follow*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRedemption, const UTclStreamEvent_Anywhere_Redemption*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscription, const UTclStreamEvent_Anywhere_Subscription*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionGift, const UTclStreamEvent_Anywhere_SubscriptionGift*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionRenewal, const UTclStreamEvent_Anywhere_SubscriptionRenewal*, Event, UTclViewer*, Viewer);

public:
	ATentacle();

	// --- Stream, across platforms ---

	/**
	 * Fires for every stream event, whatever its platform or kind, including
	 * kinds this version of the plugin has no delegate of its own for: the one
	 * to use for doing your own processing. Cast Event to the event class you
	 * want to handle; its class says which platform it came from.
	 *
	 * For one event this fires first, then the event's own delegate on its
	 * platform's component (for example On Channel Follow on the Twitch
	 * component), then its cross-platform view here (On Follow). Viewer is the
	 * viewer the event is about, or null for an event about the channel itself.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnStreamEvent OnStreamEvent;

	/**
	 * Fires when a viewer joins chat. Only Twitch reports joins: Tentacle checks who
	 * is in the Twitch chat every 30 seconds, so a join arrives up to 30 seconds after
	 * it happens, and viewers already in chat when Tentacle connects to Twitch raise none.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnChatJoin OnChatJoin;

	/** A chat message, on any platform, that Tentacle classified as a command. A command does not also fire On Chat Message. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnChatCommand OnChatCommand;

	/** A chat message, on any platform, that is not a command. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnChatMessage OnChatMessage;

	/** A chat message or command carrying emojis, fired in addition to On Chat Message or On Chat Command. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnChatMessageWithEmojis OnChatMessageWithEmojis;

	/** A chat message or command carrying emotes, fired in addition to On Chat Message or On Chat Command. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnChatMessageWithEmotes OnChatMessageWithEmotes;

	/** A viewer was banned, on any platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnBan OnBan;

	/** The channel's title or category changed, on any platform. Describes the channel, so it carries no viewer. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnChannelUpdate OnChannelUpdate;

	/** A viewer donated, on any platform (bits on Twitch). */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnDonation OnDonation;

	/** A viewer followed, on any platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnFollow OnFollow;

	/** A viewer redeemed a channel reward, on any platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnRedemption OnRedemption;

	/** A viewer subscribed for the first time, on any platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnSubscription OnSubscription;

	/** A viewer gifted subscriptions, on any platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnSubscriptionGift OnSubscriptionGift;

	/** A viewer renewed a subscription, on any platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Stream")
	FOnSubscriptionRenewal OnSubscriptionRenewal;

	/**
	 * The longest message Send Chat Message and Send Chat Message To Viewer
	 * accept, in characters. Twitch and Kick both cap a chat message at 500.
	 */
	static constexpr int32 ChatMessageMaxLength = 500;

	/**
	 * The longest message the Send Chat Message nodes accept, in characters
	 * (500). A longer message is not sent.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Stream")
	static int32 GetChatMessageMaxLength();

	/**
	 * Send a chat message to every connected platform. To send on one platform
	 * alone, with that platform's own options, use Send Chat Message on the
	 * Twitch component or Post Chat Message on the Kick component instead.
	 *
	 * Fire-and-forget: the message is handed to Tentacle, which sends it on
	 * every platform that is connected. Nothing is reported back, so a send
	 * that fails app-side (no account connected, platform outage) surfaces in
	 * Tentacle's log rather than in the game.
	 *
	 * @param Message The chat text. 1-500 characters; anything else is refused
	 *                with a warning in the log instead of being sent.
	 * @param bIsBot  Send as the bot identity where the platform supports one
	 *                (Kick). Twitch has no separate bot identity yet, so the
	 *                message comes from the broadcaster account there.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Stream", meta=(AdvancedDisplay="bIsBot"))
	void SendChatMessage(const FString& Message, bool bIsBot = false);

	/**
	 * Send a chat message that mentions a specific viewer ("@name message") on
	 * every connected platform.
	 *
	 * The mention uses the viewer's name on each platform, so the message only
	 * reaches platforms where the viewer has an identity.
	 *
	 * @param Viewer  The viewer to mention. Required.
	 * @param Message The chat text, before the mention is prepended. 1-500
	 *                characters.
	 * @param bIsBot  Send as the bot identity where the platform supports one.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Stream", meta=(AdvancedDisplay="bIsBot"))
	void SendChatMessageToViewer(UTclViewer* Viewer, const FString& Message, bool bIsBot = false);

	/**
	 * Serialize a Stream_SendChatMessage message for the given arguments.
	 *
	 * Split out from the send path so the wire contract can be asserted without
	 * a live WebSocket connection. Performs no validation; the send nodes check
	 * the message before calling this. An empty ViewerId is untargeted, and
	 * ETclChatSendTarget::AllConnected omits the platform field entirely.
	 */
	static FString BuildChatMessagePayload(
		const FString& Message,
		ETclChatSendTarget Target,
		bool bIsBot,
		const FString& ViewerId);

	// The deprecated Stream component fires last, after the surfaces below; the
	// reference leaves it out, since the docs describe only the current surface.
	/**
	 * Handle one Stream_Event message: de-duplicate it, upsert its viewer, and
	 * fire every stream surface - this actor's catch-all, the platform
	 * component's delegate for the kind, and this actor's cross-platform view,
	 * in that order. C++ only.
	 */
	void HandleStreamEvent(UTclMsg_ServerToClient_StreamEvent* Msg);

	/** Handle one Stream_ChatJoin message the same way. C++ only. */
	void HandleChatJoin(UTclMsg_ServerToClient_StreamChatJoin* Msg);

	/**
	 * Fire this actor's chat delegates for one chat message: the message or
	 * command, then the emoji and emote events. Called by the platform
	 * components after their own chat delegates. C++ only.
	 */
	void DispatchChatMessage(UTclStreamEvent_Anywhere_ChatMessage* ChatMsg, UTclViewer* Viewer);

	// --- Delegates ---

	/**
	 * Fires once per viewer, the first time Tentacle tells this session about
	 * them; never when a known viewer's data is refreshed. The Viewers
	 * component's On Viewer Added fires right after this one.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle")
	FOnViewerAdded
	OnViewerAdded;

	UPROPERTY(BlueprintAssignable, Category="Tentacle")
	FOnConnectionStatusChanged
	OnConnectionStatusChanged;

	/**
	 * Fires with every message this actor hands to Tentacle, as it is handed
	 * over and whether or not a session is open to carry it. C++ only; for
	 * logging or inspecting what a game sends.
	 */
	FOnSendMsg OnSendMsg;

	// --- Subsystem components ---

	/** Twitch on its own: every Twitch event, one delegate per kind, and the calls that address Twitch alone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Twitch* Twitch;

	/** Kick on its own: every Kick event, one delegate per kind, and the calls that address Kick alone. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Kick* Kick;

	/**
	 * Deprecated. Cross-platform stream events and chat now live on the Tentacle
	 * actor itself, and each platform's own on its Twitch and Kick components.
	 * Everything here still fires and works, after the new ones.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle", meta=(DeprecatedProperty, DeprecationMessage="Use the Tentacle actor for cross-platform stream events and chat, and its Twitch and Kick components for each platform's own."))
	UTentacle_Stream* Stream;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Obs* Obs;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_ViewerProperties* ViewerProperties;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_CustomEvents* CustomEvents;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_CustomControls* CustomControls;

	/**
	 * The viewer cache: every viewer Tentacle has told this session about, one
	 * object per viewer for the whole session.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Viewers* Viewers;

	/** Tentacle's chat bot as the streamer configured it, with every command it answers, kept current. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Bot* Bot;

	/** The timers the streamer defined in Tentacle, firing at the game, and their definitions, kept current. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Timers* Timers;

	/**
	 * Signing the player in to Twitch and Kick from the game, and each
	 * platform's connection. Only answered when Tentacle runs Embedded; the
	 * Tentacle app has its own sign-in.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle")
	UTentacle_Connections* Connections;

	// --- Settings & HTTP ---

	UPROPERTY()
	UTclSettings* Settings;

	UPROPERTY()
	UTentacleHttpClient* HttpClient;

	// --- Session ---

	/**
	 * Connect as soon as the actor begins play. Turn it off to call Connect()
	 * yourself - for a game that only wants Tentacle during a particular mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	bool bAutoConnect = true;

	/**
	 * Name shown for this game in Tentacle's engine list. Empty means the
	 * project name, with "(Editor)" appended when running inside the editor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	FString SessionLabel;

	/**
	 * Tentacle's connection password, for a Tentacle whose user set one under
	 * Settings > System > Connection Password. Empty, the default, sends none,
	 * which is all Tentacle asks for until a password is set. A game can set it
	 * from its own settings screen before calling Connect. Not used in Embedded
	 * mode.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle", meta=(PasswordField=true))
	FString ConnectionPassword;

	/**
	 * Tentacle data folder to read the sidecar discovery file from, for running
	 * against an isolated dev instance. Empty means Tentacle's default folder
	 * (the Tentacle folder under %APPDATA% on Windows).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle", AdvancedDisplay)
	FString DataDirectoryOverride;

	/**
	 * Where Tentacle runs while you Play In Editor: Paired, with the Tentacle
	 * app on this machine, or Embedded, inside the game itself. A -tentacle-mode=paired or
	 * -tentacle-mode=embedded argument on the command line wins over it. Read
	 * each time the actor connects.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	ETclTentacleMode EditorMode = ETclTentacleMode::Paired;

	/**
	 * Where Tentacle runs in a packaged game, or one run standalone: Paired,
	 * with the Tentacle app on the player's machine, or Embedded, inside the
	 * game itself, so players need no app. A -tentacle-mode=paired or -tentacle-mode=embedded
	 * argument on the command line wins over it. Read each time the actor
	 * connects.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle")
	ETclTentacleMode BuildMode = ETclTentacleMode::Paired;

	/**
	 * Embedded mode: the folder Tentacle keeps its database, the player's
	 * signed-in accounts, and its log in. Empty means a Tentacle folder in the
	 * project's Saved folder, which a Shipping build keeps per player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Embedded")
	FString EmbeddedDataDirectory;

	/**
	 * Embedded mode: the client ID of your own Twitch app, to sign the player in
	 * with instead of Tentacle's, for your game's name on Twitch's consent page.
	 * Empty uses Tentacle's.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Embedded")
	FString TwitchClientId;

	// The default is ENGINE_CONFIG_FOLDERS.unreal in
	// packages/common/src/general/config-export.ts, where the Tentacle app
	// exports to, and the folder Tentacle.Build.cs stages; the app's
	// config-folders test holds the three together.
	/**
	 * Embedded mode: the folder Tentacle reads the game's config files from, the
	 * timers, custom events, viewer properties, and bot commands the Tentacle
	 * app's Data > Export writes there. Relative to the project folder
	 * unless it is a full path. Every .json file in it is applied each time
	 * Tentacle starts: what the files name is brought in line with them, what
	 * they stop naming is removed (a viewer property is kept, with its values),
	 * and what the game creates itself is never touched. The default folder ships inside a packaged
	 * game with no setup. A folder of your own must be under Content and listed
	 * in Project Settings > Packaging > Additional Non-Asset Directories to
	 * Package.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tentacle|Embedded")
	FString ConfigDirectory = TEXT("Config/Tentacle");

	/** The id Tentacle assigned to this session. Empty while disconnected. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle")
	FString SessionId;

	// --- State ---

	UPROPERTY(BlueprintReadOnly, Category="Tentacle")
	ETclConnectionStatus ConnectionStatus = ETclConnectionStatus::Disconnected;

	UPROPERTY(BlueprintReadOnly, Category="Tentacle")
	bool bIsReady = false;

	/** True while a session with Tentacle is open: the same as Connection Status being Connected. */
	UFUNCTION(BlueprintPure, Category="Tentacle")
	bool
	IsConnected() const;

	/**
	 * True when Tentacle runs inside the game rather than in the Tentacle app:
	 * Editor Mode while playing in the editor, Build Mode otherwise, or a
	 * -tentacle-mode argument on the command line. Signing in from the game
	 * only applies when this is true.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle")
	bool
	IsEmbedded() const;

	// --- Viewer Management ---
	//
	// The viewer cache itself lives on the Viewers component. These stay on the
	// actor because graphs have always found them here; each one does exactly
	// what the component's member of the same name does.

	/**
	 * Upsert a viewer from JSON data. Creates or updates the viewer.
	 * Returns the viewer (or nullptr if viewer data is invalid).
	 */
	UTclViewer*
	UpsertViewer(const TSharedPtr<FJsonObject>& ViewerJson);

	/**
	 * Upsert a viewer from already-parsed data. Creates or updates the viewer.
	 * Preferred over the JSON overload when parsed data is already available (preserves platform info).
	 */
	UTclViewer*
	UpsertViewer(UTclViewerData* ViewerData);

	/** Find a viewer by ID. The same as Find Viewer By Id on the Viewers component. */
	UFUNCTION(BlueprintPure, Category="Tentacle")
	UTclViewer*
	FindViewerById(const FString& ViewerId, bool& Found) const;

	/** Destroy a viewer. The same as Destroy Viewer on the Viewers component. */
	UFUNCTION(BlueprintCallable, Category="Tentacle")
	void
	DestroyViewer(const FString& ViewerId);

	/** Destroy all viewers. The same as Destroy All Viewers on the Viewers component. */
	UFUNCTION(BlueprintCallable, Category="Tentacle")
	void
	DestroyAllViewers();

	/**
	 * Re-run the property merge over every cached viewer using its last parsed
	 * data. Called when the viewer-property registry (collections + definitions)
	 * loads or refreshes, so viewers seen before the registry was cached resolve
	 * their property names/types. Idempotent and reconciliation-safe.
	 */
	void
	RemergeAllViewerProperties();

	/**
	 * Revert every cached viewer's property entries in the given collection to
	 * the definition defaults (see UTclViewer::ResetPropertiesToDefaultsForCollection).
	 * Called after a successful server-side collection reset so gameplay state
	 * reflects it immediately instead of waiting for each viewer's next snapshot.
	 */
	void
	ResetAllViewerPropertiesToDefaults(const FString& CollectionId);

	/**
	 * Revert every cached viewer's property entry for the given definition to
	 * the definition default (see UTclViewer::ResetPropertyToDefaultForDefinition).
	 * Called after a successful server-side definition reset so gameplay state
	 * reflects it immediately instead of waiting for each viewer's next snapshot.
	 */
	void
	ResetAllViewerPropertiesToDefaultForDefinition(const FString& DefinitionId);

	// --- Connection ---

	/**
	 * Start connecting, and keep retrying until Disconnect(). Idempotent, so a
	 * game that is unsure whether it already connected can just call it.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle")
	void
	Connect();

	/** Close the session and stop retrying. Safe to call while disconnected. */
	UFUNCTION(BlueprintCallable, Category="Tentacle")
	void
	Disconnect();

	/**
	 * Send a JSON message to Tentacle. Dropped while the session is not open;
	 * Tentacle asks for the custom controls again once it is.
	 */
	void
	SendMsg(const FString& JsonString);

	/** The endpoints this actor resolved at BeginPlay (discovery file or defaults). */
	const FTclSidecarEndpoints&
	GetEndpoints() const { return Endpoints; }

	/** The connection to Tentacle; null before BeginPlay and after EndPlay. */
	TSharedPtr<FTclEngineConnection>
	GetConnection() const { return Connection; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginDestroy() override;

private:
	/** Set by EndPlay, after which Connect opens nothing. */
	bool bEndedPlay = false;

	// The deprecated Stream component, whose sends still take a target, sends
	// through the targeted helpers below. The entry point's own sends always
	// target every connected platform, and the platform components send their
	// platform's own request instead. The cond block keeps the friend
	// declaration out of the generated reference page, which documents public
	// surface only.
	/// @cond TENTACLE_INTERNAL
	friend class UTentacle_Stream;
	/// @endcond

	FTclSidecarEndpoints Endpoints;
	TSharedPtr<FTclEngineConnection> Connection;

	/** Embedded mode: the HTTP client's calls, carried over the connection. Null in Paired mode. */
	TSharedPtr<FTclConnectionProcedures> Procedures;

	/** Keeps a handler from running twice for one stream event that arrives twice. Tentacle re-sends nothing after a reconnect, so this is a safeguard. */
	FTclDeduplicator StreamEventDedup{100};

	/** A chat send on one target, validated like every send node. */
	void SendChatMessageToTarget(const FString& Message, ETclChatSendTarget Target, bool bIsBot);

	/** A mention on one target; refuses a null viewer with a warning. */
	void SendChatMessageToViewerOnTarget(
		UTclViewer* Viewer,
		const FString& Message,
		ETclChatSendTarget Target,
		bool bIsBot);

	/** Shared body of every chat send node. An empty ViewerId is untargeted. */
	void SendChatMessageInternal(
		const FString& Message,
		ETclChatSendTarget Target,
		bool bIsBot,
		const FString& ViewerId);

	void SetConnectionStatus(ETclConnectionStatus NewStatus);
	void HandleWebSocketMessage(const FString& JsonMessage);
	void HandleMsg(const TSharedPtr<FJsonObject>& MsgJson);

	void InitializeSubsystems();
	void PostInitializeSubsystems();
	void StartEngineConnection();

	/**
	 * Routes the HTTP client's calls for a connection about to start: over the
	 * connection when embedded, to the app over HTTP otherwise. Fails whatever
	 * was waiting on the previous route.
	 */
	void RouteProcedures(bool bEmbedded);

	/** Who this process says it is in the hello handshake. */
	FTclEngineIdentity BuildIdentity() const;

	/** The embedded core's options, from the Embedded settings, with each definition file read. */
	FTclEmbeddedOptions BuildEmbeddedOptions() const;
};
