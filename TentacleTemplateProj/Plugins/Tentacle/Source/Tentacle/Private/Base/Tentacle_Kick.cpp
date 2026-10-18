// © 2025 Eldritch One. All rights reserved.

#include "Base/Tentacle_Kick.h"
#include "Base/Tentacle.h"
#include "Base/TclViewer.h"
#include "Dom/JsonObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Utilities/TclCommon.h"

// ============================================================================
// Dispatch
// ============================================================================

void
UTentacle_Kick::DispatchEvent(UTclStreamEvent_Kick_Base* Event, UTclViewer* Viewer)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle && Event);

	if (auto* ChatMsg = Cast<UTclStreamEvent_Kick_ChatMessage>(Event))
	{
		// Kick's own event is the whole message, commands included; the split
		// into message and command is the Tentacle actor's.
		OnChatMessage.Broadcast(ChatMsg, Viewer);
		if (auto* Anywhere = ChatMsg->GetAnywhereEvent())
			Tentacle->DispatchChatMessage(Anywhere, Viewer);
	}
	else if (auto* Followed = Cast<UTclStreamEvent_Kick_ChannelFollowed>(Event))
	{
		OnChannelFollow.Broadcast(Followed, Viewer);
		Tentacle->OnFollow.Broadcast(Followed->GetAnywhereEvent(), Viewer);
	}
	else if (auto* RewardRedemption = Cast<UTclStreamEvent_Kick_ChannelRewardRedemptionUpdated>(Event))
	{
		OnChannelRewardRedemptionUpdated.Broadcast(RewardRedemption, Viewer);
		Tentacle->OnRedemption.Broadcast(RewardRedemption->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubNew = Cast<UTclStreamEvent_Kick_ChannelSubscriptionNew>(Event))
	{
		OnChannelSubscriptionCreated.Broadcast(SubNew, Viewer);
		Tentacle->OnSubscription.Broadcast(SubNew->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubGifts = Cast<UTclStreamEvent_Kick_ChannelSubscriptionGifts>(Event))
	{
		OnChannelSubscriptionGifts.Broadcast(SubGifts, Viewer);
		Tentacle->OnSubscriptionGift.Broadcast(SubGifts->GetAnywhereEvent(), Viewer);
	}
	else if (auto* SubRenewal = Cast<UTclStreamEvent_Kick_ChannelSubscriptionRenewal>(Event))
	{
		OnChannelSubscriptionRenewal.Broadcast(SubRenewal, Viewer);
		// A renewal is the cross-platform renewal view (paired with Twitch's resub
		// message), not the subscription view, which is first-time subscriptions
		// only. This matches the app's cross-platform event mapping.
		Tentacle->OnSubscriptionRenewal.Broadcast(SubRenewal->GetAnywhereEvent(), Viewer);
	}
	else if (auto* MetadataUpdated = Cast<UTclStreamEvent_Kick_LivestreamMetadataUpdated>(Event))
	{
		OnLivestreamMetadataUpdated.Broadcast(MetadataUpdated);
		Tentacle->OnChannelUpdate.Broadcast(MetadataUpdated->GetAnywhereEvent());
	}
	else if (auto* Livestream = Cast<UTclStreamEvent_Kick_LivestreamStatusUpdated>(Event))
	{
		OnLivestreamStatusUpdated.Broadcast(Livestream);
	}
	else if (auto* ModBanned = Cast<UTclStreamEvent_Kick_ModerationBanned>(Event))
	{
		OnModerationBanned.Broadcast(ModBanned, Viewer);
		Tentacle->OnBan.Broadcast(ModBanned->GetAnywhereEvent(), Viewer);
	}
	else
	{
		tclDebugF("Tentacle Kick: Unhandled event kind '%s'", *Event->Kind);
	}
}

// ============================================================================
// Chat Send
// ============================================================================

FString
UTentacle_Kick::BuildPostChatMessagePayload(const FString& Content, ETclKickChatMessageType Type, const FString& ReplyToMessageId)
{
	const TSharedRef<FJsonObject> DataObj = MakeShared<FJsonObject>();
	DataObj->SetStringField(TEXT("content"), Content);
	// Kick's `type` travels as `messageType`, beside the message's own `type`.
	DataObj->SetStringField(TEXT("messageType"), Type == ETclKickChatMessageType::Bot ? TEXT("bot") : TEXT("user"));
	if (!ReplyToMessageId.IsEmpty())
	{
		DataObj->SetStringField(TEXT("replyToMessageId"), ReplyToMessageId);
	}

	const TSharedRef<FJsonObject> MsgObj = MakeShared<FJsonObject>();
	MsgObj->SetStringField(TEXT("type"), TEXT("Kick_PostChatMessage"));
	MsgObj->SetObjectField(TEXT("data"), DataObj);

	FString JsonString;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString);
	FJsonSerializer::Serialize(MsgObj, Writer);
	return JsonString;
}

void
UTentacle_Kick::PostChatMessage(const FString& Content, ETclKickChatMessageType Type, const FString& ReplyToMessageId)
{
	auto* Tentacle = GetOwner<ATentacle>();
	tclSoftAssert(Tentacle);

	// Validated here rather than left to Tentacle: a warning in the Unreal log is
	// visible to whoever wrote the Blueprint, a refusal across the socket is not.
	if (Content.IsEmpty())
	{
		tclWarn("Tentacle: Post Chat Message needs content; nothing posted.");
		return;
	}
	if (Content.Len() > ATentacle::ChatMessageMaxLength)
	{
		tclWarnF("Tentacle: Post Chat Message is %d characters, over the %d limit; nothing posted.", Content.Len(), ATentacle::ChatMessageMaxLength);
		return;
	}

	Tentacle->SendMsg(BuildPostChatMessagePayload(Content, Type, ReplyToMessageId));
}
