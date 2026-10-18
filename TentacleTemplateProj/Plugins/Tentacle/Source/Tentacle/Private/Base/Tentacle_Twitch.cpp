// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Twitch.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_TwitchExtras.h"
#include "Base/TclViewer.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/TclCommon.h"

UTentacle_Twitch::UTentacle_Twitch()
{
	Extras = CreateDefaultSubobject<UTentacle_TwitchExtras>(TEXT("Extras"));
}

// ============================================================================
// Dispatch
// ============================================================================

void
UTentacle_Twitch::DispatchEvent(UTclStreamEvent_Twitch_Base* Event, UTclViewer* Viewer)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Event);

	// One branch per concrete Twitch event class. They are siblings under
	// UTclStreamEvent_Twitch_Base, so the order carries no meaning - it mirrors
	// the delegate order in the header. A kind with a cross-platform view fires
	// the Tentacle actor's view right after its own delegate. The `else` at the
	// end only runs for an event class the app sends that this plugin predates.
	if (auto* AdBreakBegin = Cast<UTclStreamEvent_Twitch_AdBreakBegin>(Event))
	{
		OnChannelAdBreakBegin.Broadcast(AdBreakBegin, Viewer);
	}
	else if (auto* Ban = Cast<UTclStreamEvent_Twitch_Ban>(Event))
	{
		OnChannelBan.Broadcast(Ban, Viewer);
		Tentacle->OnBan.Broadcast(Ban->GetAnywhereEvent(), Viewer);
	}
	else if (auto* BitsUse = Cast<UTclStreamEvent_Twitch_BitsUse>(Event))
	{
		OnChannelBitsUse.Broadcast(BitsUse, Viewer);
	}
	else if (auto* ChatClear = Cast<UTclStreamEvent_Twitch_ChatClear>(Event))
	{
		OnChannelChatClear.Broadcast(ChatClear);
	}
	else if (auto* ChatClearUser = Cast<UTclStreamEvent_Twitch_ChatClearUserMessages>(Event))
	{
		OnChannelChatClearUserMessages.Broadcast(ChatClearUser, Viewer);
	}
	else if (auto* ChatMsg = Cast<UTclStreamEvent_Twitch_ChatMessage>(Event))
	{
		// Twitch's own event is the whole message, commands included; the split
		// into message and command is the Tentacle actor's.
		OnChannelChatMessage.Broadcast(ChatMsg, Viewer);
		if (auto* Anywhere = ChatMsg->GetAnywhereEvent())
			Tentacle->DispatchChatMessage(Anywhere, Viewer);
	}
	else if (auto* ChatMessageDelete = Cast<UTclStreamEvent_Twitch_ChatMessageDelete>(Event))
	{
		OnChannelChatMessageDelete.Broadcast(ChatMessageDelete, Viewer);
	}
	else if (auto* ChatSettings = Cast<UTclStreamEvent_Twitch_ChatSettingsUpdate>(Event))
	{
		OnChannelChatSettingsUpdate.Broadcast(ChatSettings);
	}
	else if (auto* Cheer = Cast<UTclStreamEvent_Twitch_Cheer>(Event))
	{
		OnChannelCheer.Broadcast(Cheer, Viewer);
		Tentacle->OnDonation.Broadcast(Cheer->GetAnywhereEvent(), Viewer);
	}
	else if (auto* Follow = Cast<UTclStreamEvent_Twitch_Follow>(Event))
	{
		OnChannelFollow.Broadcast(Follow, Viewer);
		Tentacle->OnFollow.Broadcast(Follow->GetAnywhereEvent(), Viewer);
	}
	else if (auto* ModeratorAdd = Cast<UTclStreamEvent_Twitch_ModeratorAdd>(Event))
	{
		OnChannelModeratorAdd.Broadcast(ModeratorAdd, Viewer);
	}
	else if (auto* ModeratorRemove = Cast<UTclStreamEvent_Twitch_ModeratorRemove>(Event))
	{
		OnChannelModeratorRemove.Broadcast(ModeratorRemove, Viewer);
	}
	else if (auto* AutomaticRedemption = Cast<UTclStreamEvent_Twitch_AutomaticRewardRedemptionAdd>(Event))
	{
		OnChannelPointsAutomaticRewardRedemptionAdd.Broadcast(AutomaticRedemption, Viewer);
	}
	else if (auto* RewardAdd = Cast<UTclStreamEvent_Twitch_RewardAdd>(Event))
	{
		OnChannelPointsCustomRewardAdd.Broadcast(RewardAdd);
	}
	else if (auto* RedemptionAdd = Cast<UTclStreamEvent_Twitch_RedemptionAdd>(Event))
	{
		OnChannelPointsCustomRewardRedemptionAdd.Broadcast(RedemptionAdd, Viewer);
		Tentacle->OnRedemption.Broadcast(RedemptionAdd->GetAnywhereEvent(), Viewer);
	}
	else if (auto* RedemptionUpdate = Cast<UTclStreamEvent_Twitch_RedemptionUpdate>(Event))
	{
		OnChannelPointsCustomRewardRedemptionUpdate.Broadcast(RedemptionUpdate, Viewer);
		Tentacle->OnRedemption.Broadcast(RedemptionUpdate->GetAnywhereEvent(), Viewer);
	}
	else if (auto* RewardRemove = Cast<UTclStreamEvent_Twitch_RewardRemove>(Event))
	{
		OnChannelPointsCustomRewardRemove.Broadcast(RewardRemove);
	}
	else if (auto* RewardUpdate = Cast<UTclStreamEvent_Twitch_RewardUpdate>(Event))
	{
		OnChannelPointsCustomRewardUpdate.Broadcast(RewardUpdate);
	}
	else if (auto* PollBegin = Cast<UTclStreamEvent_Twitch_PollBegin>(Event))
	{
		OnChannelPollBegin.Broadcast(PollBegin);
	}
	else if (auto* PollEnd = Cast<UTclStreamEvent_Twitch_PollEnd>(Event))
	{
		OnChannelPollEnd.Broadcast(PollEnd);
	}
	else if (auto* PollProgress = Cast<UTclStreamEvent_Twitch_PollProgress>(Event))
	{
		OnChannelPollProgress.Broadcast(PollProgress);
	}
	else if (auto* PredictionBegin = Cast<UTclStreamEvent_Twitch_PredictionBegin>(Event))
	{
		OnChannelPredictionBegin.Broadcast(PredictionBegin);
	}
	else if (auto* PredictionEnd = Cast<UTclStreamEvent_Twitch_PredictionEnd>(Event))
	{
		OnChannelPredictionEnd.Broadcast(PredictionEnd);
	}
	else if (auto* PredictionLock = Cast<UTclStreamEvent_Twitch_PredictionLock>(Event))
	{
		OnChannelPredictionLock.Broadcast(PredictionLock);
	}
	else if (auto* PredictionProgress = Cast<UTclStreamEvent_Twitch_PredictionProgress>(Event))
	{
		OnChannelPredictionProgress.Broadcast(PredictionProgress);
	}
	else if (auto* Raid = Cast<UTclStreamEvent_Twitch_Raid>(Event))
	{
		// Opened first, so a handler already sees the window open.
		if (Extras)
			Extras->BeginIncomingRaid();
		OnChannelRaid.Broadcast(Raid, Viewer);
	}
	else if (auto* SharedChatBegin = Cast<UTclStreamEvent_Twitch_SharedChatBegin>(Event))
	{
		OnChannelSharedChatSessionBegin.Broadcast(SharedChatBegin);
	}
	else if (auto* SharedChatEnd = Cast<UTclStreamEvent_Twitch_SharedChatEnd>(Event))
	{
		OnChannelSharedChatSessionEnd.Broadcast(SharedChatEnd);
	}
	else if (auto* SharedChatUpdate = Cast<UTclStreamEvent_Twitch_SharedChatUpdate>(Event))
	{
		OnChannelSharedChatSessionUpdate.Broadcast(SharedChatUpdate);
	}
	else if (auto* Sub = Cast<UTclStreamEvent_Twitch_Subscription>(Event))
	{
		OnChannelSubscribe.Broadcast(Sub, Viewer);
		Tentacle->OnSubscription.Broadcast(Sub->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubEnd = Cast<UTclStreamEvent_Twitch_SubscriptionEnd>(Event))
	{
		OnChannelSubscriptionEnd.Broadcast(SubEnd, Viewer);
	}
	else if (auto* SubGift = Cast<UTclStreamEvent_Twitch_SubscriptionGift>(Event))
	{
		OnChannelSubscriptionGift.Broadcast(SubGift, Viewer);
		Tentacle->OnSubscriptionGift.Broadcast(SubGift->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubMessage = Cast<UTclStreamEvent_Twitch_SubscriptionMessage>(Event))
	{
		OnChannelSubscriptionMessage.Broadcast(SubMessage, Viewer);
		Tentacle->OnSubscriptionRenewal.Broadcast(SubMessage->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SuspiciousMessage = Cast<UTclStreamEvent_Twitch_SuspiciousUserMessage>(Event))
	{
		OnChannelSuspiciousUserMessage.Broadcast(SuspiciousMessage, Viewer);
	}
	else if (auto* SuspiciousUpdate = Cast<UTclStreamEvent_Twitch_SuspiciousUserUpdate>(Event))
	{
		OnChannelSuspiciousUserUpdate.Broadcast(SuspiciousUpdate, Viewer);
	}
	else if (auto* Unban = Cast<UTclStreamEvent_Twitch_Unban>(Event))
	{
		OnChannelUnban.Broadcast(Unban, Viewer);
	}
	else if (auto* UnbanRequestCreate = Cast<UTclStreamEvent_Twitch_UnbanRequestCreate>(Event))
	{
		OnChannelUnbanRequestCreate.Broadcast(UnbanRequestCreate, Viewer);
	}
	else if (auto* UnbanRequestResolve = Cast<UTclStreamEvent_Twitch_UnbanRequestResolve>(Event))
	{
		OnChannelUnbanRequestResolve.Broadcast(UnbanRequestResolve, Viewer);
	}
	else if (auto* ChannelUpdate = Cast<UTclStreamEvent_Twitch_ChannelUpdate>(Event))
	{
		OnChannelUpdate.Broadcast(ChannelUpdate);
		Tentacle->OnChannelUpdate.Broadcast(ChannelUpdate->GetAnywhereEvent());
	}
	else if (auto* VipAdd = Cast<UTclStreamEvent_Twitch_VipAdd>(Event))
	{
		OnChannelVipAdd.Broadcast(VipAdd, Viewer);
	}
	else if (auto* VipRemove = Cast<UTclStreamEvent_Twitch_VipRemove>(Event))
	{
		OnChannelVipRemove.Broadcast(VipRemove, Viewer);
	}
	else if (auto* WarningAcknowledge = Cast<UTclStreamEvent_Twitch_WarningAcknowledge>(Event))
	{
		OnChannelWarningAcknowledgement.Broadcast(WarningAcknowledge, Viewer);
	}
	else if (auto* WarningSend = Cast<UTclStreamEvent_Twitch_WarningSend>(Event))
	{
		OnChannelWarningSend.Broadcast(WarningSend, Viewer);
	}
	else if (auto* CharityProgress = Cast<UTclStreamEvent_Twitch_CharityCampaignProgress>(Event))
	{
		OnCharityCampaignProgress.Broadcast(CharityProgress);
	}
	else if (auto* CharityStart = Cast<UTclStreamEvent_Twitch_CharityCampaignStart>(Event))
	{
		OnCharityCampaignStart.Broadcast(CharityStart);
	}
	else if (auto* CharityStop = Cast<UTclStreamEvent_Twitch_CharityCampaignStop>(Event))
	{
		OnCharityCampaignStop.Broadcast(CharityStop);
	}
	else if (auto* CharityDonation = Cast<UTclStreamEvent_Twitch_CharityDonation>(Event))
	{
		OnCharityDonation.Broadcast(CharityDonation, Viewer);
	}
	else if (auto* GoalBegin = Cast<UTclStreamEvent_Twitch_GoalBegin>(Event))
	{
		OnGoalBegin.Broadcast(GoalBegin);
	}
	else if (auto* GoalEnd = Cast<UTclStreamEvent_Twitch_GoalEnd>(Event))
	{
		OnGoalEnd.Broadcast(GoalEnd);
	}
	else if (auto* GoalProgress = Cast<UTclStreamEvent_Twitch_GoalProgress>(Event))
	{
		OnGoalProgress.Broadcast(GoalProgress);
	}
	else if (auto* HypeTrainBegin = Cast<UTclStreamEvent_Twitch_HypeTrainBegin>(Event))
	{
		OnHypeTrainBegin.Broadcast(HypeTrainBegin);
	}
	else if (auto* HypeTrainEnd = Cast<UTclStreamEvent_Twitch_HypeTrainEnd>(Event))
	{
		OnHypeTrainEnd.Broadcast(HypeTrainEnd);
	}
	else if (auto* HypeTrainProgress = Cast<UTclStreamEvent_Twitch_HypeTrainProgress>(Event))
	{
		OnHypeTrainProgress.Broadcast(HypeTrainProgress);
	}
	else if (auto* ShieldBegin = Cast<UTclStreamEvent_Twitch_ShieldModeBegin>(Event))
	{
		OnShieldModeBegin.Broadcast(ShieldBegin);
	}
	else if (auto* ShieldEnd = Cast<UTclStreamEvent_Twitch_ShieldModeEnd>(Event))
	{
		OnShieldModeEnd.Broadcast(ShieldEnd);
	}
	else if (auto* ShoutoutCreate = Cast<UTclStreamEvent_Twitch_ShoutoutCreate>(Event))
	{
		OnShoutoutCreate.Broadcast(ShoutoutCreate);
	}
	else if (auto* Shoutout = Cast<UTclStreamEvent_Twitch_ShoutoutReceive>(Event))
	{
		OnShoutoutReceived.Broadcast(Shoutout);
	}
	else if (auto* StreamOffline = Cast<UTclStreamEvent_Twitch_StreamOffline>(Event))
	{
		OnStreamOffline.Broadcast(StreamOffline);
	}
	else if (auto* StreamOnline = Cast<UTclStreamEvent_Twitch_StreamOnline>(Event))
	{
		OnStreamOnline.Broadcast(StreamOnline);
	}
	else if (auto* UserUpdate = Cast<UTclStreamEvent_Twitch_UserUpdate>(Event))
	{
		OnUserUpdate.Broadcast(UserUpdate);
	}
	else if (auto* Whisper = Cast<UTclStreamEvent_Twitch_WhisperMessage>(Event))
	{
		OnWhisperReceived.Broadcast(Whisper, Viewer);
	}
	else
	{
		tclDebugF("Tentacle Twitch: Unhandled event kind '%s'", *Event->Kind);
	}
}

// ============================================================================
// Chat Send
// ============================================================================

FString
UTentacle_Twitch::BuildSendChatMessagePayload(const FString& Message, const FString& ReplyParentMessageId)
{
	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetStringField(TEXT("message"), Message);
	if (!ReplyParentMessageId.IsEmpty())
	{
		DataObj->SetStringField(TEXT("replyParentMessageId"), ReplyParentMessageId);
	}

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("Twitch_SendChatMessage"));
	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);
	return JsonString;
}

void
UTentacle_Twitch::SendChatMessage(const FString& Message, const FString& ReplyParentMessageId)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	// Validated here rather than left to Tentacle: a warning in the Unreal log is
	// visible to whoever wrote the Blueprint, a refusal across the socket is not.
	if (Message.IsEmpty())
	{
		tclWarn("Tentacle: Send Chat Message needs a message; nothing sent.");
		return;
	}
	if (Message.Len() > ATentacle::ChatMessageMaxLength)
	{
		tclWarnF("Tentacle: Send Chat Message is %d characters, over the %d limit; nothing sent.", Message.Len(), ATentacle::ChatMessageMaxLength);
		return;
	}

	Tentacle->SendMsg(BuildSendChatMessagePayload(Message, ReplyParentMessageId));
}
