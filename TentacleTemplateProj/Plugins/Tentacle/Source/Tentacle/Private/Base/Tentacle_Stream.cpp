// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Stream.h"
#include "Base/Tentacle.h"
#include "Base/Tentacle_Twitch.h"
#include "Base/Tentacle_TwitchExtras.h"
#include "Base/TclViewer.h"
#include "TentacleModule.h"
#include "Sdk/TclMsg_ServerToClient_StreamChatJoin.h"
#include "Sdk/TclMsg_ServerToClient_StreamEvent.h"
#include "Sdk/TclViewerData.h"
// Every concrete UTclStreamEvent_Twitch_* / UTclStreamEvent_Kick_* class the
// dispatchers cast to is pulled in by Base/Tentacle_Stream.h, which needs them
// for its delegate declarations. Only the two platform bases (used for the
// first-level platform test) are named here.
#include "Sdk/TclStreamEvent_Twitch_Base.h"
#include "Sdk/TclStreamEvent_Kick_Base.h"
#include "Sdk/TclHttpClient.h"
#include "Sdk/TclSubathonStats_Subscriptions.h"
#include "Sdk/TclSubathonStats_Donations.h"
#include "Sdk/TclSubathonStats_GiftedSubscriptions.h"
#include "Utilities/TclCommon.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Deprecated. Every stream message is handled once, by ATentacle, which fires
// the Tentacle actor's and the platform components' delegates and then this
// component's, through DispatchDeprecatedEvent / DispatchDeprecatedChatJoin.

// ============================================================================
// Chat Join
// ============================================================================

void
UTentacle_Stream::HandleMsg_ChatJoin(UTclMsg_ServerToClient_StreamChatJoin* Msg)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	Tentacle->HandleChatJoin(Msg);
}

void
UTentacle_Stream::DispatchDeprecatedChatJoin(UTclViewerData* ViewerData, UTclViewer* Viewer)
{
	// The ChatJoin message doesn't have a $platform field directly — determine from viewer data
	if (ViewerData && ViewerData->Twitch)
		OnChatJoin_Twitch.Broadcast(Viewer);
	else if (ViewerData && ViewerData->Kick)
		OnChatJoin_Kick.Broadcast(Viewer);

	// The cross-platform view fires after the platform's own delegate, as the
	// other Anywhere views do, for every join that names a viewer.
	if (ViewerData)
		OnChatJoin_Anywhere.Broadcast(Viewer);
}

// ============================================================================
// Stream Events
// ============================================================================

void
UTentacle_Stream::HandleMsg_Event(UTclMsg_ServerToClient_StreamEvent* Msg)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	Tentacle->HandleStreamEvent(Msg);
}

void
UTentacle_Stream::DispatchDeprecatedEvent(UTclStreamEventBase* Event, UTclViewer* Viewer)
{
	tclSoftAssert(Event);

	// The catch-alls go first: every event, then every event of its platform.
	// De-duplication and the viewer upsert have already happened, so a duplicate
	// never reaches them and the viewer they hand over is the cached one.
	OnEvent_Anywhere.Broadcast(Event, Viewer);

	// Dispatch by platform
	if (auto* TwitchEvent = Cast<UTclStreamEvent_Twitch_Base>(Event))
	{
		OnEvent_Twitch.Broadcast(TwitchEvent, Viewer);
		DispatchEvent_Twitch(Event, Viewer);
	}
	else if (auto* KickEvent = Cast<UTclStreamEvent_Kick_Base>(Event))
	{
		OnEvent_Kick.Broadcast(KickEvent, Viewer);
		DispatchEvent_Kick(Event, Viewer);
	}
}

void
UTentacle_Stream::DispatchEvent_Twitch(UTclStreamEventBase* Event, UTclViewer* Viewer)
{
	// One branch per concrete Twitch event class. All UTclStreamEvent_Twitch_*
	// classes are siblings under UTclStreamEvent_Twitch_Base, so the chain order
	// carries no meaning - it simply mirrors the delegate order in the header.
	// The `else` at the end only fires for an event class the app sends that
	// this plugin version predates.
	if (auto* ChatMsg = Cast<UTclStreamEvent_Twitch_ChatMessage>(Event))
	{
		DispatchChatMessage_Twitch(ChatMsg, Viewer);
	}
	else if (auto* AdBreakBegin = Cast<UTclStreamEvent_Twitch_AdBreakBegin>(Event))
	{
		OnAdBreakBegin_Twitch.Broadcast(AdBreakBegin, Viewer);
	}
	else if (auto* AutomaticRedemption = Cast<UTclStreamEvent_Twitch_AutomaticRewardRedemptionAdd>(Event))
	{
		OnAutomaticRewardRedemptionAdd_Twitch.Broadcast(AutomaticRedemption, Viewer);
	}
	else if (auto* Ban = Cast<UTclStreamEvent_Twitch_Ban>(Event))
	{
		OnBan_Twitch.Broadcast(Ban, Viewer);
		OnBan_Anywhere.Broadcast(Ban->GetAnywhereEvent(), Viewer);
	}
	else if (auto* BitsUse = Cast<UTclStreamEvent_Twitch_BitsUse>(Event))
	{
		OnBitsUse_Twitch.Broadcast(BitsUse, Viewer);
	}
	else if (auto* ChannelUpdate = Cast<UTclStreamEvent_Twitch_ChannelUpdate>(Event))
	{
		OnChannelUpdate_Twitch.Broadcast(ChannelUpdate);
		OnChannelUpdate_Anywhere.Broadcast(ChannelUpdate->GetAnywhereEvent());
	}
	else if (auto* CharityProgress = Cast<UTclStreamEvent_Twitch_CharityCampaignProgress>(Event))
	{
		OnCharityCampaignProgress_Twitch.Broadcast(CharityProgress);
	}
	else if (auto* CharityStart = Cast<UTclStreamEvent_Twitch_CharityCampaignStart>(Event))
	{
		OnCharityCampaignStart_Twitch.Broadcast(CharityStart);
	}
	else if (auto* CharityStop = Cast<UTclStreamEvent_Twitch_CharityCampaignStop>(Event))
	{
		OnCharityCampaignStop_Twitch.Broadcast(CharityStop);
	}
	else if (auto* CharityDonation = Cast<UTclStreamEvent_Twitch_CharityDonation>(Event))
	{
		OnCharityDonation_Twitch.Broadcast(CharityDonation, Viewer);
	}
	else if (auto* ChatClear = Cast<UTclStreamEvent_Twitch_ChatClear>(Event))
	{
		OnChatClear_Twitch.Broadcast(ChatClear);
	}
	else if (auto* ChatClearUser = Cast<UTclStreamEvent_Twitch_ChatClearUserMessages>(Event))
	{
		OnChatClearUserMessages_Twitch.Broadcast(ChatClearUser, Viewer);
	}
	else if (auto* ChatMessageDelete = Cast<UTclStreamEvent_Twitch_ChatMessageDelete>(Event))
	{
		OnChatMessageDelete_Twitch.Broadcast(ChatMessageDelete, Viewer);
	}
	else if (auto* ChatSettings = Cast<UTclStreamEvent_Twitch_ChatSettingsUpdate>(Event))
	{
		OnChatSettingsUpdate_Twitch.Broadcast(ChatSettings);
	}
	else if (auto* Cheer = Cast<UTclStreamEvent_Twitch_Cheer>(Event))
	{
		OnCheer_Twitch.Broadcast(Cheer, Viewer);
		OnDonation_Anywhere.Broadcast(Cheer->GetAnywhereEvent(), Viewer);
	}
	else if (auto* Follow = Cast<UTclStreamEvent_Twitch_Follow>(Event))
	{
		OnFollow_Twitch.Broadcast(Follow, Viewer);
		OnFollow_Anywhere.Broadcast(Follow->GetAnywhereEvent(), Viewer);
	}
	else if (auto* GoalBegin = Cast<UTclStreamEvent_Twitch_GoalBegin>(Event))
	{
		OnGoalBegin_Twitch.Broadcast(GoalBegin);
	}
	else if (auto* GoalEnd = Cast<UTclStreamEvent_Twitch_GoalEnd>(Event))
	{
		OnGoalEnd_Twitch.Broadcast(GoalEnd);
	}
	else if (auto* GoalProgress = Cast<UTclStreamEvent_Twitch_GoalProgress>(Event))
	{
		OnGoalProgress_Twitch.Broadcast(GoalProgress);
	}
	else if (auto* HypeTrainBegin = Cast<UTclStreamEvent_Twitch_HypeTrainBegin>(Event))
	{
		OnHypeTrainBegin_Twitch.Broadcast(HypeTrainBegin);
	}
	else if (auto* HypeTrainEnd = Cast<UTclStreamEvent_Twitch_HypeTrainEnd>(Event))
	{
		OnHypeTrainEnd_Twitch.Broadcast(HypeTrainEnd);
	}
	else if (auto* HypeTrainProgress = Cast<UTclStreamEvent_Twitch_HypeTrainProgress>(Event))
	{
		OnHypeTrainProgress_Twitch.Broadcast(HypeTrainProgress);
	}
	else if (auto* ModeratorAdd = Cast<UTclStreamEvent_Twitch_ModeratorAdd>(Event))
	{
		OnModeratorAdd_Twitch.Broadcast(ModeratorAdd, Viewer);
	}
	else if (auto* ModeratorRemove = Cast<UTclStreamEvent_Twitch_ModeratorRemove>(Event))
	{
		OnModeratorRemove_Twitch.Broadcast(ModeratorRemove, Viewer);
	}
	else if (auto* PollBegin = Cast<UTclStreamEvent_Twitch_PollBegin>(Event))
	{
		OnPollBegin_Twitch.Broadcast(PollBegin);
	}
	else if (auto* PollEnd = Cast<UTclStreamEvent_Twitch_PollEnd>(Event))
	{
		OnPollEnd_Twitch.Broadcast(PollEnd);
	}
	else if (auto* PollProgress = Cast<UTclStreamEvent_Twitch_PollProgress>(Event))
	{
		OnPollProgress_Twitch.Broadcast(PollProgress);
	}
	else if (auto* PredictionBegin = Cast<UTclStreamEvent_Twitch_PredictionBegin>(Event))
	{
		OnPredictionBegin_Twitch.Broadcast(PredictionBegin);
	}
	else if (auto* PredictionEnd = Cast<UTclStreamEvent_Twitch_PredictionEnd>(Event))
	{
		OnPredictionEnd_Twitch.Broadcast(PredictionEnd);
	}
	else if (auto* PredictionLock = Cast<UTclStreamEvent_Twitch_PredictionLock>(Event))
	{
		OnPredictionLock_Twitch.Broadcast(PredictionLock);
	}
	else if (auto* PredictionProgress = Cast<UTclStreamEvent_Twitch_PredictionProgress>(Event))
	{
		OnPredictionProgress_Twitch.Broadcast(PredictionProgress);
	}
	else if (auto* Raid = Cast<UTclStreamEvent_Twitch_Raid>(Event))
	{
		bIsTwitchRaidOngoing = true;
		OnRaid_Twitch.Broadcast(Raid, Viewer);
	}
	else if (auto* RedemptionAdd = Cast<UTclStreamEvent_Twitch_RedemptionAdd>(Event))
	{
		OnRedemptionAdd_Twitch.Broadcast(RedemptionAdd, Viewer);
		OnRedemption_Anywhere.Broadcast(RedemptionAdd->GetAnywhereEvent(), Viewer);
	}
	else if (auto* RedemptionUpdate = Cast<UTclStreamEvent_Twitch_RedemptionUpdate>(Event))
	{
		OnRedemptionUpdate_Twitch.Broadcast(RedemptionUpdate, Viewer);
		OnRedemption_Anywhere.Broadcast(RedemptionUpdate->GetAnywhereEvent(), Viewer);
	}
	else if (auto* RewardAdd = Cast<UTclStreamEvent_Twitch_RewardAdd>(Event))
	{
		OnRewardAdd_Twitch.Broadcast(RewardAdd);
	}
	else if (auto* RewardRemove = Cast<UTclStreamEvent_Twitch_RewardRemove>(Event))
	{
		OnRewardRemove_Twitch.Broadcast(RewardRemove);
	}
	else if (auto* RewardUpdate = Cast<UTclStreamEvent_Twitch_RewardUpdate>(Event))
	{
		OnRewardUpdate_Twitch.Broadcast(RewardUpdate);
	}
	else if (auto* SharedChatBegin = Cast<UTclStreamEvent_Twitch_SharedChatBegin>(Event))
	{
		OnSharedChatBegin_Twitch.Broadcast(SharedChatBegin);
	}
	else if (auto* SharedChatEnd = Cast<UTclStreamEvent_Twitch_SharedChatEnd>(Event))
	{
		OnSharedChatEnd_Twitch.Broadcast(SharedChatEnd);
	}
	else if (auto* SharedChatUpdate = Cast<UTclStreamEvent_Twitch_SharedChatUpdate>(Event))
	{
		OnSharedChatUpdate_Twitch.Broadcast(SharedChatUpdate);
	}
	else if (auto* ShieldBegin = Cast<UTclStreamEvent_Twitch_ShieldModeBegin>(Event))
	{
		OnShieldModeBegin_Twitch.Broadcast(ShieldBegin);
	}
	else if (auto* ShieldEnd = Cast<UTclStreamEvent_Twitch_ShieldModeEnd>(Event))
	{
		OnShieldModeEnd_Twitch.Broadcast(ShieldEnd);
	}
	else if (auto* ShoutoutCreate = Cast<UTclStreamEvent_Twitch_ShoutoutCreate>(Event))
	{
		OnShoutoutCreate_Twitch.Broadcast(ShoutoutCreate);
	}
	else if (auto* Shoutout = Cast<UTclStreamEvent_Twitch_ShoutoutReceive>(Event))
	{
		OnShoutoutReceive_Twitch.Broadcast(Shoutout);
	}
	else if (auto* StreamOnline = Cast<UTclStreamEvent_Twitch_StreamOnline>(Event))
	{
		OnStreamOnline_Twitch.Broadcast(StreamOnline);
	}
	else if (auto* StreamOffline = Cast<UTclStreamEvent_Twitch_StreamOffline>(Event))
	{
		OnStreamOffline_Twitch.Broadcast(StreamOffline);
	}
	else if (auto* Sub = Cast<UTclStreamEvent_Twitch_Subscription>(Event))
	{
		OnSubscription_Twitch.Broadcast(Sub, Viewer);
		OnSubscription_Anywhere.Broadcast(Sub->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubEnd = Cast<UTclStreamEvent_Twitch_SubscriptionEnd>(Event))
	{
		OnSubscriptionEnd_Twitch.Broadcast(SubEnd, Viewer);
	}
	else if (auto* SubGift = Cast<UTclStreamEvent_Twitch_SubscriptionGift>(Event))
	{
		OnSubscriptionGift_Twitch.Broadcast(SubGift, Viewer);
		OnSubscriptionGift_Anywhere.Broadcast(SubGift->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubMessage = Cast<UTclStreamEvent_Twitch_SubscriptionMessage>(Event))
	{
		OnSubscriptionMessage_Twitch.Broadcast(SubMessage, Viewer);
		OnSubscriptionRenewal_Anywhere.Broadcast(SubMessage->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SuspiciousMessage = Cast<UTclStreamEvent_Twitch_SuspiciousUserMessage>(Event))
	{
		OnSuspiciousUserMessage_Twitch.Broadcast(SuspiciousMessage, Viewer);
	}
	else if (auto* SuspiciousUpdate = Cast<UTclStreamEvent_Twitch_SuspiciousUserUpdate>(Event))
	{
		OnSuspiciousUserUpdate_Twitch.Broadcast(SuspiciousUpdate, Viewer);
	}
	else if (auto* Unban = Cast<UTclStreamEvent_Twitch_Unban>(Event))
	{
		OnUnban_Twitch.Broadcast(Unban, Viewer);
	}
	else if (auto* UnbanRequestCreate = Cast<UTclStreamEvent_Twitch_UnbanRequestCreate>(Event))
	{
		OnUnbanRequestCreate_Twitch.Broadcast(UnbanRequestCreate, Viewer);
	}
	else if (auto* UnbanRequestResolve = Cast<UTclStreamEvent_Twitch_UnbanRequestResolve>(Event))
	{
		OnUnbanRequestResolve_Twitch.Broadcast(UnbanRequestResolve, Viewer);
	}
	else if (auto* UserUpdate = Cast<UTclStreamEvent_Twitch_UserUpdate>(Event))
	{
		OnUserUpdate_Twitch.Broadcast(UserUpdate);
	}
	else if (auto* VipAdd = Cast<UTclStreamEvent_Twitch_VipAdd>(Event))
	{
		OnVipAdd_Twitch.Broadcast(VipAdd, Viewer);
	}
	else if (auto* VipRemove = Cast<UTclStreamEvent_Twitch_VipRemove>(Event))
	{
		OnVipRemove_Twitch.Broadcast(VipRemove, Viewer);
	}
	else if (auto* WarningAcknowledge = Cast<UTclStreamEvent_Twitch_WarningAcknowledge>(Event))
	{
		OnWarningAcknowledge_Twitch.Broadcast(WarningAcknowledge, Viewer);
	}
	else if (auto* WarningSend = Cast<UTclStreamEvent_Twitch_WarningSend>(Event))
	{
		OnWarningSend_Twitch.Broadcast(WarningSend, Viewer);
	}
	else if (auto* Whisper = Cast<UTclStreamEvent_Twitch_WhisperMessage>(Event))
	{
		OnWhisperMessage_Twitch.Broadcast(Whisper, Viewer);
	}
	else
	{
		tclDebugF("Tentacle Stream: Unhandled Twitch event kind '%s'", *Event->Kind);
	}
}

void
UTentacle_Stream::DispatchEvent_Kick(UTclStreamEventBase* Event, UTclViewer* Viewer)
{
	if (auto* ChatMsg = Cast<UTclStreamEvent_Kick_ChatMessage>(Event))
	{
		DispatchChatMessage_Kick(ChatMsg, Viewer);
	}
	else if (auto* Followed = Cast<UTclStreamEvent_Kick_ChannelFollowed>(Event))
	{
		OnChannelFollowed_Kick.Broadcast(Followed, Viewer);
		OnFollow_Anywhere.Broadcast(Followed->GetAnywhereEvent(), Viewer);
	}
	else if (auto* RewardRedemption = Cast<UTclStreamEvent_Kick_ChannelRewardRedemptionUpdated>(Event))
	{
		OnChannelRewardRedemptionUpdated_Kick.Broadcast(RewardRedemption, Viewer);
		OnRedemption_Anywhere.Broadcast(RewardRedemption->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubNew = Cast<UTclStreamEvent_Kick_ChannelSubscriptionNew>(Event))
	{
		OnChannelSubscriptionNew_Kick.Broadcast(SubNew, Viewer);
		OnSubscription_Anywhere.Broadcast(SubNew->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubGifts = Cast<UTclStreamEvent_Kick_ChannelSubscriptionGifts>(Event))
	{
		OnChannelSubscriptionGifts_Kick.Broadcast(SubGifts, Viewer);
		OnSubscriptionGift_Anywhere.Broadcast(SubGifts->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubRenewal = Cast<UTclStreamEvent_Kick_ChannelSubscriptionRenewal>(Event))
	{
		OnChannelSubscriptionRenewal_Kick.Broadcast(SubRenewal, Viewer);
		// Renewals map to the Anywhere renewal view (paired with Twitch's resub
		// message), not to the Anywhere subscription view, which is first-time
		// subscriptions only. This matches the app's Anywhere event mapping.
		OnSubscriptionRenewal_Anywhere.Broadcast(SubRenewal->GetAnywhereEvent(), Viewer);
	}
	else if (auto* MetadataUpdated = Cast<UTclStreamEvent_Kick_LivestreamMetadataUpdated>(Event))
	{
		OnLivestreamMetadataUpdated_Kick.Broadcast(MetadataUpdated);
		OnChannelUpdate_Anywhere.Broadcast(MetadataUpdated->GetAnywhereEvent());
	}
	else if (auto* Livestream = Cast<UTclStreamEvent_Kick_LivestreamStatusUpdated>(Event))
	{
		OnLivestreamStatusUpdated_Kick.Broadcast(Livestream);
	}
	else if (auto* ModBanned = Cast<UTclStreamEvent_Kick_ModerationBanned>(Event))
	{
		OnModerationBanned_Kick.Broadcast(ModBanned, Viewer);
		OnBan_Anywhere.Broadcast(ModBanned->GetAnywhereEvent(), Viewer);
	}
	else
	{
		tclDebugF("Tentacle Stream: Unhandled Kick event kind '%s'", *Event->Kind);
	}
}

void
UTentacle_Stream::DispatchChatMessage_Twitch(UTclStreamEvent_Twitch_ChatMessage* ChatMsg, UTclViewer* Viewer)
{
	auto* Anywhere = ChatMsg->GetAnywhereEvent();
	const bool bHasCommand = Anywhere && !Anywhere->Command.IsEmpty();

	if (bHasCommand)
		OnChatCommand_Twitch.Broadcast(ChatMsg, Viewer);
	else
		OnChatMessage_Twitch.Broadcast(ChatMsg, Viewer);

	if (Anywhere && !Anywhere->Emojis.IsEmpty())
		OnChatMessageWithEmojis_Twitch.Broadcast(ChatMsg, Viewer);

	if (Anywhere && !Anywhere->Emotes.IsEmpty())
		OnChatMessageWithEmotes_Twitch.Broadcast(ChatMsg, Viewer);

	if (Anywhere)
		DispatchChatMessage_Anywhere(Anywhere, Viewer);
}

void
UTentacle_Stream::DispatchChatMessage_Kick(UTclStreamEvent_Kick_ChatMessage* ChatMsg, UTclViewer* Viewer)
{
	auto* Anywhere = ChatMsg->GetAnywhereEvent();
	const bool bHasCommand = Anywhere && !Anywhere->Command.IsEmpty();

	if (bHasCommand)
		OnChatCommand_Kick.Broadcast(ChatMsg, Viewer);
	else
		OnChatMessage_Kick.Broadcast(ChatMsg, Viewer);

	if (Anywhere && !Anywhere->Emojis.IsEmpty())
		OnChatMessageWithEmojis_Kick.Broadcast(ChatMsg, Viewer);

	if (Anywhere && !Anywhere->Emotes.IsEmpty())
		OnChatMessageWithEmotes_Kick.Broadcast(ChatMsg, Viewer);

	if (Anywhere)
		DispatchChatMessage_Anywhere(Anywhere, Viewer);
}

void
UTentacle_Stream::DispatchChatMessage_Anywhere(UTclStreamEvent_Anywhere_ChatMessage* ChatMsg, UTclViewer* Viewer)
{
	if (!ChatMsg->Command.IsEmpty())
		OnChatCommand_Anywhere.Broadcast(ChatMsg, Viewer);
	else
		OnChatMessage_Anywhere.Broadcast(ChatMsg, Viewer);

	if (!ChatMsg->Emojis.IsEmpty())
		OnChatMessageWithEmojis_Anywhere.Broadcast(ChatMsg, Viewer);

	if (!ChatMsg->Emotes.IsEmpty())
		OnChatMessageWithEmotes_Anywhere.Broadcast(ChatMsg, Viewer);
}

// ============================================================================
// Raid
// ============================================================================

void
UTentacle_Stream::EndRaid_Twitch()
{
	// The Twitch component's Extras own the raid window, and closing it there
	// closes this copy too (MirrorRaidEnded).
	auto* Tentacle = GetOwner<ATentacle>();
	if (Tentacle && Tentacle->Twitch && Tentacle->Twitch->Extras)
	{
		Tentacle->Twitch->Extras->EndIncomingRaid();
		return;
	}

	MirrorRaidEnded();
}

void
UTentacle_Stream::MirrorRaidEnded()
{
	bIsTwitchRaidOngoing = false;
	OnRaidEnded_Twitch.Broadcast();
}

// ============================================================================
// Subathon
// ============================================================================

void
UTentacle_Stream::FetchTwitchSubathonDonations()
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	Tentacle->HttpClient->Donations(
		TEXT(""),
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self)](const FString& ResponseJson)
		{
			tclLambdaGet(Self);

			// Parse the tRPC response wrapper: { "result": { "data": { ... } } }
			TSharedPtr<FJsonObject> Root;
			const auto Reader = TJsonReaderFactory<>::Create(ResponseJson);
			if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
			{
				const auto ResultObj = Root->GetObjectField(TEXT("result"));
				if (ResultObj.IsValid())
				{
					const auto DataObj = ResultObj->GetObjectField(TEXT("data"));
					if (DataObj.IsValid())
					{
						const float Dollars = DataObj->GetNumberField(TEXT("dollars"));
						const float Goal = DataObj->GetNumberField(TEXT("goal"));
						const float GoalProgress = DataObj->GetNumberField(TEXT("goalProgress"));
						Self->OnFetchSubathonDonationsComplete_Twitch.Broadcast(Dollars, Goal, GoalProgress);
					}
				}
			}
		}),
		UTentacleHttpClient::FOnError::CreateLambda([](int32, const FString& Err)
		{
			UE_LOG(LogTentacle, Warning, TEXT("Tentacle Stream: Subathon donations fetch failed: %s"), *Err);
		})
	);
}

void
UTentacle_Stream::FetchTwitchSubathonGiftedSubscriptions()
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	Tentacle->HttpClient->GiftedSubs(
		TEXT(""),
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self)](const FString& ResponseJson)
		{
			tclLambdaGet(Self);

			TSharedPtr<FJsonObject> Root;
			const auto Reader = TJsonReaderFactory<>::Create(ResponseJson);
			if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
			{
				const auto ResultObj = Root->GetObjectField(TEXT("result"));
				if (ResultObj.IsValid())
				{
					const auto DataObj = ResultObj->GetObjectField(TEXT("data"));
					if (DataObj.IsValid())
					{
						const int32 Count = DataObj->GetIntegerField(TEXT("count"));
						const int32 Goal = DataObj->GetIntegerField(TEXT("goal"));
						const float GoalProgress = DataObj->GetNumberField(TEXT("goalProgress"));
						Self->OnFetchSubathonGiftedSubscriptionsComplete_Twitch.Broadcast(Count, Goal, GoalProgress);
					}
				}
			}
		}),
		UTentacleHttpClient::FOnError::CreateLambda([](int32, const FString& Err)
		{
			UE_LOG(LogTentacle, Warning, TEXT("Tentacle Stream: Subathon gifted subs fetch failed: %s"), *Err);
		})
	);
}

void
UTentacle_Stream::FetchTwitchSubathonSubscriptions()
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Tentacle->HttpClient);

	Tentacle->HttpClient->Subs(
		TEXT(""),
		UTentacleHttpClient::FOnSuccess::CreateLambda([tclLambdaCapture(this, Self)](const FString& ResponseJson)
		{
			tclLambdaGet(Self);

			TSharedPtr<FJsonObject> Root;
			const auto Reader = TJsonReaderFactory<>::Create(ResponseJson);
			if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
			{
				const auto ResultObj = Root->GetObjectField(TEXT("result"));
				if (ResultObj.IsValid())
				{
					const auto DataObj = ResultObj->GetObjectField(TEXT("data"));
					if (DataObj.IsValid())
					{
						const int32 Count = DataObj->GetIntegerField(TEXT("count"));
						const int32 Goal = DataObj->GetIntegerField(TEXT("goal"));
						const float GoalProgress = DataObj->GetNumberField(TEXT("goalProgress"));
						Self->OnFetchSubathonSubscriptionsComplete_Twitch.Broadcast(Count, Goal, GoalProgress);
					}
				}
			}
		}),
		UTentacleHttpClient::FOnError::CreateLambda([](int32, const FString& Err)
		{
			UE_LOG(LogTentacle, Warning, TEXT("Tentacle Stream: Subathon subs fetch failed: %s"), *Err);
		})
	);
}

// ============================================================================
// Chat Send
// ============================================================================
//
// Deprecated; each forwards to the Tentacle actor, which sends and validates
// exactly as it always did here.

int32
UTentacle_Stream::GetChatMessageMaxLength()
{
	return ATentacle::GetChatMessageMaxLength();
}

void
UTentacle_Stream::SendChatMessage(const FString& Message, ETclChatSendTarget Target, bool bIsBot)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	Tentacle->SendChatMessageToTarget(Message, Target, bIsBot);
}

void
UTentacle_Stream::SendChatMessageToViewer(UTclViewer* Viewer, const FString& Message, ETclChatSendTarget Target, bool bIsBot)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	Tentacle->SendChatMessageToViewerOnTarget(Viewer, Message, Target, bIsBot);
}

FString
UTentacle_Stream::BuildChatMessagePayload(const FString& Message, ETclChatSendTarget Target, bool bIsBot, const FString& ViewerId)
{
	return ATentacle::BuildChatMessagePayload(Message, Target, bIsBot, ViewerId);
}
