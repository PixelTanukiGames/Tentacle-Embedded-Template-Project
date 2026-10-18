// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sdk/TclStreamEvent_Twitch_Base.h"
#include "Sdk/TclStreamEvent_Twitch_AdBreakBegin.h"
#include "Sdk/TclStreamEvent_Twitch_AutomaticRewardRedemptionAdd.h"
#include "Sdk/TclStreamEvent_Twitch_Ban.h"
#include "Sdk/TclStreamEvent_Twitch_BitsUse.h"
#include "Sdk/TclStreamEvent_Twitch_ChannelUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_CharityCampaignProgress.h"
#include "Sdk/TclStreamEvent_Twitch_CharityCampaignStart.h"
#include "Sdk/TclStreamEvent_Twitch_CharityCampaignStop.h"
#include "Sdk/TclStreamEvent_Twitch_CharityDonation.h"
#include "Sdk/TclStreamEvent_Twitch_ChatClear.h"
#include "Sdk/TclStreamEvent_Twitch_ChatClearUserMessages.h"
#include "Sdk/TclStreamEvent_Twitch_ChatMessage.h"
#include "Sdk/TclStreamEvent_Twitch_ChatMessageDelete.h"
#include "Sdk/TclStreamEvent_Twitch_ChatSettingsUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_Cheer.h"
#include "Sdk/TclStreamEvent_Twitch_Follow.h"
#include "Sdk/TclStreamEvent_Twitch_GoalBegin.h"
#include "Sdk/TclStreamEvent_Twitch_GoalEnd.h"
#include "Sdk/TclStreamEvent_Twitch_GoalProgress.h"
#include "Sdk/TclStreamEvent_Twitch_HypeTrainBegin.h"
#include "Sdk/TclStreamEvent_Twitch_HypeTrainEnd.h"
#include "Sdk/TclStreamEvent_Twitch_HypeTrainProgress.h"
#include "Sdk/TclStreamEvent_Twitch_ModeratorAdd.h"
#include "Sdk/TclStreamEvent_Twitch_ModeratorRemove.h"
#include "Sdk/TclStreamEvent_Twitch_PollBegin.h"
#include "Sdk/TclStreamEvent_Twitch_PollEnd.h"
#include "Sdk/TclStreamEvent_Twitch_PollProgress.h"
#include "Sdk/TclStreamEvent_Twitch_PredictionBegin.h"
#include "Sdk/TclStreamEvent_Twitch_PredictionEnd.h"
#include "Sdk/TclStreamEvent_Twitch_PredictionLock.h"
#include "Sdk/TclStreamEvent_Twitch_PredictionProgress.h"
#include "Sdk/TclStreamEvent_Twitch_Raid.h"
#include "Sdk/TclStreamEvent_Twitch_RedemptionAdd.h"
#include "Sdk/TclStreamEvent_Twitch_RedemptionUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_RewardAdd.h"
#include "Sdk/TclStreamEvent_Twitch_RewardRemove.h"
#include "Sdk/TclStreamEvent_Twitch_RewardUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_SharedChatBegin.h"
#include "Sdk/TclStreamEvent_Twitch_SharedChatEnd.h"
#include "Sdk/TclStreamEvent_Twitch_SharedChatUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_ShieldModeBegin.h"
#include "Sdk/TclStreamEvent_Twitch_ShieldModeEnd.h"
#include "Sdk/TclStreamEvent_Twitch_ShoutoutCreate.h"
#include "Sdk/TclStreamEvent_Twitch_ShoutoutReceive.h"
#include "Sdk/TclStreamEvent_Twitch_StreamOffline.h"
#include "Sdk/TclStreamEvent_Twitch_StreamOnline.h"
#include "Sdk/TclStreamEvent_Twitch_Subscription.h"
#include "Sdk/TclStreamEvent_Twitch_SubscriptionEnd.h"
#include "Sdk/TclStreamEvent_Twitch_SubscriptionGift.h"
#include "Sdk/TclStreamEvent_Twitch_SubscriptionMessage.h"
#include "Sdk/TclStreamEvent_Twitch_SuspiciousUserMessage.h"
#include "Sdk/TclStreamEvent_Twitch_SuspiciousUserUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_Unban.h"
#include "Sdk/TclStreamEvent_Twitch_UnbanRequestCreate.h"
#include "Sdk/TclStreamEvent_Twitch_UnbanRequestResolve.h"
#include "Sdk/TclStreamEvent_Twitch_UserUpdate.h"
#include "Sdk/TclStreamEvent_Twitch_VipAdd.h"
#include "Sdk/TclStreamEvent_Twitch_VipRemove.h"
#include "Sdk/TclStreamEvent_Twitch_WarningAcknowledge.h"
#include "Sdk/TclStreamEvent_Twitch_WarningSend.h"
#include "Sdk/TclStreamEvent_Twitch_WhisperMessage.h"
#include "Tentacle_Twitch.generated.h"

class UTclViewer;
class UTentacle_TwitchExtras;

/**
 * Twitch on its own: every Twitch event, one delegate per kind, and the calls
 * that address Twitch alone, each named as Twitch's own documentation names it
 * (EventSub's Channel Cheer is On Channel Cheer here). Reached as the Tentacle
 * actor's Twitch component.
 *
 * The Tentacle actor itself carries the cross-platform side, in Tentacle's own
 * words - On Follow for a follow on any platform, the chat command split, the
 * catch-all, Send Chat Message to every connected platform - and those fire
 * around the Twitch delegates here. Listen here for what only Twitch has, or to
 * react to Twitch alone. What Tentacle adds to Twitch lives in Extras.
 */
UCLASS(BlueprintType)
class TENTACLE_API UTentacle_Twitch : public UActorComponent
{
	GENERATED_BODY()

	// One per event kind, named after its EventSub subscription type's title.
	// Events that identify a viewer pass that viewer alongside the event; events
	// about the channel, a poll or a goal pass the event only.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelAdBreakBegin, const UTclStreamEvent_Twitch_AdBreakBegin*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelBan, const UTclStreamEvent_Twitch_Ban*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelBitsUse, const UTclStreamEvent_Twitch_BitsUse*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelChatClear, const UTclStreamEvent_Twitch_ChatClear*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelChatClearUserMessages, const UTclStreamEvent_Twitch_ChatClearUserMessages*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelChatMessage, const UTclStreamEvent_Twitch_ChatMessage*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelChatMessageDelete, const UTclStreamEvent_Twitch_ChatMessageDelete*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelChatSettingsUpdate, const UTclStreamEvent_Twitch_ChatSettingsUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelCheer, const UTclStreamEvent_Twitch_Cheer*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelFollow, const UTclStreamEvent_Twitch_Follow*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelModeratorAdd, const UTclStreamEvent_Twitch_ModeratorAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelModeratorRemove, const UTclStreamEvent_Twitch_ModeratorRemove*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelPointsAutomaticRewardRedemptionAdd, const UTclStreamEvent_Twitch_AutomaticRewardRedemptionAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPointsCustomRewardAdd, const UTclStreamEvent_Twitch_RewardAdd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelPointsCustomRewardRedemptionAdd, const UTclStreamEvent_Twitch_RedemptionAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelPointsCustomRewardRedemptionUpdate, const UTclStreamEvent_Twitch_RedemptionUpdate*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPointsCustomRewardRemove, const UTclStreamEvent_Twitch_RewardRemove*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPointsCustomRewardUpdate, const UTclStreamEvent_Twitch_RewardUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPollBegin, const UTclStreamEvent_Twitch_PollBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPollEnd, const UTclStreamEvent_Twitch_PollEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPollProgress, const UTclStreamEvent_Twitch_PollProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPredictionBegin, const UTclStreamEvent_Twitch_PredictionBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPredictionEnd, const UTclStreamEvent_Twitch_PredictionEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPredictionLock, const UTclStreamEvent_Twitch_PredictionLock*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelPredictionProgress, const UTclStreamEvent_Twitch_PredictionProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelRaid, const UTclStreamEvent_Twitch_Raid*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelSharedChatSessionBegin, const UTclStreamEvent_Twitch_SharedChatBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelSharedChatSessionEnd, const UTclStreamEvent_Twitch_SharedChatEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelSharedChatSessionUpdate, const UTclStreamEvent_Twitch_SharedChatUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscribe, const UTclStreamEvent_Twitch_Subscription*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionEnd, const UTclStreamEvent_Twitch_SubscriptionEnd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionGift, const UTclStreamEvent_Twitch_SubscriptionGift*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionMessage, const UTclStreamEvent_Twitch_SubscriptionMessage*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSuspiciousUserMessage, const UTclStreamEvent_Twitch_SuspiciousUserMessage*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSuspiciousUserUpdate, const UTclStreamEvent_Twitch_SuspiciousUserUpdate*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelUnban, const UTclStreamEvent_Twitch_Unban*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelUnbanRequestCreate, const UTclStreamEvent_Twitch_UnbanRequestCreate*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelUnbanRequestResolve, const UTclStreamEvent_Twitch_UnbanRequestResolve*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelUpdate, const UTclStreamEvent_Twitch_ChannelUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelVipAdd, const UTclStreamEvent_Twitch_VipAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelVipRemove, const UTclStreamEvent_Twitch_VipRemove*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelWarningAcknowledgement, const UTclStreamEvent_Twitch_WarningAcknowledge*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelWarningSend, const UTclStreamEvent_Twitch_WarningSend*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharityCampaignProgress, const UTclStreamEvent_Twitch_CharityCampaignProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharityCampaignStart, const UTclStreamEvent_Twitch_CharityCampaignStart*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharityCampaignStop, const UTclStreamEvent_Twitch_CharityCampaignStop*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharityDonation, const UTclStreamEvent_Twitch_CharityDonation*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalBegin, const UTclStreamEvent_Twitch_GoalBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalEnd, const UTclStreamEvent_Twitch_GoalEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalProgress, const UTclStreamEvent_Twitch_GoalProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypeTrainBegin, const UTclStreamEvent_Twitch_HypeTrainBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypeTrainEnd, const UTclStreamEvent_Twitch_HypeTrainEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypeTrainProgress, const UTclStreamEvent_Twitch_HypeTrainProgress*, Event);
	// The app resolves the acting moderator as the viewer of both shield-mode
	// events, but these shipped without one on the Stream component and every
	// engine follows that shape; read the moderator off the event.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldModeBegin, const UTclStreamEvent_Twitch_ShieldModeBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldModeEnd, const UTclStreamEvent_Twitch_ShieldModeEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShoutoutCreate, const UTclStreamEvent_Twitch_ShoutoutCreate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShoutoutReceived, const UTclStreamEvent_Twitch_ShoutoutReceive*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreamOffline, const UTclStreamEvent_Twitch_StreamOffline*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreamOnline, const UTclStreamEvent_Twitch_StreamOnline*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserUpdate, const UTclStreamEvent_Twitch_UserUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWhisperReceived, const UTclStreamEvent_Twitch_WhisperMessage*, Event, UTclViewer*, Viewer);

public:
	UTentacle_Twitch();

	/** What Tentacle adds to Twitch: things Twitch itself does not have, such as the incoming raid window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tentacle|Twitch")
	UTentacle_TwitchExtras* Extras;

	// One per event kind

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelAdBreakBegin OnChannelAdBreakBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelBan OnChannelBan;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelBitsUse OnChannelBitsUse;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelChatClear OnChannelChatClear;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelChatClearUserMessages OnChannelChatClearUserMessages;

	/**
	 * Every Twitch chat message, commands included, as Twitch sends it. The
	 * Tentacle actor splits chat into On Chat Message and On Chat Command, and
	 * fires those right after this.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelChatMessage OnChannelChatMessage;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelChatMessageDelete OnChannelChatMessageDelete;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelChatSettingsUpdate OnChannelChatSettingsUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelCheer OnChannelCheer;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelFollow OnChannelFollow;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelModeratorAdd OnChannelModeratorAdd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelModeratorRemove OnChannelModeratorRemove;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPointsAutomaticRewardRedemptionAdd OnChannelPointsAutomaticRewardRedemptionAdd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPointsCustomRewardAdd OnChannelPointsCustomRewardAdd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPointsCustomRewardRedemptionAdd OnChannelPointsCustomRewardRedemptionAdd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPointsCustomRewardRedemptionUpdate OnChannelPointsCustomRewardRedemptionUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPointsCustomRewardRemove OnChannelPointsCustomRewardRemove;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPointsCustomRewardUpdate OnChannelPointsCustomRewardUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPollBegin OnChannelPollBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPollEnd OnChannelPollEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPollProgress OnChannelPollProgress;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPredictionBegin OnChannelPredictionBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPredictionEnd OnChannelPredictionEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPredictionLock OnChannelPredictionLock;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelPredictionProgress OnChannelPredictionProgress;

	/**
	 * Fires when another channel raids this one. It opens Extras' incoming raid
	 * window (Is Incoming Raid Ongoing) until End Incoming Raid.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelRaid OnChannelRaid;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSharedChatSessionBegin OnChannelSharedChatSessionBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSharedChatSessionEnd OnChannelSharedChatSessionEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSharedChatSessionUpdate OnChannelSharedChatSessionUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSubscribe OnChannelSubscribe;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSubscriptionEnd OnChannelSubscriptionEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSubscriptionGift OnChannelSubscriptionGift;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSubscriptionMessage OnChannelSubscriptionMessage;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSuspiciousUserMessage OnChannelSuspiciousUserMessage;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelSuspiciousUserUpdate OnChannelSuspiciousUserUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelUnban OnChannelUnban;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelUnbanRequestCreate OnChannelUnbanRequestCreate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelUnbanRequestResolve OnChannelUnbanRequestResolve;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelUpdate OnChannelUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch", meta=(DisplayName="On Channel VIP Add"))
	FOnChannelVipAdd OnChannelVipAdd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch", meta=(DisplayName="On Channel VIP Remove"))
	FOnChannelVipRemove OnChannelVipRemove;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelWarningAcknowledgement OnChannelWarningAcknowledgement;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnChannelWarningSend OnChannelWarningSend;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnCharityCampaignProgress OnCharityCampaignProgress;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnCharityCampaignStart OnCharityCampaignStart;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnCharityCampaignStop OnCharityCampaignStop;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnCharityDonation OnCharityDonation;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnGoalBegin OnGoalBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnGoalEnd OnGoalEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnGoalProgress OnGoalProgress;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnHypeTrainBegin OnHypeTrainBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnHypeTrainEnd OnHypeTrainEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnHypeTrainProgress OnHypeTrainProgress;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnShieldModeBegin OnShieldModeBegin;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnShieldModeEnd OnShieldModeEnd;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnShoutoutCreate OnShoutoutCreate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnShoutoutReceived OnShoutoutReceived;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnStreamOffline OnStreamOffline;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnStreamOnline OnStreamOnline;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnUserUpdate OnUserUpdate;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Twitch")
	FOnWhisperReceived OnWhisperReceived;

	// Chat send

	/**
	 * Send a message in the broadcaster's Twitch chat, from the broadcaster:
	 * Twitch's Send Chat Message. To send on every connected platform at once,
	 * use the Tentacle actor's Send Chat Message instead.
	 *
	 * Fire-and-forget: nothing is reported back, so a message Twitch drops
	 * surfaces in Tentacle's log rather than in the game. During a shared chat
	 * session the message reaches every channel in the session.
	 *
	 * @param Message              The message. 1-500 characters; anything else is
	 *                             refused with a warning in the log instead of being sent.
	 * @param ReplyParentMessageId The id of the chat message this one replies to
	 *                             (a Twitch chat message's Message Id). Empty for none.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Twitch", meta=(AdvancedDisplay="ReplyParentMessageId"))
	void SendChatMessage(const FString& Message, const FString& ReplyParentMessageId = TEXT(""));

	/**
	 * Serialize a Twitch_SendChatMessage message for the given arguments. Split
	 * out so the wire contract can be asserted without a live connection;
	 * performs no validation. An empty ReplyParentMessageId leaves the reply out.
	 */
	static FString BuildSendChatMessagePayload(const FString& Message, const FString& ReplyParentMessageId);

	// --- Dispatch (called by ATentacle) ---

	/**
	 * Fire the delegate for one Twitch event's kind, then the Tentacle actor's
	 * cross-platform view of it (for a chat message, the actor's chat split).
	 * Called once per event, after de-duplication, the viewer upsert, and the
	 * actor's On Stream Event.
	 */
	void DispatchEvent(UTclStreamEvent_Twitch_Base* Event, UTclViewer* Viewer);
};
