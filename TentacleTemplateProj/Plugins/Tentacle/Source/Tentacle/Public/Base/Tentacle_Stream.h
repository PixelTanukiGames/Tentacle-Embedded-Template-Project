// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enums/TclChatSendTarget.h"
#include "Sdk/TclStreamEventBase.h"
#include "Sdk/TclStreamEvent_Twitch_Base.h"
#include "Sdk/TclStreamEvent_Kick_Base.h"
#include "Sdk/TclStreamEvent_Anywhere_Ban.h"
#include "Sdk/TclStreamEvent_Anywhere_ChannelUpdate.h"
#include "Sdk/TclStreamEvent_Anywhere_ChatMessage.h"
#include "Sdk/TclStreamEvent_Anywhere_Donation.h"
#include "Sdk/TclStreamEvent_Anywhere_Follow.h"
#include "Sdk/TclStreamEvent_Anywhere_Redemption.h"
#include "Sdk/TclStreamEvent_Anywhere_Subscription.h"
#include "Sdk/TclStreamEvent_Anywhere_SubscriptionGift.h"
#include "Sdk/TclStreamEvent_Anywhere_SubscriptionRenewal.h"
#include "Sdk/TclStreamEvent_Kick_ChannelFollowed.h"
#include "Sdk/TclStreamEvent_Kick_ChannelRewardRedemptionUpdated.h"
#include "Sdk/TclStreamEvent_Kick_ChannelSubscriptionGifts.h"
#include "Sdk/TclStreamEvent_Kick_ChannelSubscriptionNew.h"
#include "Sdk/TclStreamEvent_Kick_ChannelSubscriptionRenewal.h"
#include "Sdk/TclStreamEvent_Kick_ChatMessage.h"
#include "Sdk/TclStreamEvent_Kick_LivestreamMetadataUpdated.h"
#include "Sdk/TclStreamEvent_Kick_LivestreamStatusUpdated.h"
#include "Sdk/TclStreamEvent_Kick_ModerationBanned.h"
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
#include "Tentacle_Stream.generated.h"

class UTclViewer;
class UTclViewerData;
class UTclMsg_ServerToClient_StreamChatJoin;
class UTclMsg_ServerToClient_StreamEvent;

/**
 * Deprecated: the stream surface as it shipped in 1.0, with the platform in
 * every name (On Follow (Anywhere), On Cheer (Twitch)).
 *
 * Cross-platform events and chat now live on the Tentacle actor itself (On
 * Follow, Send Chat Message), and each platform's own on its Twitch and Kick
 * components (On Cheer on the Twitch component). Every member here keeps
 * firing and working, after the new ones, so graphs built on it keep running;
 * each one's deprecation message names its replacement.
 */
UCLASS(BlueprintType)
class TENTACLE_API UTentacle_Stream : public UActorComponent
{
	GENERATED_BODY()

	// These delegate types are shipped API: their names and signatures stay
	// exactly as they were, whatever replaces them.

	// Every event
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEvent_Anywhere, const UTclStreamEventBase*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEvent_Twitch, const UTclStreamEvent_Twitch_Base*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEvent_Kick, const UTclStreamEvent_Kick_Base*, Event, UTclViewer*, Viewer);

	// Stream (anywhere)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatJoin_Anywhere, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatCommand_Anywhere, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessage_Anywhere, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmojis_Anywhere, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmotes_Anywhere, const UTclStreamEvent_Anywhere_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBan_Anywhere, const UTclStreamEvent_Anywhere_Ban*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelUpdate_Anywhere, const UTclStreamEvent_Anywhere_ChannelUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDonation_Anywhere, const UTclStreamEvent_Anywhere_Donation*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFollow_Anywhere, const UTclStreamEvent_Anywhere_Follow*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscription_Anywhere, const UTclStreamEvent_Anywhere_Subscription*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionGift_Anywhere, const UTclStreamEvent_Anywhere_SubscriptionGift*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionRenewal_Anywhere, const UTclStreamEvent_Anywhere_SubscriptionRenewal*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRedemption_Anywhere, const UTclStreamEvent_Anywhere_Redemption*, Event, UTclViewer*, Viewer);

	// Kick
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatCommand_Kick, const UTclStreamEvent_Kick_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatJoin_Kick, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessage_Kick, const UTclStreamEvent_Kick_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmojis_Kick, const UTclStreamEvent_Kick_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmotes_Kick, const UTclStreamEvent_Kick_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelFollowed_Kick, const UTclStreamEvent_Kick_ChannelFollowed*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelRewardRedemptionUpdated_Kick, const UTclStreamEvent_Kick_ChannelRewardRedemptionUpdated*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionGifts_Kick, const UTclStreamEvent_Kick_ChannelSubscriptionGifts*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionNew_Kick, const UTclStreamEvent_Kick_ChannelSubscriptionNew*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionRenewal_Kick, const UTclStreamEvent_Kick_ChannelSubscriptionRenewal*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLivestreamMetadataUpdated_Kick, const UTclStreamEvent_Kick_LivestreamMetadataUpdated*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLivestreamStatusUpdated_Kick, const UTclStreamEvent_Kick_LivestreamStatusUpdated*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModerationBanned_Kick, const UTclStreamEvent_Kick_ModerationBanned*, Event, UTclViewer*, Viewer);

	// Twitch
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatCommand_Twitch, const UTclStreamEvent_Twitch_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatJoin_Twitch, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessage_Twitch, const UTclStreamEvent_Twitch_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmojis_Twitch, const UTclStreamEvent_Twitch_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageWithEmotes_Twitch, const UTclStreamEvent_Twitch_ChatMessage*, ChatMessage, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAdBreakBegin_Twitch, const UTclStreamEvent_Twitch_AdBreakBegin*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAutomaticRewardRedemptionAdd_Twitch, const UTclStreamEvent_Twitch_AutomaticRewardRedemptionAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBan_Twitch, const UTclStreamEvent_Twitch_Ban*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBitsUse_Twitch, const UTclStreamEvent_Twitch_BitsUse*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChannelUpdate_Twitch, const UTclStreamEvent_Twitch_ChannelUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharityCampaignProgress_Twitch, const UTclStreamEvent_Twitch_CharityCampaignProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharityCampaignStart_Twitch, const UTclStreamEvent_Twitch_CharityCampaignStart*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharityCampaignStop_Twitch, const UTclStreamEvent_Twitch_CharityCampaignStop*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharityDonation_Twitch, const UTclStreamEvent_Twitch_CharityDonation*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatClear_Twitch, const UTclStreamEvent_Twitch_ChatClear*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatClearUserMessages_Twitch, const UTclStreamEvent_Twitch_ChatClearUserMessages*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageDelete_Twitch, const UTclStreamEvent_Twitch_ChatMessageDelete*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatSettingsUpdate_Twitch, const UTclStreamEvent_Twitch_ChatSettingsUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCheer_Twitch, const UTclStreamEvent_Twitch_Cheer*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFollow_Twitch, const UTclStreamEvent_Twitch_Follow*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalBegin_Twitch, const UTclStreamEvent_Twitch_GoalBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalEnd_Twitch, const UTclStreamEvent_Twitch_GoalEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoalProgress_Twitch, const UTclStreamEvent_Twitch_GoalProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypeTrainBegin_Twitch, const UTclStreamEvent_Twitch_HypeTrainBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypeTrainEnd_Twitch, const UTclStreamEvent_Twitch_HypeTrainEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHypeTrainProgress_Twitch, const UTclStreamEvent_Twitch_HypeTrainProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModeratorAdd_Twitch, const UTclStreamEvent_Twitch_ModeratorAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModeratorRemove_Twitch, const UTclStreamEvent_Twitch_ModeratorRemove*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPollBegin_Twitch, const UTclStreamEvent_Twitch_PollBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPollEnd_Twitch, const UTclStreamEvent_Twitch_PollEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPollProgress_Twitch, const UTclStreamEvent_Twitch_PollProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPredictionBegin_Twitch, const UTclStreamEvent_Twitch_PredictionBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPredictionEnd_Twitch, const UTclStreamEvent_Twitch_PredictionEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPredictionLock_Twitch, const UTclStreamEvent_Twitch_PredictionLock*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPredictionProgress_Twitch, const UTclStreamEvent_Twitch_PredictionProgress*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRaid_Twitch, const UTclStreamEvent_Twitch_Raid*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRaidEnded_Twitch);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRedemptionAdd_Twitch, const UTclStreamEvent_Twitch_RedemptionAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRedemptionUpdate_Twitch, const UTclStreamEvent_Twitch_RedemptionUpdate*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardAdd_Twitch, const UTclStreamEvent_Twitch_RewardAdd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardRemove_Twitch, const UTclStreamEvent_Twitch_RewardRemove*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardUpdate_Twitch, const UTclStreamEvent_Twitch_RewardUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSharedChatBegin_Twitch, const UTclStreamEvent_Twitch_SharedChatBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSharedChatEnd_Twitch, const UTclStreamEvent_Twitch_SharedChatEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSharedChatUpdate_Twitch, const UTclStreamEvent_Twitch_SharedChatUpdate*, Event);
	// The app resolves the acting moderator as the row's viewer for both shield-mode
	// events, so by convention these would take (Event, Viewer). They shipped as
	// OneParam in v1.0.1 and widening a delegate breaks every Blueprint graph already
	// bound to it, so the shipped signature stays. Read the moderator off the event.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldModeBegin_Twitch, const UTclStreamEvent_Twitch_ShieldModeBegin*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldModeEnd_Twitch, const UTclStreamEvent_Twitch_ShieldModeEnd*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShoutoutCreate_Twitch, const UTclStreamEvent_Twitch_ShoutoutCreate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShoutoutReceive_Twitch, const UTclStreamEvent_Twitch_ShoutoutReceive*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreamOffline_Twitch, const UTclStreamEvent_Twitch_StreamOffline*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStreamOnline_Twitch, const UTclStreamEvent_Twitch_StreamOnline*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscription_Twitch, const UTclStreamEvent_Twitch_Subscription*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionEnd_Twitch, const UTclStreamEvent_Twitch_SubscriptionEnd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionGift_Twitch, const UTclStreamEvent_Twitch_SubscriptionGift*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSubscriptionMessage_Twitch, const UTclStreamEvent_Twitch_SubscriptionMessage*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSuspiciousUserMessage_Twitch, const UTclStreamEvent_Twitch_SuspiciousUserMessage*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSuspiciousUserUpdate_Twitch, const UTclStreamEvent_Twitch_SuspiciousUserUpdate*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnban_Twitch, const UTclStreamEvent_Twitch_Unban*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnbanRequestCreate_Twitch, const UTclStreamEvent_Twitch_UnbanRequestCreate*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnbanRequestResolve_Twitch, const UTclStreamEvent_Twitch_UnbanRequestResolve*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserUpdate_Twitch, const UTclStreamEvent_Twitch_UserUpdate*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVipAdd_Twitch, const UTclStreamEvent_Twitch_VipAdd*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVipRemove_Twitch, const UTclStreamEvent_Twitch_VipRemove*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWarningAcknowledge_Twitch, const UTclStreamEvent_Twitch_WarningAcknowledge*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWarningSend_Twitch, const UTclStreamEvent_Twitch_WarningSend*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWhisperMessage_Twitch, const UTclStreamEvent_Twitch_WhisperMessage*, Event, UTclViewer*, Viewer);

	// Subathon
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFetchSubathonDonationsComplete_Twitch, float, Dollars, float, Goal, float, GoalProgress);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFetchSubathonGiftedSubscriptionsComplete_Twitch, int32, Count, int32, Goal, float, GoalProgress);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFetchSubathonSubscriptionsComplete_Twitch, int32, Count, int32, Goal, float, GoalProgress);

public:
	// Every event

	/** Deprecated: use On Stream Event on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Event (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Stream Event on the Tentacle actor."))
	FOnEvent_Anywhere OnEvent_Anywhere;

	/** Deprecated: use On Stream Event on the Tentacle actor, and check the event's class for its platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Event (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Stream Event on the Tentacle actor, and check the event's class for its platform."))
	FOnEvent_Twitch OnEvent_Twitch;

	/** Deprecated: use On Stream Event on the Tentacle actor, and check the event's class for its platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Event (Kick)", DeprecatedProperty, DeprecationMessage="Use On Stream Event on the Tentacle actor, and check the event's class for its platform."))
	FOnEvent_Kick OnEvent_Kick;

	// Stream (anywhere)

	/** Deprecated: use On Chat Join on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Join (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Chat Join on the Tentacle actor."))
	FOnChatJoin_Anywhere OnChatJoin_Anywhere;

	/** Deprecated: use On Chat Command on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Command (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Chat Command on the Tentacle actor."))
	FOnChatCommand_Anywhere OnChatCommand_Anywhere;

	/** Deprecated: use On Chat Message on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Chat Message on the Tentacle actor."))
	FOnChatMessage_Anywhere OnChatMessage_Anywhere;

	/** Deprecated: use On Chat Message With Emojis on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message With Emojis (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Chat Message With Emojis on the Tentacle actor."))
	FOnChatMessageWithEmojis_Anywhere OnChatMessageWithEmojis_Anywhere;

	/** Deprecated: use On Chat Message With Emotes on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message With Emotes (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Chat Message With Emotes on the Tentacle actor."))
	FOnChatMessageWithEmotes_Anywhere OnChatMessageWithEmotes_Anywhere;

	/** Deprecated: use On Ban on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Ban (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Ban on the Tentacle actor."))
	FOnBan_Anywhere OnBan_Anywhere;

	/** Deprecated: use On Channel Update on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Update (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Channel Update on the Tentacle actor."))
	FOnChannelUpdate_Anywhere OnChannelUpdate_Anywhere;

	/** Deprecated: use On Donation on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Donation (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Donation on the Tentacle actor."))
	FOnDonation_Anywhere OnDonation_Anywhere;

	/** Deprecated: use On Follow on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Follow (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Follow on the Tentacle actor."))
	FOnFollow_Anywhere OnFollow_Anywhere;

	/** Deprecated: use On Subscription on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Subscription on the Tentacle actor."))
	FOnSubscription_Anywhere OnSubscription_Anywhere;

	/** Deprecated: use On Subscription Gift on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription Gift (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Subscription Gift on the Tentacle actor."))
	FOnSubscriptionGift_Anywhere OnSubscriptionGift_Anywhere;

	/** Deprecated: use On Subscription Renewal on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription Renewal (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Subscription Renewal on the Tentacle actor."))
	FOnSubscriptionRenewal_Anywhere OnSubscriptionRenewal_Anywhere;

	/** Deprecated: use On Redemption on the Tentacle actor. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Redemption (Anywhere)", DeprecatedProperty, DeprecationMessage="Use On Redemption on the Tentacle actor."))
	FOnRedemption_Anywhere OnRedemption_Anywhere;

	// Kick

	/** Deprecated: use On Chat Command on the Tentacle actor, and check the message's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Command (Kick)", DeprecatedProperty, DeprecationMessage="Use On Chat Command on the Tentacle actor, and check the message's platform."))
	FOnChatCommand_Kick OnChatCommand_Kick;

	/** Deprecated: use On Chat Join on the Tentacle actor, and check the viewer's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Join (Kick)", DeprecatedProperty, DeprecationMessage="Use On Chat Join on the Tentacle actor, and check the viewer's platform."))
	FOnChatJoin_Kick OnChatJoin_Kick;

	/** Deprecated: use On Chat Message on the Tentacle actor and check the message's platform, or On Chat Message on Tentacle's Kick component, which includes commands. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message (Kick)", DeprecatedProperty, DeprecationMessage="Use On Chat Message on the Tentacle actor and check the message's platform, or On Chat Message on Tentacle's Kick component, which includes commands."))
	FOnChatMessage_Kick OnChatMessage_Kick;

	/** Deprecated: use On Chat Message With Emojis on the Tentacle actor, and check the message's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message With Emojis (Kick)", DeprecatedProperty, DeprecationMessage="Use On Chat Message With Emojis on the Tentacle actor, and check the message's platform."))
	FOnChatMessageWithEmojis_Kick OnChatMessageWithEmojis_Kick;

	/** Deprecated: use On Chat Message With Emotes on the Tentacle actor, and check the message's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message With Emotes (Kick)", DeprecatedProperty, DeprecationMessage="Use On Chat Message With Emotes on the Tentacle actor, and check the message's platform."))
	FOnChatMessageWithEmotes_Kick OnChatMessageWithEmotes_Kick;

	/** Deprecated: use On Channel Follow on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Followed (Kick)", DeprecatedProperty, DeprecationMessage="Use On Channel Follow on Tentacle's Kick component."))
	FOnChannelFollowed_Kick OnChannelFollowed_Kick;

	/** Deprecated: use On Channel Reward Redemption Updated on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Reward Redemption Updated (Kick)", DeprecatedProperty, DeprecationMessage="Use On Channel Reward Redemption Updated on Tentacle's Kick component."))
	FOnChannelRewardRedemptionUpdated_Kick OnChannelRewardRedemptionUpdated_Kick;

	/** Deprecated: use On Channel Subscription Gifts on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Subscription Gifts (Kick)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscription Gifts on Tentacle's Kick component."))
	FOnChannelSubscriptionGifts_Kick OnChannelSubscriptionGifts_Kick;

	/** Deprecated: use On Channel Subscription Created on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Subscription New (Kick)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscription Created on Tentacle's Kick component."))
	FOnChannelSubscriptionNew_Kick OnChannelSubscriptionNew_Kick;

	/** Deprecated: use On Channel Subscription Renewal on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Subscription Renewal (Kick)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscription Renewal on Tentacle's Kick component."))
	FOnChannelSubscriptionRenewal_Kick OnChannelSubscriptionRenewal_Kick;

	/** Deprecated: use On Livestream Metadata Updated on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Livestream Metadata Updated (Kick)", DeprecatedProperty, DeprecationMessage="Use On Livestream Metadata Updated on Tentacle's Kick component."))
	FOnLivestreamMetadataUpdated_Kick OnLivestreamMetadataUpdated_Kick;

	/** Deprecated: use On Livestream Status Updated on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Livestream Status Updated (Kick)", DeprecatedProperty, DeprecationMessage="Use On Livestream Status Updated on Tentacle's Kick component."))
	FOnLivestreamStatusUpdated_Kick OnLivestreamStatusUpdated_Kick;

	/** Deprecated: use On Moderation Banned on Tentacle's Kick component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Moderation Banned (Kick)", DeprecatedProperty, DeprecationMessage="Use On Moderation Banned on Tentacle's Kick component."))
	FOnModerationBanned_Kick OnModerationBanned_Kick;

	// Twitch

	/** Deprecated: use On Chat Command on the Tentacle actor, and check the message's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Command (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Chat Command on the Tentacle actor, and check the message's platform."))
	FOnChatCommand_Twitch OnChatCommand_Twitch;

	/** Deprecated: use On Chat Join on the Tentacle actor, and check the viewer's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Join (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Chat Join on the Tentacle actor, and check the viewer's platform."))
	FOnChatJoin_Twitch OnChatJoin_Twitch;

	/** Deprecated: use On Chat Message on the Tentacle actor and check the message's platform, or On Channel Chat Message on Tentacle's Twitch component, which includes commands. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Chat Message on the Tentacle actor and check the message's platform, or On Channel Chat Message on Tentacle's Twitch component, which includes commands."))
	FOnChatMessage_Twitch OnChatMessage_Twitch;

	/** Deprecated: use On Chat Message With Emojis on the Tentacle actor, and check the message's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message With Emojis (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Chat Message With Emojis on the Tentacle actor, and check the message's platform."))
	FOnChatMessageWithEmojis_Twitch OnChatMessageWithEmojis_Twitch;

	/** Deprecated: use On Chat Message With Emotes on the Tentacle actor, and check the message's platform. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message With Emotes (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Chat Message With Emotes on the Tentacle actor, and check the message's platform."))
	FOnChatMessageWithEmotes_Twitch OnChatMessageWithEmotes_Twitch;

	/** Deprecated: use On Channel Ad Break Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Ad Break Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Ad Break Begin on Tentacle's Twitch component."))
	FOnAdBreakBegin_Twitch OnAdBreakBegin_Twitch;

	/** Deprecated: use On Channel Points Automatic Reward Redemption Add on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Automatic Reward Redemption Add (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Points Automatic Reward Redemption Add on Tentacle's Twitch component."))
	FOnAutomaticRewardRedemptionAdd_Twitch OnAutomaticRewardRedemptionAdd_Twitch;

	/** Deprecated: use On Channel Ban on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Ban (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Ban on Tentacle's Twitch component."))
	FOnBan_Twitch OnBan_Twitch;

	/** Deprecated: use On Channel Bits Use on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Bits Use (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Bits Use on Tentacle's Twitch component."))
	FOnBitsUse_Twitch OnBitsUse_Twitch;

	/** Deprecated: use On Channel Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Channel Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Update on Tentacle's Twitch component."))
	FOnChannelUpdate_Twitch OnChannelUpdate_Twitch;

	/** Deprecated: use On Charity Campaign Progress on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Charity Campaign Progress (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Charity Campaign Progress on Tentacle's Twitch component."))
	FOnCharityCampaignProgress_Twitch OnCharityCampaignProgress_Twitch;

	/** Deprecated: use On Charity Campaign Start on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Charity Campaign Start (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Charity Campaign Start on Tentacle's Twitch component."))
	FOnCharityCampaignStart_Twitch OnCharityCampaignStart_Twitch;

	/** Deprecated: use On Charity Campaign Stop on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Charity Campaign Stop (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Charity Campaign Stop on Tentacle's Twitch component."))
	FOnCharityCampaignStop_Twitch OnCharityCampaignStop_Twitch;

	/** Deprecated: use On Charity Donation on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Charity Donation (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Charity Donation on Tentacle's Twitch component."))
	FOnCharityDonation_Twitch OnCharityDonation_Twitch;

	/** Deprecated: use On Channel Chat Clear on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Clear (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Chat Clear on Tentacle's Twitch component."))
	FOnChatClear_Twitch OnChatClear_Twitch;

	/** Deprecated: use On Channel Chat Clear User Messages on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Clear User Messages (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Chat Clear User Messages on Tentacle's Twitch component."))
	FOnChatClearUserMessages_Twitch OnChatClearUserMessages_Twitch;

	/** Deprecated: use On Channel Chat Message Delete on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Message Delete (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Chat Message Delete on Tentacle's Twitch component."))
	FOnChatMessageDelete_Twitch OnChatMessageDelete_Twitch;

	/** Deprecated: use On Channel Chat Settings Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Chat Settings Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Chat Settings Update on Tentacle's Twitch component."))
	FOnChatSettingsUpdate_Twitch OnChatSettingsUpdate_Twitch;

	/** Deprecated: use On Channel Cheer on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Cheer (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Cheer on Tentacle's Twitch component."))
	FOnCheer_Twitch OnCheer_Twitch;

	/** Deprecated: use On Channel Follow on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Follow (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Follow on Tentacle's Twitch component."))
	FOnFollow_Twitch OnFollow_Twitch;

	/** Deprecated: use On Goal Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Goal Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Goal Begin on Tentacle's Twitch component."))
	FOnGoalBegin_Twitch OnGoalBegin_Twitch;

	/** Deprecated: use On Goal End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Goal End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Goal End on Tentacle's Twitch component."))
	FOnGoalEnd_Twitch OnGoalEnd_Twitch;

	/** Deprecated: use On Goal Progress on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Goal Progress (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Goal Progress on Tentacle's Twitch component."))
	FOnGoalProgress_Twitch OnGoalProgress_Twitch;

	/** Deprecated: use On Hype Train Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Hype Train Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Hype Train Begin on Tentacle's Twitch component."))
	FOnHypeTrainBegin_Twitch OnHypeTrainBegin_Twitch;

	/** Deprecated: use On Hype Train End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Hype Train End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Hype Train End on Tentacle's Twitch component."))
	FOnHypeTrainEnd_Twitch OnHypeTrainEnd_Twitch;

	/** Deprecated: use On Hype Train Progress on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Hype Train Progress (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Hype Train Progress on Tentacle's Twitch component."))
	FOnHypeTrainProgress_Twitch OnHypeTrainProgress_Twitch;

	/** Deprecated: use On Channel Moderator Add on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Moderator Add (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Moderator Add on Tentacle's Twitch component."))
	FOnModeratorAdd_Twitch OnModeratorAdd_Twitch;

	/** Deprecated: use On Channel Moderator Remove on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Moderator Remove (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Moderator Remove on Tentacle's Twitch component."))
	FOnModeratorRemove_Twitch OnModeratorRemove_Twitch;

	/** Deprecated: use On Channel Poll Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Poll Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Poll Begin on Tentacle's Twitch component."))
	FOnPollBegin_Twitch OnPollBegin_Twitch;

	/** Deprecated: use On Channel Poll End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Poll End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Poll End on Tentacle's Twitch component."))
	FOnPollEnd_Twitch OnPollEnd_Twitch;

	/** Deprecated: use On Channel Poll Progress on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Poll Progress (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Poll Progress on Tentacle's Twitch component."))
	FOnPollProgress_Twitch OnPollProgress_Twitch;

	/** Deprecated: use On Channel Prediction Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Prediction Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Prediction Begin on Tentacle's Twitch component."))
	FOnPredictionBegin_Twitch OnPredictionBegin_Twitch;

	/** Deprecated: use On Channel Prediction End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Prediction End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Prediction End on Tentacle's Twitch component."))
	FOnPredictionEnd_Twitch OnPredictionEnd_Twitch;

	/** Deprecated: use On Channel Prediction Lock on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Prediction Lock (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Prediction Lock on Tentacle's Twitch component."))
	FOnPredictionLock_Twitch OnPredictionLock_Twitch;

	/** Deprecated: use On Channel Prediction Progress on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Prediction Progress (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Prediction Progress on Tentacle's Twitch component."))
	FOnPredictionProgress_Twitch OnPredictionProgress_Twitch;

	/** Deprecated: use On Channel Raid on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Raid (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Raid on Tentacle's Twitch component."))
	FOnRaid_Twitch OnRaid_Twitch;

	/** Deprecated: use On Incoming Raid Ended on the Extras of Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Raid Ended (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Incoming Raid Ended on the Extras of Tentacle's Twitch component."))
	FOnRaidEnded_Twitch OnRaidEnded_Twitch;

	/** Deprecated: use On Channel Points Custom Reward Redemption Add on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Redemption Add (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Points Custom Reward Redemption Add on Tentacle's Twitch component."))
	FOnRedemptionAdd_Twitch OnRedemptionAdd_Twitch;

	/** Deprecated: use On Channel Points Custom Reward Redemption Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Redemption Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Points Custom Reward Redemption Update on Tentacle's Twitch component."))
	FOnRedemptionUpdate_Twitch OnRedemptionUpdate_Twitch;

	/** Deprecated: use On Channel Points Custom Reward Add on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Reward Add (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Points Custom Reward Add on Tentacle's Twitch component."))
	FOnRewardAdd_Twitch OnRewardAdd_Twitch;

	/** Deprecated: use On Channel Points Custom Reward Remove on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Reward Remove (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Points Custom Reward Remove on Tentacle's Twitch component."))
	FOnRewardRemove_Twitch OnRewardRemove_Twitch;

	/** Deprecated: use On Channel Points Custom Reward Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Reward Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Points Custom Reward Update on Tentacle's Twitch component."))
	FOnRewardUpdate_Twitch OnRewardUpdate_Twitch;

	/** Deprecated: use On Channel Shared Chat Session Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shared Chat Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Shared Chat Session Begin on Tentacle's Twitch component."))
	FOnSharedChatBegin_Twitch OnSharedChatBegin_Twitch;

	/** Deprecated: use On Channel Shared Chat Session End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shared Chat End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Shared Chat Session End on Tentacle's Twitch component."))
	FOnSharedChatEnd_Twitch OnSharedChatEnd_Twitch;

	/** Deprecated: use On Channel Shared Chat Session Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shared Chat Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Shared Chat Session Update on Tentacle's Twitch component."))
	FOnSharedChatUpdate_Twitch OnSharedChatUpdate_Twitch;

	/** Deprecated: use On Shield Mode Begin on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shield Mode Begin (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Shield Mode Begin on Tentacle's Twitch component."))
	FOnShieldModeBegin_Twitch OnShieldModeBegin_Twitch;

	/** Deprecated: use On Shield Mode End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shield Mode End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Shield Mode End on Tentacle's Twitch component."))
	FOnShieldModeEnd_Twitch OnShieldModeEnd_Twitch;

	/** Deprecated: use On Shoutout Create on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shoutout Create (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Shoutout Create on Tentacle's Twitch component."))
	FOnShoutoutCreate_Twitch OnShoutoutCreate_Twitch;

	/** Deprecated: use On Shoutout Received on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Shoutout Receive (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Shoutout Received on Tentacle's Twitch component."))
	FOnShoutoutReceive_Twitch OnShoutoutReceive_Twitch;

	/** Deprecated: use On Stream Offline on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Stream Offline (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Stream Offline on Tentacle's Twitch component."))
	FOnStreamOffline_Twitch OnStreamOffline_Twitch;

	/** Deprecated: use On Stream Online on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Stream Online (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Stream Online on Tentacle's Twitch component."))
	FOnStreamOnline_Twitch OnStreamOnline_Twitch;

	/** Deprecated: use On Channel Subscribe on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscribe on Tentacle's Twitch component."))
	FOnSubscription_Twitch OnSubscription_Twitch;

	/** Deprecated: use On Channel Subscription End on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription End (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscription End on Tentacle's Twitch component."))
	FOnSubscriptionEnd_Twitch OnSubscriptionEnd_Twitch;

	/** Deprecated: use On Channel Subscription Gift on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription Gift (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscription Gift on Tentacle's Twitch component."))
	FOnSubscriptionGift_Twitch OnSubscriptionGift_Twitch;

	/** Deprecated: use On Channel Subscription Message on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Subscription Message (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Subscription Message on Tentacle's Twitch component."))
	FOnSubscriptionMessage_Twitch OnSubscriptionMessage_Twitch;

	/** Deprecated: use On Channel Suspicious User Message on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Suspicious User Message (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Suspicious User Message on Tentacle's Twitch component."))
	FOnSuspiciousUserMessage_Twitch OnSuspiciousUserMessage_Twitch;

	/** Deprecated: use On Channel Suspicious User Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Suspicious User Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Suspicious User Update on Tentacle's Twitch component."))
	FOnSuspiciousUserUpdate_Twitch OnSuspiciousUserUpdate_Twitch;

	/** Deprecated: use On Channel Unban on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Unban (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Unban on Tentacle's Twitch component."))
	FOnUnban_Twitch OnUnban_Twitch;

	/** Deprecated: use On Channel Unban Request Create on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Unban Request Create (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Unban Request Create on Tentacle's Twitch component."))
	FOnUnbanRequestCreate_Twitch OnUnbanRequestCreate_Twitch;

	/** Deprecated: use On Channel Unban Request Resolve on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Unban Request Resolve (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Unban Request Resolve on Tentacle's Twitch component."))
	FOnUnbanRequestResolve_Twitch OnUnbanRequestResolve_Twitch;

	/** Deprecated: use On User Update on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On User Update (Twitch)", DeprecatedProperty, DeprecationMessage="Use On User Update on Tentacle's Twitch component."))
	FOnUserUpdate_Twitch OnUserUpdate_Twitch;

	/** Deprecated: use On Channel VIP Add on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On VIP Add (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel VIP Add on Tentacle's Twitch component."))
	FOnVipAdd_Twitch OnVipAdd_Twitch;

	/** Deprecated: use On Channel VIP Remove on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On VIP Remove (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel VIP Remove on Tentacle's Twitch component."))
	FOnVipRemove_Twitch OnVipRemove_Twitch;

	/** Deprecated: use On Channel Warning Acknowledgement on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Warning Acknowledge (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Warning Acknowledgement on Tentacle's Twitch component."))
	FOnWarningAcknowledge_Twitch OnWarningAcknowledge_Twitch;

	/** Deprecated: use On Channel Warning Send on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Warning Send (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Channel Warning Send on Tentacle's Twitch component."))
	FOnWarningSend_Twitch OnWarningSend_Twitch;

	/** Deprecated: use On Whisper Received on Tentacle's Twitch component. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Whisper Message (Twitch)", DeprecatedProperty, DeprecationMessage="Use On Whisper Received on Tentacle's Twitch component."))
	FOnWhisperMessage_Twitch OnWhisperMessage_Twitch;

	// Subathon
	//
	// Internal only, and not working yet: the subathon counters behind these are
	// unfinished (the start date and goals are fixed presets in the app). They
	// are deprecated here along with the rest of the component.

	/** Deprecated, and internal only: subathon stats do not work yet. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Fetch Subathon Donations Complete (Twitch)", DeprecatedProperty, DeprecationMessage="Subathon stats are internal only and do not work yet."))
	FOnFetchSubathonDonationsComplete_Twitch OnFetchSubathonDonationsComplete_Twitch;

	/** Deprecated, and internal only: subathon stats do not work yet. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Fetch Subathon Gifted Subscriptions Complete (Twitch)", DeprecatedProperty, DeprecationMessage="Subathon stats are internal only and do not work yet."))
	FOnFetchSubathonGiftedSubscriptionsComplete_Twitch OnFetchSubathonGiftedSubscriptionsComplete_Twitch;

	/** Deprecated, and internal only: subathon stats do not work yet. */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Deprecated", meta=(DisplayName="On Fetch Subathon Subscriptions Complete (Twitch)", DeprecatedProperty, DeprecationMessage="Subathon stats are internal only and do not work yet."))
	FOnFetchSubathonSubscriptionsComplete_Twitch OnFetchSubathonSubscriptionsComplete_Twitch;

	// --- Handler methods ---

	/** Deprecated C++ entry point: forwards to ATentacle::HandleChatJoin, which fires every surface. */
	void HandleMsg_ChatJoin(UTclMsg_ServerToClient_StreamChatJoin* Msg);

	/** Deprecated C++ entry point: forwards to ATentacle::HandleStreamEvent, which fires every surface. */
	void HandleMsg_Event(UTclMsg_ServerToClient_StreamEvent* Msg);

	/**
	 * Fire this component's delegates for one stream event, in their original
	 * order. Called by ATentacle after the new surfaces, once per event, after
	 * de-duplication and the viewer upsert.
	 */
	void DispatchDeprecatedEvent(UTclStreamEventBase* Event, UTclViewer* Viewer);

	/** Fire this component's chat join delegates. Called by ATentacle after the new surfaces. */
	void DispatchDeprecatedChatJoin(UTclViewerData* ViewerData, UTclViewer* Viewer);

	/**
	 * Close this component's copy of the raid window and fire its raid-ended
	 * event. Called by End Incoming Raid on the Twitch component's Extras, which
	 * own the window.
	 */
	void MirrorRaidEnded();

	// --- Subathon fetch methods ---

	/** Deprecated, and internal only: subathon stats do not work yet. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Deprecated", meta=(DisplayName="Fetch Subathon Donations (Twitch)", DeprecatedFunction, DeprecationMessage="Subathon stats are internal only and do not work yet."))
	void FetchTwitchSubathonDonations();

	/** Deprecated, and internal only: subathon stats do not work yet. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Deprecated", meta=(DisplayName="Fetch Subathon Gifted Subscriptions (Twitch)", DeprecatedFunction, DeprecationMessage="Subathon stats are internal only and do not work yet."))
	void FetchTwitchSubathonGiftedSubscriptions();

	/** Deprecated, and internal only: subathon stats do not work yet. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Deprecated", meta=(DisplayName="Fetch Subathon Subscriptions (Twitch)", DeprecatedFunction, DeprecationMessage="Subathon stats are internal only and do not work yet."))
	void FetchTwitchSubathonSubscriptions();

	/** Deprecated: use End Incoming Raid on the Extras of Tentacle's Twitch component. Ends the raid there too. */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Deprecated", meta=(DisplayName="End Raid (Twitch)", DeprecatedFunction, DeprecationMessage="Use End Incoming Raid on the Extras of Tentacle's Twitch component."))
	void EndRaid_Twitch();

	/** Deprecated: use Is Incoming Raid Ongoing on the Extras of Tentacle's Twitch component. Kept in step with it. */
	UPROPERTY(BlueprintReadOnly, Category="Tentacle|Deprecated", meta=(DeprecatedProperty, DeprecationMessage="Use Is Incoming Raid Ongoing on the Extras of Tentacle's Twitch component."))
	bool bIsTwitchRaidOngoing = false;

	// --- Chat send methods ---

	/** Deprecated: use ATentacle::ChatMessageMaxLength. */
	static constexpr int32 ChatMessageMaxLength = 500;

	/** Deprecated: use Get Chat Message Max Length on the Tentacle actor. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Deprecated", meta=(DeprecatedFunction, DeprecationMessage="Use Get Chat Message Max Length on the Tentacle actor."))
	static int32 GetChatMessageMaxLength();

	/**
	 * Deprecated: use Send Chat Message on the Tentacle actor for every
	 * connected platform (Target All Connected), or for one platform alone,
	 * Send Chat Message on Tentacle's Twitch component or Post Chat Message on
	 * its Kick component. Still sends exactly as it always has.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Deprecated", meta=(DisplayName="Send Chat Message", AdvancedDisplay="Target,bIsBot", DeprecatedFunction, DeprecationMessage="Use Send Chat Message on the Tentacle actor for every connected platform. For one platform alone, use Send Chat Message on Tentacle's Twitch component or Post Chat Message on its Kick component."))
	void SendChatMessage(
		const FString& Message,
		ETclChatSendTarget Target = ETclChatSendTarget::AllConnected,
		bool bIsBot = false);

	/**
	 * Deprecated: use Send Chat Message To Viewer on the Tentacle actor, which
	 * mentions the viewer on every connected platform. Still sends exactly as it
	 * always has, a Target of one platform included.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Deprecated", meta=(DisplayName="Send Chat Message To Viewer", AdvancedDisplay="Target,bIsBot", DeprecatedFunction, DeprecationMessage="Use Send Chat Message To Viewer on the Tentacle actor, which mentions the viewer on every connected platform."))
	void SendChatMessageToViewer(
		UTclViewer* Viewer,
		const FString& Message,
		ETclChatSendTarget Target = ETclChatSendTarget::AllConnected,
		bool bIsBot = false);

	/** Deprecated: use ATentacle::BuildChatMessagePayload, which this forwards to. */
	static FString BuildChatMessagePayload(
		const FString& Message,
		ETclChatSendTarget Target,
		bool bIsBot,
		const FString& ViewerId);

private:
	void DispatchEvent_Twitch(UTclStreamEventBase* Event, UTclViewer* Viewer);
	void DispatchEvent_Kick(UTclStreamEventBase* Event, UTclViewer* Viewer);
	void DispatchChatMessage_Twitch(UTclStreamEvent_Twitch_ChatMessage* ChatMsg, UTclViewer* Viewer);
	void DispatchChatMessage_Kick(UTclStreamEvent_Kick_ChatMessage* ChatMsg, UTclViewer* Viewer);
	void DispatchChatMessage_Anywhere(UTclStreamEvent_Anywhere_ChatMessage* ChatMsg, UTclViewer* Viewer);
};
