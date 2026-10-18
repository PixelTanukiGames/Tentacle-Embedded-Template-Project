// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/Tentacle_Stream.h"
#include "Enums/TclChatSendTarget.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/**
	 * Parse a built payload, returning its "data" object and writing the
	 * message discriminant to OutType. Parsing rather than string-comparing is
	 * deliberate: FJsonObject does not guarantee field order, so an exact-string
	 * assertion would be flaky.
	 */
	TSharedPtr<FJsonObject>
	ParsePayloadData(const FString& Payload, FString& OutType)
	{
		TSharedPtr<FJsonObject> Root;
		const auto Reader = TJsonReaderFactory<>::Create(Payload);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			return nullptr;
		}

		OutType = Root->GetStringField(TEXT("type"));
		return Root->GetObjectField(TEXT("data"));
	}
}

/**
 * Pins the Stream_SendChatMessage wire contract the app's zod schema validates
 * on the other end of the socket. The optional fields are the interesting part:
 * absent means "every connected platform" / "mentions nobody", so emitting them
 * as empty strings would change the meaning of the message.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclChatSendPayloadTest,
	"Tentacle.Stream.BuildsChatMessagePayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclChatSendPayloadTest::RunTest(const FString& Parameters)
{
	// Untargeted send to every connected platform: both optional fields absent.
	{
		FString Type;
		const TSharedPtr<FJsonObject> Data = ParsePayloadData(
			UTentacle_Stream::BuildChatMessagePayload(TEXT("Hello chat!"), ETclChatSendTarget::AllConnected, false, FString()),
			Type);
		if (!TestNotNull(TEXT("AllConnected: data object"), Data.Get())) return false;

		TestEqual(TEXT("AllConnected: type"), Type, FString(TEXT("Stream_SendChatMessage")));
		TestEqual(TEXT("AllConnected: message"), Data->GetStringField(TEXT("message")), FString(TEXT("Hello chat!")));
		TestFalse(TEXT("AllConnected: isBot"), Data->GetBoolField(TEXT("isBot")));
		TestFalse(TEXT("AllConnected: platform omitted"), Data->HasField(TEXT("platform")));
		TestFalse(TEXT("AllConnected: viewerId omitted"), Data->HasField(TEXT("viewerId")));
	}

	// Targeting Twitch emits the platform's lower-case wire value.
	{
		FString Type;
		const TSharedPtr<FJsonObject> Data = ParsePayloadData(
			UTentacle_Stream::BuildChatMessagePayload(TEXT("Round starting!"), ETclChatSendTarget::Twitch, false, FString()),
			Type);
		if (!TestNotNull(TEXT("Twitch: data object"), Data.Get())) return false;

		TestEqual(TEXT("Twitch: platform"), Data->GetStringField(TEXT("platform")), FString(TEXT("twitch")));
	}

	// Kick, sent as the bot identity.
	{
		FString Type;
		const TSharedPtr<FJsonObject> Data = ParsePayloadData(
			UTentacle_Stream::BuildChatMessagePayload(TEXT("Type !join to enter"), ETclChatSendTarget::Kick, true, FString()),
			Type);
		if (!TestNotNull(TEXT("Kick: data object"), Data.Get())) return false;

		TestEqual(TEXT("Kick: platform"), Data->GetStringField(TEXT("platform")), FString(TEXT("kick")));
		TestTrue(TEXT("Kick: isBot"), Data->GetBoolField(TEXT("isBot")));
	}

	// A viewer-targeted send carries the viewer id; the app turns it into the
	// "@name message" mention using the viewer's per-platform name.
	{
		FString Type;
		const TSharedPtr<FJsonObject> Data = ParsePayloadData(
			UTentacle_Stream::BuildChatMessagePayload(TEXT("thanks for the follow!"), ETclChatSendTarget::AllConnected, false, TEXT("viewer_123")),
			Type);
		if (!TestNotNull(TEXT("Viewer: data object"), Data.Get())) return false;

		TestEqual(TEXT("Viewer: viewerId"), Data->GetStringField(TEXT("viewerId")), FString(TEXT("viewer_123")));
		TestFalse(TEXT("Viewer: platform still omitted"), Data->HasField(TEXT("platform")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
