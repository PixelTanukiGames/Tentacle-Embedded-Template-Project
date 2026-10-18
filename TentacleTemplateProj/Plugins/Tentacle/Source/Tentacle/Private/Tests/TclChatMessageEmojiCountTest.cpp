// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// NewObject and GetTransientPackage: the editor build gets them through its
// shared PCH, but UAT BuildPlugin compiles against an installed engine where
// 5.4 and newer enforce include-what-you-use, so the package build fails
// without this where the editor build passes.
#include "UObject/Package.h"
#include "Sdk/TclChatMessageEmoji.h"
#include "Sdk/TclStreamEvent_Anywhere_ChatMessage.h"
#include "Utilities/TclChatMessageUtils.h"

namespace
{
	/** Add one emoji entry to a message, under the key the app sends it by. */
	void
	AddEmoji(
		UTclStreamEvent_Anywhere_ChatMessage* Message,
		const FString& Key,
		const FString& Code,
		const FString& Name,
		const TArray<FString>& Aliases,
		int32 Count)
	{
		UTclChatMessageEmoji* Emoji = NewObject<UTclChatMessageEmoji>(GetTransientPackage());
		Emoji->Code = Code;
		Emoji->Name = Name;
		Emoji->Aliases = Aliases;
		Emoji->Count = Count;
		Message->Emojis.Add(Key, Emoji);
	}

	/**
	 * A message using thumbs-up at three tones, keyed the way the app keys skin-tone
	 * variants: one entry per tone, each carrying the tone-free name and the aliases
	 * they share.
	 */
	UTclStreamEvent_Anywhere_ChatMessage*
	MixedTonesMessage()
	{
		const TArray<FString> ThumbsUpAliases = { TEXT("+1"), TEXT("thumbsup") };
		UTclStreamEvent_Anywhere_ChatMessage* Message = NewObject<UTclStreamEvent_Anywhere_ChatMessage>(GetTransientPackage());
		AddEmoji(Message, TEXT("thumbs_up"), TEXT("\U0001F44D"), TEXT("thumbs_up"), ThumbsUpAliases, 1);
		AddEmoji(Message, TEXT("thumbs_up-light"), TEXT("\U0001F44D\U0001F3FB"), TEXT("thumbs_up"), ThumbsUpAliases, 2);
		AddEmoji(Message, TEXT("thumbs_up-dark"), TEXT("\U0001F44D\U0001F3FF"), TEXT("thumbs_up"), ThumbsUpAliases, 1);
		AddEmoji(Message, TEXT("fire"), TEXT("\U0001F525"), TEXT("fire"), {}, 3);
		return Message;
	}
}

/**
 * Counting an emoji that was sent at several skin tones. Each tone is its own
 * entry, so a game asking for the plain name has to get every tone of it -
 * otherwise a graph reacting to thumbs-up stops firing for anyone who uses a
 * tone, which is exactly what it did while all the tones shared one entry.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclChatMessageEmojiCountTest,
	"Tentacle.ChatMessage.CountsSkinToneVariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclChatMessageEmojiCountTest::RunTest(const FString& Parameters)
{
	UTclStreamEvent_Anywhere_ChatMessage* Message = MixedTonesMessage();

	// The plain name covers every tone: 1 untoned + 2 light + 1 dark.
	TestEqual(TEXT("plain name counts every tone"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("thumbs_up")), 4);

	// An alias is another name for the same emoji, so it counts the same.
	TestEqual(TEXT("alias counts every tone"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("+1")), 4);
	TestEqual(TEXT("second alias counts every tone"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("thumbsup")), 4);

	// A tone-qualified key counts just that tone.
	TestEqual(TEXT("tone key counts one tone"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("thumbs_up-light")), 2);
	TestEqual(TEXT("dark tone key counts one tone"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("thumbs_up-dark")), 1);

	// An untoned emoji is unaffected, and a name the message does not carry is zero.
	TestEqual(TEXT("untoned emoji"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("fire")), 3);
	TestEqual(TEXT("missing emoji"), UTclChatMessageUtils::GetEmojiCount(Message, TEXT("nothing")), 0);

	// Substring counting spans the tones, since every key starts with the name.
	TestEqual(TEXT("substring spans tones"), UTclChatMessageUtils::GetEmojiCountContaining(Message, TEXT("thumbs_up")), 4);
	TestEqual(TEXT("substring picks one tone"), UTclChatMessageUtils::GetEmojiCountContaining(Message, TEXT("-dark")), 1);

	return true;
}

/**
 * Parsing one emoji from the wire, as Tentacle writes it.
 *
 * The generated parser is what a game actually receives its emoji through, and
 * the tone list is the part with no precedent in the plugin: nothing else on
 * the surface is an array of a generated enum. A hand-built object proves the
 * helpers; only parsing proves the wire.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclChatMessageEmojiParseTest,
	"Tentacle.ChatMessage.ParsesTheEmojiTentacleSends",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclChatMessageEmojiParseTest::RunTest(const FString& Parameters)
{
	const FString Json = TEXT(R"({
		"code": "👍🏻",
		"codePoints": ["1F44D", "1F3FB"],
		"name": "thumbs_up",
		"skinTones": ["light"],
		"aliases": ["+1", "thumbsup"],
		"components": [],
		"count": 2,
		"positions": [{ "start": 0, "end": 3 }, { "start": 10, "end": 13 }]
	})");

	UTclChatMessageEmoji* Emoji = UTclChatMessageEmoji::ParseFromString(GetTransientPackage(), Json);
	if (!TestNotNull(TEXT("the emoji parses"), Emoji)) return false;

	TestEqual(TEXT("code"), Emoji->Code, FString(TEXT("\U0001F44D\U0001F3FB")));
	if (TestEqual(TEXT("two code points"), Emoji->CodePoints.Num(), 2))
	{
		TestEqual(TEXT("the tone second"), Emoji->CodePoints[1], FString(TEXT("1F3FB")));
	}
	TestEqual(TEXT("name"), Emoji->Name, FString(TEXT("thumbs_up")));
	TestEqual(TEXT("count"), Emoji->Count, 2);

	// The tone arrives as the generated enum, not as the string on the wire.
	if (TestEqual(TEXT("one tone"), Emoji->SkinTones.Num(), 1))
	{
		TestEqual(TEXT("parsed as the enum value"), Emoji->SkinTones[0], ETclEmojiSkinTone::Light);
	}
	if (TestEqual(TEXT("two aliases"), Emoji->Aliases.Num(), 2))
	{
		TestEqual(TEXT("the first alias"), Emoji->Aliases[0], FString(TEXT("+1")));
	}
	TestEqual(TEXT("no components"), Emoji->Components.Num(), 0);
	TestEqual(TEXT("both positions"), Emoji->Positions.Num(), 2);

	// A sequence built from several emoji names its parts and carries no tone.
	UTclChatMessageEmoji* Couple = UTclChatMessageEmoji::ParseFromString(GetTransientPackage(), TEXT(R"({
		"code": "👩‍🤝‍👨",
		"codePoints": ["1F469", "200D", "1F91D", "200D", "1F468"],
		"name": "woman_and_man_holding_hands",
		"skinTones": [],
		"aliases": [],
		"components": ["woman", "handshake", "man"],
		"count": 1,
		"positions": [{ "start": 0, "end": 7 }]
	})"));
	if (!TestNotNull(TEXT("the joined emoji parses"), Couple)) return false;

	TestEqual(TEXT("no tone"), Couple->SkinTones.Num(), 0);
	if (TestEqual(TEXT("three components"), Couple->Components.Num(), 3))
	{
		TestEqual(TEXT("named in order"), Couple->Components[1], FString(TEXT("handshake")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
