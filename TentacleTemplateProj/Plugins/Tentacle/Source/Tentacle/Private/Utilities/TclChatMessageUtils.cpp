// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclChatMessageUtils.h"
#include "Misc/Parse.h"
#include "Sdk/TclStreamEvent_Anywhere_ChatMessage.h"
#include "Sdk/TclStreamEvent_Twitch_ChatMessage.h"
#include "Sdk/TclChatMessageEmoji.h"
#include "Sdk/TclChatMessageEmote.h"
#include "Utilities/TclBadges.h"

// The same rules as tentacle-web's src/emoji.ts and the Unity and Godot
// plugins, held together by the same table of cases in each one's tests. A
// named namespace rather than an anonymous one, because a unity build merges
// this file with others that have helpers of their own.
namespace TclEmojiImageFileNames
{
	/** VARIATION SELECTOR-16, which asks for the colour emoji rather than the text glyph. */
	constexpr uint32 VariationSelector = 0xFE0F;

	/** ZERO WIDTH JOINER, which welds several emoji into one. */
	constexpr uint32 ZeroWidthJoiner = 0x200D;

	/** The regional indicator letters a country flag is spelled with, A to Z. */
	constexpr uint32 RegionalIndicatorA = 0x1F1E6;
	constexpr uint32 RegionalIndicatorZ = 0x1F1FF;

	/** WAVING BLACK FLAG, the base of a subdivision flag such as England's. */
	constexpr uint32 BlackFlag = 0x1F3F4;

	/** CANCEL TAG, which ends the tag letters a subdivision flag is spelled with. */
	constexpr uint32 CancelTag = 0xE007F;

	/** How far a tag character sits above the ASCII character it stands for. */
	constexpr uint32 TagOffset = 0xE0000;

	// Twemoji files one emoji against its own rule: eye in speech bubble keeps no
	// variation selector even though it has a joiner. It is the only one in
	// Emoji 18.0, checked against every file Twemoji v17.0.3 ships.
	constexpr uint32 TwemojiEyeInSpeechBubble[] = { 0x1F441, 0xFE0F, 0x200D, 0x1F5E8, 0xFE0F };

	// The folder each set's PNG images are in, pinned to a release so the address
	// cannot move under a game. Noto's are Google's own Noto CDN, by emoji
	// version, where flags are drawn as emoji at every size; the others are the
	// set's GitHub release on jsDelivr. tentacle-web's tests/emoji-urls.test.ts
	// requests real images from each, at every size, and fails when these differ
	// from its own.
	const TCHAR* const NotoFolder = TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/");
	const TCHAR* const TwemojiFolder = TEXT("https://cdn.jsdelivr.net/gh/jdecked/twemoji@v17.0.3/assets/");
	const TCHAR* const OpenMojiFolder = TEXT("https://cdn.jsdelivr.net/gh/hfg-gmuend/openmoji@17.0.0/color/");

	// The sizes, in pixels, each set ships its PNG images in, smallest first.
	constexpr int32 NotoSizes[] = { 32, 72, 128, 512 };
	constexpr int32 TwemojiSizes[] = { 72 };
	constexpr int32 OpenMojiSizes[] = { 72, 618 };

	/** The smallest of a set's sizes that is at least Size pixels, or its largest when none is. */
	template <int32 Count>
	int32
	ImageSizeFor(const int32 (&Sizes)[Count], int32 Size)
	{
		for (const int32 Available : Sizes)
		{
			if (Available >= Size) return Available;
		}
		return Sizes[Count - 1];
	}

	/** Read hex code points into their values. False when there are none, or one is not written in hex. */
	bool
	ParseCodePoints(const TArray<FString>& CodePoints, TArray<uint32>& OutValues)
	{
		for (const FString& CodePoint : CodePoints)
		{
			if (CodePoint.IsEmpty() || CodePoint.Len() > 6) return false;
			for (const TCHAR Character : CodePoint)
			{
				if (!FChar::IsHexDigit(Character)) return false;
			}
			OutValues.Add(FParse::HexNumber(*CodePoint));
		}
		return !OutValues.IsEmpty();
	}

	/** The two letters a country flag spells ("US"), or empty when the emoji is not one. */
	FString
	CountryFlagLetters(const TArray<uint32>& Values)
	{
		if (Values.Num() != 2) return FString();
		FString Letters;
		for (const uint32 Value : Values)
		{
			if (Value < RegionalIndicatorA || Value > RegionalIndicatorZ) return FString();
			Letters.AppendChar(static_cast<TCHAR>(TEXT('A') + (Value - RegionalIndicatorA)));
		}
		return Letters;
	}

	/** The region and subdivision a subdivision flag spells ("GB-ENG"), or empty when the emoji is not one. */
	FString
	SubdivisionFlagCode(const TArray<uint32>& Values)
	{
		if (Values.Num() < 3 || Values[0] != BlackFlag || Values.Last() != CancelTag) return FString();
		FString Letters;
		for (int32 Index = 1; Index < Values.Num() - 1; ++Index)
		{
			Letters.AppendChar(static_cast<TCHAR>(Values[Index] - TagOffset));
		}
		Letters = Letters.ToUpper();
		return Letters.Left(2) + TEXT("-") + Letters.Mid(2);
	}

	/**
	 * How Noto names an emoji: its code points in lowercase, joined with
	 * underscores, with every variation selector dropped ("2764_200d_1f525").
	 */
	FString
	NotoSequenceName(const TArray<uint32>& Values)
	{
		TArray<FString> Parts;
		for (const uint32 Value : Values)
		{
			if (Value != VariationSelector) Parts.Add(FString::Printf(TEXT("%04x"), Value));
		}
		return FString::Join(Parts, TEXT("_"));
	}

	/**
	 * The name of a file in Noto's own download: "emoji_u" and the emoji's Noto
	 * name. Flags are the exception: the download keeps them apart, named by
	 * region code.
	 */
	FString
	NotoFileName(const TArray<uint32>& Values)
	{
		FString Flag = CountryFlagLetters(Values);
		if (Flag.IsEmpty()) Flag = SubdivisionFlagCode(Values);
		if (!Flag.IsEmpty()) return Flag + TEXT(".png");
		return TEXT("emoji_u") + NotoSequenceName(Values) + TEXT(".png");
	}

	/**
	 * Twemoji's name: the code points in lowercase without leading zeros, joined
	 * with hyphens, with the variation selector dropped unless the emoji has a
	 * joiner.
	 */
	FString
	TwemojiFileName(const TArray<uint32>& Values)
	{
		bool bIsException = Values.Num() == UE_ARRAY_COUNT(TwemojiEyeInSpeechBubble);
		for (int32 Index = 0; bIsException && Index < Values.Num(); ++Index)
		{
			bIsException = Values[Index] == TwemojiEyeInSpeechBubble[Index];
		}
		const bool bKeepSelector = Values.Contains(ZeroWidthJoiner) && !bIsException;

		TArray<FString> Parts;
		for (const uint32 Value : Values)
		{
			if (bKeepSelector || Value != VariationSelector) Parts.Add(FString::Printf(TEXT("%x"), Value));
		}
		return FString::Join(Parts, TEXT("-")) + TEXT(".png");
	}

	/**
	 * OpenMoji's name: the code points in uppercase, joined with hyphens, with
	 * the variation selector dropped only from an emoji that is one character and
	 * the selector.
	 */
	FString
	OpenMojiFileName(const TArray<uint32>& Values)
	{
		const bool bDropSelector = Values.Num() == 2 && Values[1] == VariationSelector;
		const int32 Kept = bDropSelector ? 1 : Values.Num();

		TArray<FString> Parts;
		for (int32 Index = 0; Index < Kept; ++Index)
		{
			Parts.Add(FString::Printf(TEXT("%04X"), Values[Index]));
		}
		return FString::Join(Parts, TEXT("-")) + TEXT(".png");
	}
}

int32
UTclChatMessageUtils::GetEmojiCount(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Name, bool bCaseSensitive)
{
	if (!IsValid(Message)) return 0;

	// A skin-toned emoji is keyed by its tone ("thumbs_up-light") and carries the
	// tone-free name ("thumbs_up"), so matching the key counts one tone while
	// matching the name counts every tone of that emoji. An alias is another
	// name for the same emoji ("+1"), so it counts the same as the name.
	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	int32 Total = 0;
	for (const auto& [Key, Emoji] : Message->Emojis)
	{
		if (!IsValid(Emoji)) continue;
		const bool bMatches = Key.Equals(Name, SearchCase)
			|| Emoji->Name.Equals(Name, SearchCase)
			|| Emoji->Aliases.ContainsByPredicate([&](const FString& Alias) { return Alias.Equals(Name, SearchCase); });
		if (bMatches) Total += Emoji->Count;
	}
	return Total;
}

int32
UTclChatMessageUtils::GetEmoteCount(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Name, bool bCaseSensitive)
{
	if (!IsValid(Message)) return 0;

	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	for (const auto& [Key, Emote] : Message->Emotes)
	{
		if (IsValid(Emote) && Key.Equals(Name, SearchCase))
			return Emote->Count;
	}
	return 0;
}

int32
UTclChatMessageUtils::GetEmojiCountContaining(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Substring, bool bCaseSensitive)
{
	if (!IsValid(Message)) return 0;

	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	int32 Total = 0;
	for (const auto& [Key, Emoji] : Message->Emojis)
	{
		if (IsValid(Emoji) && Key.Contains(Substring, SearchCase))
			Total += Emoji->Count;
	}
	return Total;
}

int32
UTclChatMessageUtils::GetEmoteCountContaining(const UTclStreamEvent_Anywhere_ChatMessage* Message, const FString& Substring, bool bCaseSensitive)
{
	if (!IsValid(Message)) return 0;

	const ESearchCase::Type SearchCase = bCaseSensitive ? ESearchCase::CaseSensitive : ESearchCase::IgnoreCase;
	int32 Total = 0;
	for (const auto& [Key, Emote] : Message->Emotes)
	{
		if (IsValid(Emote) && Key.Contains(Substring, SearchCase))
			Total += Emote->Count;
	}
	return Total;
}

bool
UTclChatMessageUtils::HasEmojis(const UTclStreamEvent_Anywhere_ChatMessage* Message)
{
	if (!IsValid(Message)) return false;
	return !Message->Emojis.IsEmpty();
}

bool
UTclChatMessageUtils::HasEmotes(const UTclStreamEvent_Anywhere_ChatMessage* Message)
{
	if (!IsValid(Message)) return false;
	return !Message->Emotes.IsEmpty();
}

bool
UTclChatMessageUtils::HasBadge_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message, const FString& SetId, bool bCaseSensitive)
{
	return IsValid(Message) && TclBadges::AnyTwitch(Message->Badges, { SetId }, bCaseSensitive);
}

bool
UTclChatMessageUtils::IsBroadcaster_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message)
{
	return IsValid(Message) && TclBadges::AnyTwitch(Message->Badges, TclBadges::TwitchBroadcaster());
}

bool
UTclChatMessageUtils::IsModerator_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message)
{
	return IsValid(Message) && TclBadges::AnyTwitch(Message->Badges, TclBadges::TwitchModerator());
}

bool
UTclChatMessageUtils::IsVip_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message)
{
	return IsValid(Message) && TclBadges::AnyTwitch(Message->Badges, TclBadges::TwitchVip());
}

bool
UTclChatMessageUtils::IsSubscriber_Twitch(const UTclStreamEvent_Twitch_ChatMessage* Message)
{
	return IsValid(Message) && TclBadges::AnyTwitch(Message->Badges, TclBadges::TwitchSubscriber());
}

TArray<uint32>
UTclChatMessageUtils::CodePointsOf(const FString& S)
{
	TArray<uint32> Out;
	for (int32 i = 0; i < S.Len(); )
	{
		uint32 cp = (uint32)(uint16)S[i];
		if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < S.Len())
		{
			uint32 low = (uint32)(uint16)S[i + 1];
			if (low >= 0xDC00 && low <= 0xDFFF) { cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00); i += 2; }
			else { ++i; }
		}
		else { ++i; }
		Out.Add(cp);
	}
	return Out;
}

TArray<FString>
UTclChatMessageUtils::GetEmojiCodePoints(const FString& Emoji)
{
	TArray<FString> Out;
	for (uint32 cp : CodePointsOf(Emoji))
		Out.Add(FString::Printf(TEXT("U+%04X"), cp));
	return Out;
}

TArray<FString>
UTclChatMessageUtils::GetEmojiHexValues(const FString& Emoji)
{
	TArray<FString> Out;
	for (uint32 cp : CodePointsOf(Emoji))
		Out.Add(FString::Printf(TEXT("%04x"), cp));
	return Out;
}

FString
UTclChatMessageUtils::GetEmojiImageFileName(const UTclChatMessageEmoji* Emoji, ETclEmojiImageSet ImageSet)
{
	using namespace TclEmojiImageFileNames;

	if (!IsValid(Emoji)) return FString();

	TArray<uint32> Values;
	if (!ParseCodePoints(Emoji->CodePoints, Values)) return FString();

	switch (ImageSet)
	{
	case ETclEmojiImageSet::Noto:
		return NotoFileName(Values);
	case ETclEmojiImageSet::Twemoji:
		return TwemojiFileName(Values);
	case ETclEmojiImageSet::OpenMoji:
		return OpenMojiFileName(Values);
	}
	return FString();
}

FString
UTclChatMessageUtils::GetEmojiImageUrl(const UTclChatMessageEmoji* Emoji, ETclEmojiImageSet ImageSet, int32 Size)
{
	using namespace TclEmojiImageFileNames;

	if (!IsValid(Emoji)) return FString();

	TArray<uint32> Values;
	if (!ParseCodePoints(Emoji->CodePoints, Values)) return FString();

	switch (ImageSet)
	{
	case ETclEmojiImageSet::Noto:
		return FString::Printf(TEXT("%s%s/%d.png"), NotoFolder, *NotoSequenceName(Values), ImageSizeFor(NotoSizes, Size));
	case ETclEmojiImageSet::Twemoji:
	{
		const int32 Pixels = ImageSizeFor(TwemojiSizes, Size);
		return FString::Printf(TEXT("%s%dx%d/%s"), TwemojiFolder, Pixels, Pixels, *TwemojiFileName(Values));
	}
	case ETclEmojiImageSet::OpenMoji:
	{
		const int32 Pixels = ImageSizeFor(OpenMojiSizes, Size);
		return FString::Printf(TEXT("%s%dx%d/%s"), OpenMojiFolder, Pixels, Pixels, *OpenMojiFileName(Values));
	}
	}
	return FString();
}
