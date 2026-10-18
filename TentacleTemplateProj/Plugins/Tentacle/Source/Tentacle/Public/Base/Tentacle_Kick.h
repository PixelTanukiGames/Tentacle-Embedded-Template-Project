// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sdk/TclKickChatMessageType.h"
#include "Sdk/TclStreamEvent_Kick_Base.h"
#include "Sdk/TclStreamEvent_Kick_ChannelFollowed.h"
#include "Sdk/TclStreamEvent_Kick_ChannelRewardRedemptionUpdated.h"
#include "Sdk/TclStreamEvent_Kick_ChannelSubscriptionGifts.h"
#include "Sdk/TclStreamEvent_Kick_ChannelSubscriptionNew.h"
#include "Sdk/TclStreamEvent_Kick_ChannelSubscriptionRenewal.h"
#include "Sdk/TclStreamEvent_Kick_ChatMessage.h"
#include "Sdk/TclStreamEvent_Kick_LivestreamMetadataUpdated.h"
#include "Sdk/TclStreamEvent_Kick_LivestreamStatusUpdated.h"
#include "Sdk/TclStreamEvent_Kick_ModerationBanned.h"
#include "Tentacle_Kick.generated.h"

class UTclViewer;

/**
 * Kick on its own: every Kick event, one delegate per kind, and the calls that
 * address Kick alone, each named as Kick's own documentation names it (the
 * Channel Follow webhook is On Channel Follow here). Reached as the Tentacle
 * actor's Kick component.
 *
 * The Tentacle actor itself carries the cross-platform side, in Tentacle's own
 * words - On Follow for a follow on any platform, the chat command split, the
 * catch-all, Send Chat Message to every connected platform - and those fire
 * around the Kick delegates here. Listen here for what only Kick has, or to
 * react to Kick alone, and post here (Post Chat Message) to use Kick's own
 * options.
 */
UCLASS(BlueprintType)
class TENTACLE_API UTentacle_Kick : public UActorComponent
{
	GENERATED_BODY()

	// One per event kind, named after its webhook's title in Kick's reference.
	// Events that identify a viewer pass that viewer alongside the event; events
	// about the channel pass the event only.
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelFollow, const UTclStreamEvent_Kick_ChannelFollowed*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelRewardRedemptionUpdated, const UTclStreamEvent_Kick_ChannelRewardRedemptionUpdated*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionCreated, const UTclStreamEvent_Kick_ChannelSubscriptionNew*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionGifts, const UTclStreamEvent_Kick_ChannelSubscriptionGifts*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChannelSubscriptionRenewal, const UTclStreamEvent_Kick_ChannelSubscriptionRenewal*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessage, const UTclStreamEvent_Kick_ChatMessage*, Event, UTclViewer*, Viewer);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLivestreamMetadataUpdated, const UTclStreamEvent_Kick_LivestreamMetadataUpdated*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLivestreamStatusUpdated, const UTclStreamEvent_Kick_LivestreamStatusUpdated*, Event);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModerationBanned, const UTclStreamEvent_Kick_ModerationBanned*, Event, UTclViewer*, Viewer);

public:
	// One per event kind

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnChannelFollow OnChannelFollow;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnChannelRewardRedemptionUpdated OnChannelRewardRedemptionUpdated;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnChannelSubscriptionCreated OnChannelSubscriptionCreated;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnChannelSubscriptionGifts OnChannelSubscriptionGifts;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnChannelSubscriptionRenewal OnChannelSubscriptionRenewal;

	/**
	 * Every Kick chat message, commands included, as Kick sends it. The Tentacle
	 * actor splits chat into On Chat Message and On Chat Command, and fires those
	 * right after this.
	 */
	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnChatMessage OnChatMessage;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnLivestreamMetadataUpdated OnLivestreamMetadataUpdated;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnLivestreamStatusUpdated OnLivestreamStatusUpdated;

	UPROPERTY(BlueprintAssignable, Category="Tentacle|Kick")
	FOnModerationBanned OnModerationBanned;

	// Chat send

	/**
	 * Post a message in the broadcaster's Kick chat: Kick's Post Chat Message.
	 * To send on every connected platform at once, use the Tentacle actor's
	 * Send Chat Message instead.
	 *
	 * Fire-and-forget: nothing is reported back, so a message Kick refuses
	 * surfaces in Tentacle's log rather than in the game.
	 *
	 * @param Content          The message. 1-500 characters; anything else is
	 *                         refused with a warning in the log instead of being sent.
	 * @param Type             Post as the broadcaster (User) or as the channel's bot (Bot).
	 * @param ReplyToMessageId The id of the chat message this one replies to
	 *                         (a Kick chat message's Message Id). Empty for none.
	 */
	UFUNCTION(BlueprintCallable, Category="Tentacle|Kick", meta=(AdvancedDisplay="ReplyToMessageId"))
	void PostChatMessage(
		const FString& Content,
		ETclKickChatMessageType Type = ETclKickChatMessageType::User,
		const FString& ReplyToMessageId = TEXT(""));

	/**
	 * Serialize a Kick_PostChatMessage message for the given arguments. Split out
	 * so the wire contract can be asserted without a live connection; performs
	 * no validation. An empty ReplyToMessageId leaves the reply out.
	 */
	static FString BuildPostChatMessagePayload(
		const FString& Content,
		ETclKickChatMessageType Type,
		const FString& ReplyToMessageId);

	// --- Dispatch (called by ATentacle) ---

	/**
	 * Fire the delegate for one Kick event's kind, then the Tentacle actor's
	 * cross-platform view of it (for a chat message, the actor's chat split).
	 * Called once per event, after de-duplication, the viewer upsert, and the
	 * actor's On Stream Event.
	 */
	void DispatchEvent(UTclStreamEvent_Kick_Base* Event, UTclViewer* Viewer);
};
