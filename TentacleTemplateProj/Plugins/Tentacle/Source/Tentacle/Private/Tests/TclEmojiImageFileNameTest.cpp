// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// NewObject and GetTransientPackage: the editor build gets them through its
// shared PCH, but UAT BuildPlugin compiles against an installed engine where
// 5.4 and newer enforce include-what-you-use.
#include "UObject/Package.h"
#include "Sdk/TclChatMessageEmoji.h"
#include "Utilities/TclChatMessageUtils.h"

namespace
{
	/** One emoji and the file name each image set gives it. */
	struct FEmojiImageCase
	{
		const TCHAR* Label;
		const TCHAR* CodePoints;
		const TCHAR* Noto;
		const TCHAR* Twemoji;
		const TCHAR* OpenMoji;
	};

	// The same table as tentacle-web's tests/emoji-unit.test.ts and the Unity and
	// Godot suites. Every name was checked against the files each set ships.
	const FEmojiImageCase EmojiImageCases[] = {
		{ TEXT("grinning face"), TEXT("1F600"), TEXT("emoji_u1f600.png"), TEXT("1f600.png"), TEXT("1F600.png") },
		{ TEXT("red heart"), TEXT("2764 FE0F"), TEXT("emoji_u2764.png"), TEXT("2764.png"), TEXT("2764.png") },
		{ TEXT("copyright"), TEXT("00A9 FE0F"), TEXT("emoji_u00a9.png"), TEXT("a9.png"), TEXT("00A9.png") },
		{ TEXT("keycap 1"), TEXT("0031 FE0F 20E3"), TEXT("emoji_u0031_20e3.png"), TEXT("31-20e3.png"), TEXT("0031-FE0F-20E3.png") },
		{ TEXT("thumbs up: light skin tone"), TEXT("1F44D 1F3FB"), TEXT("emoji_u1f44d_1f3fb.png"), TEXT("1f44d-1f3fb.png"), TEXT("1F44D-1F3FB.png") },
		{ TEXT("heart on fire"), TEXT("2764 FE0F 200D 1F525"), TEXT("emoji_u2764_200d_1f525.png"), TEXT("2764-fe0f-200d-1f525.png"), TEXT("2764-FE0F-200D-1F525.png") },
		{ TEXT("face in clouds"), TEXT("1F636 200D 1F32B FE0F"), TEXT("emoji_u1f636_200d_1f32b.png"), TEXT("1f636-200d-1f32b-fe0f.png"), TEXT("1F636-200D-1F32B-FE0F.png") },
		{ TEXT("eye in speech bubble"), TEXT("1F441 FE0F 200D 1F5E8 FE0F"), TEXT("emoji_u1f441_200d_1f5e8.png"), TEXT("1f441-200d-1f5e8.png"), TEXT("1F441-FE0F-200D-1F5E8-FE0F.png") },
		{
			TEXT("woman walking facing right: medium skin tone"),
			TEXT("1F6B6 1F3FD 200D 2640 FE0F 200D 27A1 FE0F"),
			TEXT("emoji_u1f6b6_1f3fd_200d_2640_200d_27a1.png"),
			TEXT("1f6b6-1f3fd-200d-2640-fe0f-200d-27a1-fe0f.png"),
			TEXT("1F6B6-1F3FD-200D-2640-FE0F-200D-27A1-FE0F.png"),
		},
		{ TEXT("flag: United States"), TEXT("1F1FA 1F1F8"), TEXT("US.png"), TEXT("1f1fa-1f1f8.png"), TEXT("1F1FA-1F1F8.png") },
		{
			TEXT("flag: England"),
			TEXT("1F3F4 E0067 E0062 E0065 E006E E0067 E007F"),
			TEXT("GB-ENG.png"),
			TEXT("1f3f4-e0067-e0062-e0065-e006e-e0067-e007f.png"),
			TEXT("1F3F4-E0067-E0062-E0065-E006E-E0067-E007F.png"),
		},
	};

	/** An emoji entry carrying these space-separated code points, as a chat message delivers one. */
	UTclChatMessageEmoji*
	EmojiWithCodePoints(const FString& CodePoints)
	{
		UTclChatMessageEmoji* Emoji = NewObject<UTclChatMessageEmoji>(GetTransientPackage());
		CodePoints.ParseIntoArray(Emoji->CodePoints, TEXT(" "));
		return Emoji;
	}
}

/**
 * The file name each image set gives an emoji. The sets disagree on case, the
 * separator, zero padding, the variation selector, and flags, so a game that
 * joined the code points itself would miss files in every one of them.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclEmojiImageFileNameTest,
	"Tentacle.ChatMessage.NamesEmojiImageFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclEmojiImageFileNameTest::RunTest(const FString& Parameters)
{
	for (const FEmojiImageCase& Case : EmojiImageCases)
	{
		const UTclChatMessageEmoji* Emoji = EmojiWithCodePoints(Case.CodePoints);
		TestEqual(FString::Printf(TEXT("%s in Noto"), Case.Label),
			UTclChatMessageUtils::GetEmojiImageFileName(Emoji, ETclEmojiImageSet::Noto), FString(Case.Noto));
		TestEqual(FString::Printf(TEXT("%s in Twemoji"), Case.Label),
			UTclChatMessageUtils::GetEmojiImageFileName(Emoji, ETclEmojiImageSet::Twemoji), FString(Case.Twemoji));
		TestEqual(FString::Printf(TEXT("%s in OpenMoji"), Case.Label),
			UTclChatMessageUtils::GetEmojiImageFileName(Emoji, ETclEmojiImageSet::OpenMoji), FString(Case.OpenMoji));
	}

	// Nothing to name: no emoji, no code points, or code points not written in hex.
	TestEqual(TEXT("no emoji"), UTclChatMessageUtils::GetEmojiImageFileName(nullptr, ETclEmojiImageSet::Noto), FString());
	TestEqual(TEXT("no code points"), UTclChatMessageUtils::GetEmojiImageFileName(EmojiWithCodePoints(TEXT("")), ETclEmojiImageSet::Noto), FString());
	TestEqual(TEXT("not hex"), UTclChatMessageUtils::GetEmojiImageFileName(EmojiWithCodePoints(TEXT("1F600 U+FE0F")), ETclEmojiImageSet::Twemoji), FString());

	// The code points are read in either case.
	TestEqual(TEXT("lowercase input"), UTclChatMessageUtils::GetEmojiImageFileName(EmojiWithCodePoints(TEXT("2764 fe0f")), ETclEmojiImageSet::OpenMoji), FString(TEXT("2764.png")));

	return true;
}

/**
 * The address of each set's image: the file name in the set's own folder,
 * pinned to a release. The same expectations as tentacle-web's tests, whose
 * tests/emoji-urls.test.ts also requests these folders for real and holds this
 * plugin's addresses to its own.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclEmojiImageUrlTest,
	"Tentacle.ChatMessage.AddressesEmojiImages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclEmojiImageUrlTest::RunTest(const FString& Parameters)
{
	const UTclChatMessageEmoji* RedHeart = EmojiWithCodePoints(TEXT("2764 FE0F"));
	TestEqual(TEXT("Noto"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Noto),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/2764/512.png")));
	TestEqual(TEXT("Twemoji"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Twemoji),
		FString(TEXT("https://cdn.jsdelivr.net/gh/jdecked/twemoji@v17.0.3/assets/72x72/2764.png")));
	TestEqual(TEXT("OpenMoji"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::OpenMoji),
		FString(TEXT("https://cdn.jsdelivr.net/gh/hfg-gmuend/openmoji@17.0.0/color/618x618/2764.png")));

	// Noto names an image by its code points, joined emoji, keycaps, and flags alike.
	TestEqual(TEXT("a Noto joined emoji"),
		UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("2764 FE0F 200D 1F525")), ETclEmojiImageSet::Noto),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/2764_200d_1f525/512.png")));
	TestEqual(TEXT("a Noto keycap"),
		UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("0031 FE0F 20E3")), ETclEmojiImageSet::Noto),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/0031_20e3/512.png")));
	TestEqual(TEXT("a Noto country flag"),
		UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("1F1FA 1F1F8")), ETclEmojiImageSet::Noto),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/1f1fa_1f1f8/512.png")));
	TestEqual(TEXT("a Noto subdivision flag"),
		UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("1F3F4 E0067 E0062 E0065 E006E E0067 E007F")), ETclEmojiImageSet::Noto),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/1f3f4_e0067_e0062_e0065_e006e_e0067_e007f/512.png")));
	TestEqual(TEXT("a Twemoji flag"),
		UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("1F1FA 1F1F8")), ETclEmojiImageSet::Twemoji),
		FString(TEXT("https://cdn.jsdelivr.net/gh/jdecked/twemoji@v17.0.3/assets/72x72/1f1fa-1f1f8.png")));

	TestEqual(TEXT("nothing to name"), UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("")), ETclEmojiImageSet::Noto), FString());
	TestEqual(TEXT("no emoji"), UTclChatMessageUtils::GetEmojiImageUrl(nullptr, ETclEmojiImageSet::Twemoji), FString());

	// A size picks the smallest one the set ships that is at least as big, or its
	// largest when none is.
	TestEqual(TEXT("Noto at 72"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Noto, 72),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/2764/72.png")));
	TestEqual(TEXT("Noto at 100"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Noto, 100),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/2764/128.png")));
	TestEqual(TEXT("Noto at 1"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Noto, 1),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/2764/32.png")));
	TestEqual(TEXT("OpenMoji at 72"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::OpenMoji, 72),
		FString(TEXT("https://cdn.jsdelivr.net/gh/hfg-gmuend/openmoji@17.0.0/color/72x72/2764.png")));
	TestEqual(TEXT("OpenMoji at 100"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::OpenMoji, 100),
		FString(TEXT("https://cdn.jsdelivr.net/gh/hfg-gmuend/openmoji@17.0.0/color/618x618/2764.png")));
	TestEqual(TEXT("Twemoji bigger than it has"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Twemoji, 4096),
		FString(TEXT("https://cdn.jsdelivr.net/gh/jdecked/twemoji@v17.0.3/assets/72x72/2764.png")));
	TestEqual(TEXT("Noto bigger than it has"), UTclChatMessageUtils::GetEmojiImageUrl(RedHeart, ETclEmojiImageSet::Noto, 4096),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/2764/512.png")));
	TestEqual(TEXT("a Noto flag at a smaller size"),
		UTclChatMessageUtils::GetEmojiImageUrl(EmojiWithCodePoints(TEXT("1F1FA 1F1F8")), ETclEmojiImageSet::Noto, 32),
		FString(TEXT("https://fonts.gstatic.com/s/e/notoemoji/18.0/1f1fa_1f1f8/32.png")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
