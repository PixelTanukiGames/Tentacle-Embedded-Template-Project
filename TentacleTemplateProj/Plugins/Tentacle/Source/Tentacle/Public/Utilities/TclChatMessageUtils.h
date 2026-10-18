// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sdk/TclEmojiImageSet.h"
#include "TclChatMessageUtils.generated.h"

class UTclChatMessageEmoji;
class UTclStreamEvent_Anywhere_ChatMessage;
class UTclStreamEvent_Twitch_ChatMessage;

/**
 * Utility functions for querying emoji/emote counts on the cross-platform
 * "Anywhere" view of chat-message events. Operates on UTclStreamEvent_Anywhere_ChatMessage,
 * which carries the SDK-normalized $-prefixed fields shared between Twitch and Kick.
 * Also reads the roles a Twitch chat message's own badges stand for, as a
 * viewer's Is Moderator (Twitch) and its siblings read the badges they last wore.
 */
UCLASS()
class TENTACLE_API UTclChatMessageUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Get the count of a specific emoji by exact name. Returns 0 if not found.
	 *
	 * The plain name ("thumbs_up") counts every skin tone of that emoji, and so
	 * does any of its aliases ("+1"); a tone-qualified key ("thumbs_up-light")
	 * counts just that one tone.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static int32
	GetEmojiCount(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Name, bool bCaseSensitive = false);

	/** Get the count of a specific emote by exact name. Returns 0 if not found. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static int32
	GetEmoteCount(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Name, bool bCaseSensitive = false);

	/** Sum counts of all emojis whose name contains the given substring. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static int32
	GetEmojiCountContaining(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Substring, bool bCaseSensitive = false);

	/** Sum counts of all emotes whose name contains the given substring. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static int32
	GetEmoteCountContaining(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Substring, bool bCaseSensitive = false);

	/** Check if the chat message has any emojis. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static bool
	HasEmojis(const UTclStreamEvent_Anywhere_ChatMessage* Message);

	/** Check if the chat message has any emotes. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static bool
	HasEmotes(const UTclStreamEvent_Anywhere_ChatMessage* Message);

	/** Whether a Twitch chat message wears a badge of this set, such as "bits". False for an invalid message. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils", meta=(DisplayName="Has Badge (Twitch)"))
	static bool
	HasBadge_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message, const FString& SetId, bool bCaseSensitive = false);

	/** Whether a Twitch chat message is the broadcaster's. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils", meta=(DisplayName="Is Broadcaster (Twitch)"))
	static bool
	IsBroadcaster_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message);

	/** Whether a Twitch chat message is a moderator's, Lead Moderators included. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils", meta=(DisplayName="Is Moderator (Twitch)"))
	static bool
	IsModerator_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message);

	/** Whether a Twitch chat message is a VIP's. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils", meta=(DisplayName="Is Vip (Twitch)"))
	static bool
	IsVip_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message);

	/** Whether a Twitch chat message wears the subscriber or founder badge. */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils", meta=(DisplayName="Is Subscriber (Twitch)"))
	static bool
	IsSubscriber_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message);

	/**
	 * Get the Unicode code points of an emoji string, formatted "U+1F600"
	 * (uppercase hex, min width 4, "U+" prefix). Correctly handles multi-code-point
	 * emoji (ZWJ sequences, skin-tone modifiers) and UTF-16 surrogate pairs.
	 * Empty input returns an empty array.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static TArray<FString>
	GetEmojiCodePoints(const FString& Emoji);

	/**
	 * Get the Unicode code points of an emoji string, formatted "1f600"
	 * (lowercase hex, min width 4, no prefix). Correctly handles
	 * multi-code-point emoji (ZWJ sequences, skin-tone modifiers) and UTF-16
	 * surrogate pairs. Empty input returns an empty array. To name an emoji's
	 * image file, use Get Emoji Image File Name, which follows each image set's
	 * own naming rules.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static TArray<FString>
	GetEmojiHexValues(const FString& Emoji);

	/**
	 * Get the file name of an emoji's PNG image in a copy of one of the open
	 * emoji image sets you ship or host yourself: "emoji_u2764.png" in Noto's
	 * own download, "2764.png" in Twemoji and OpenMoji.
	 *
	 * Each set names its files its own way, down to how it treats the invisible
	 * variation selector, so the name comes from here rather than from the code
	 * points joined together. Noto's download names flags by region code
	 * ("US.png", "GB-ENG.png") and keeps them in a folder of their own. To load
	 * an image straight from the set, use Get Emoji Image Url. Returns an empty
	 * string for an invalid emoji.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static FString
	GetEmojiImageFileName(const UTclChatMessageEmoji* Emoji, ETclEmojiImageSet ImageSet);

	/**
	 * Get the address of an emoji's PNG image in one of the open emoji image
	 * sets.
	 *
	 * Size asks for an image at least that many pixels across: the address is
	 * for the smallest size the set ships that is as big, or its largest when
	 * none is. Noto ships 32, 72, 128, and 512 pixels, Twemoji 72, and OpenMoji
	 * 72 and 618.
	 *
	 * Noto's images come from Google's own Noto CDN, the rest from the set's
	 * release on jsDelivr. Each address names a release of the set, so it does
	 * not move. A set may not have drawn the newest emoji yet, so treat an image
	 * that fails to download as normal. Returns an empty string for an invalid
	 * emoji.
	 */
	UFUNCTION(BlueprintPure, Category="Tentacle|Chat Message Utils")
	static FString
	GetEmojiImageUrl(const UTclChatMessageEmoji* Emoji, ETclEmojiImageSet ImageSet, int32 Size = 512);

private:
	/** Walk a UTF-16 FString and yield each Unicode code point, combining surrogate pairs. */
	static TArray<uint32>
	CodePointsOf(const FString& S);
};
