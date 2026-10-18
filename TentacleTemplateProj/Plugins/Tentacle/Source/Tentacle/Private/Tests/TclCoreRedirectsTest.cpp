// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UObject/CoreRedirects.h"
#include "UObject/UnrealType.h"
#include "Sdk/TclChatMessageEmoji.h"
#include "Sdk/TclEmojiSkinTone.h"
#include "Utilities/TclTentacleMode.h"

// The plugin's CoreRedirects (Config/DefaultTentacle.ini) keep a Blueprint that
// referenced a renamed type or member loading under the new name. These tests
// resolve each old name the way the engine does when it loads such a Blueprint,
// so a redirect that is missing, misspelled, or pointing at a name that no longer
// exists fails here instead of in a user's project.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCoreRedirectsEmojiSkinToneTest,
	"Tentacle.CoreRedirects.EmojiSkinTone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCoreRedirectsEmojiSkinToneTest::RunTest(const FString& Parameters)
{
	// 1.1.0-alpha.4 called the enum ETclEmojiVariant. A Blueprint pin holds the
	// enum as an import of /Script/Tentacle.ETclEmojiVariant, which the linker
	// redirects by type and path.
	const FCoreRedirectObjectName OldEnum(TEXT("ETclEmojiVariant"), NAME_None, TEXT("/Script/Tentacle"));
	const FCoreRedirectObjectName NewEnum = FCoreRedirects::GetRedirectedName(ECoreRedirectFlags::Type_Enum, OldEnum);
	TestEqual(
		TEXT("ETclEmojiVariant redirects to the enum Tentacle declares now"),
		NewEnum.ToString(),
		StaticEnum<ETclEmojiSkinTone>()->GetPathName());

	// It also called the chat emoji's skin tones Variant. A Get node resolves a
	// member it cannot find through the property redirects.
	UClass* EmojiClass = UTclChatMessageEmoji::StaticClass();
	const FName Redirected = FProperty::FindRedirectedPropertyName(EmojiClass, TEXT("Variant"));
	TestEqual(TEXT("Variant redirects to SkinTones"), Redirected, FName(TEXT("SkinTones")));

	// The pin keeps its connections only if the new member has the type the old
	// one had: an array of the (redirected) enum.
	const FArrayProperty* SkinTones = FindFProperty<FArrayProperty>(EmojiClass, TEXT("SkinTones"));
	if (!TestNotNull(TEXT("SkinTones is an array on UTclChatMessageEmoji"), SkinTones))
	{
		return false;
	}
	const FEnumProperty* Inner = CastField<FEnumProperty>(SkinTones->Inner);
	TestTrue(
		TEXT("SkinTones holds ETclEmojiSkinTone, the type the redirected Variant pins now carry"),
		Inner != nullptr && Inner->GetEnum() == StaticEnum<ETclEmojiSkinTone>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclCoreRedirectsTentacleModeTest,
	"Tentacle.CoreRedirects.TentacleModePaired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclCoreRedirectsTentacleModeTest::RunTest(const FString& Parameters)
{
	// Until 1.2.0-alpha.3 Paired mode was called App. An actor's saved Editor
	// Mode or Build Mode, or a Blueprint pin's default, holds the value by name,
	// which the engine looks up through the enum's value redirects.
	const UEnum* Mode = StaticEnum<ETclTentacleMode>();
	TestEqual(TEXT("App resolves to Paired"), Mode->GetValueByNameString(TEXT("App")), static_cast<int64>(ETclTentacleMode::Paired));
	TestEqual(TEXT("so does its full name"), Mode->GetValueByNameString(TEXT("ETclTentacleMode::App")), static_cast<int64>(ETclTentacleMode::Paired));
	TestEqual(TEXT("Embedded is unchanged"), Mode->GetValueByNameString(TEXT("Embedded")), static_cast<int64>(ETclTentacleMode::Embedded));
	return true;
}

#endif
