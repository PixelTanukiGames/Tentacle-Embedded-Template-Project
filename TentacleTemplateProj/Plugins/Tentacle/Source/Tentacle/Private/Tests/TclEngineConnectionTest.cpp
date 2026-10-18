// © 2025 Eldritch One. All rights reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Utilities/TclEngineIdentity.h"
#include "Utilities/TclReconnectPolicy.h"
#include "Utilities/TclSidecarEndpoints.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

/**
 * Pins the discovery-file contract the sidecar writes on the other side: the
 * file is the primary source, every field falls back on its own, and anything
 * unreadable degrades to the compiled-in defaults rather than to a failure.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclSidecarEndpointsTest,
	"Tentacle.Engine.ParsesDiscoveryFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclSidecarEndpointsTest::RunTest(const FString& Parameters)
{
	// The file as the sidecar writes it after a normal boot.
	{
		const FTclSidecarEndpoints Endpoints = FTclSidecarEndpoints::ParseDiscoveryJson(
			TEXT("{\"port\":47821,\"baseUrl\":\"http://127.0.0.1:47821\",\"enginePort\":48123,\"engineUrl\":\"ws://127.0.0.1:48123\"}"));
		TestEqual(TEXT("full file: http base url"), Endpoints.HttpBaseUrl, FString(TEXT("http://127.0.0.1:47821")));
		TestEqual(TEXT("full file: engine socket url"), Endpoints.EngineSocketUrl, FString(TEXT("ws://127.0.0.1:48123")));
		TestTrue(TEXT("full file: from discovery file"), Endpoints.bFromDiscoveryFile);
	}

	// Ports alone are enough: the urls are derived on loopback.
	{
		const FTclSidecarEndpoints Endpoints = FTclSidecarEndpoints::ParseDiscoveryJson(TEXT("{\"port\":50001,\"enginePort\":50002}"));
		TestEqual(TEXT("ports only: http base url"), Endpoints.HttpBaseUrl, FString(TEXT("http://127.0.0.1:50001")));
		TestEqual(TEXT("ports only: engine socket url"), Endpoints.EngineSocketUrl, FString(TEXT("ws://127.0.0.1:50002")));
	}

	// The sidecar omits the engine keys when its listener failed to start; the
	// HTTP half is still taken from the file and the socket keeps its default.
	{
		const FTclSidecarEndpoints Endpoints = FTclSidecarEndpoints::ParseDiscoveryJson(TEXT("{\"port\":47821,\"baseUrl\":\"http://127.0.0.1:47821\"}"));
		TestEqual(TEXT("no engine keys: http base url"), Endpoints.HttpBaseUrl, FString(TEXT("http://127.0.0.1:47821")));
		TestEqual(TEXT("no engine keys: engine default kept"), Endpoints.EngineSocketUrl, FString(TEXT("ws://127.0.0.1:47822")));
		TestTrue(TEXT("no engine keys: from discovery file"), Endpoints.bFromDiscoveryFile);
	}

	// Garbage (a half-written file mid-rewrite) yields the defaults, never a failure.
	{
		const FTclSidecarEndpoints Endpoints = FTclSidecarEndpoints::ParseDiscoveryJson(TEXT("{\"port\":478"));
		TestEqual(TEXT("malformed: http default"), Endpoints.HttpBaseUrl, FString(TEXT("http://127.0.0.1:47821")));
		TestEqual(TEXT("malformed: engine default"), Endpoints.EngineSocketUrl, FString(TEXT("ws://127.0.0.1:47822")));
		TestFalse(TEXT("malformed: not from discovery file"), Endpoints.bFromDiscoveryFile);
	}

	// The defaults match the constants the sidecar hardcodes.
	{
		const FTclSidecarEndpoints Defaults;
		TestEqual(TEXT("defaults: http port"), Defaults.HttpBaseUrl, FString::Printf(TEXT("http://127.0.0.1:%d"), FTclSidecarEndpoints::DefaultHttpPort));
		TestEqual(TEXT("defaults: engine port"), Defaults.EngineSocketUrl, FString::Printf(TEXT("ws://127.0.0.1:%d"), FTclSidecarEndpoints::DefaultEnginePort));
	}

	return true;
}

/**
 * Pins the Session_Hello wire contract the sidecar validates before it admits
 * a session: the frame shape, the field set, the 128-character clamp that
 * turns an over-long label into a truncated one instead of a refused socket,
 * and the connection password, sent only when set and never altered.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclEngineIdentityTest,
	"Tentacle.Engine.BuildsHelloFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclEngineIdentityTest::RunTest(const FString& Parameters)
{
	FTclEngineIdentity Identity;
	Identity.EngineVersion = TEXT("5.7.0");
	Identity.PluginVersion = TEXT("1.1");
	Identity.InstanceId = TEXT("abc123");
	Identity.Label = TEXT("  My Game (Editor)  ");

	FString Problem;
	TestTrue(TEXT("valid identity"), Identity.IsValid(Problem));

	// Parsing rather than string-comparing: FJsonObject does not guarantee
	// field order, so an exact-string assertion would be flaky.
	TSharedPtr<FJsonObject> Root;
	const auto Reader = TJsonReaderFactory<>::Create(Identity.ToHelloMessage());
	if (!TestTrue(TEXT("hello parses"), FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())) return false;

	TestEqual(TEXT("type"), Root->GetStringField(TEXT("type")), FString(TEXT("Session_Hello")));
	const TSharedPtr<FJsonObject> Data = Root->GetObjectField(TEXT("data"));
	if (!TestNotNull(TEXT("data object"), Data.Get())) return false;

	TestEqual(TEXT("engine"), Data->GetStringField(TEXT("engine")), FString(TEXT("unreal")));
	TestEqual(TEXT("engineVersion"), Data->GetStringField(TEXT("engineVersion")), FString(TEXT("5.7.0")));
	TestEqual(TEXT("pluginVersion"), Data->GetStringField(TEXT("pluginVersion")), FString(TEXT("1.1")));
	TestEqual(TEXT("instanceId"), Data->GetStringField(TEXT("instanceId")), FString(TEXT("abc123")));
	TestEqual(TEXT("label is trimmed"), Data->GetStringField(TEXT("label")), FString(TEXT("My Game (Editor)")));

	// No password unless Tentacle's user set one: the field is left out.
	TestFalse(TEXT("no password field without a password"), Data->HasField(TEXT("password")));

	// A password goes out exactly as entered, never trimmed or truncated.
	{
		FTclEngineIdentity WithPassword = Identity;
		WithPassword.Password = TEXT(" hunter2 ") + FString::ChrN(FTclEngineIdentity::MaxFieldLength, TEXT('x'));
		TestEqual(TEXT("password sent as entered"),
			WithPassword.ToHelloPayload()->GetStringField(TEXT("password")), WithPassword.Password);
	}

	// An over-long label is clamped to what the server accepts.
	{
		FTclEngineIdentity Long = Identity;
		Long.Label = FString::ChrN(FTclEngineIdentity::MaxFieldLength + 40, TEXT('x'));
		TestEqual(TEXT("label clamped"), Long.ToHelloPayload()->GetStringField(TEXT("label")).Len(), FTclEngineIdentity::MaxFieldLength);
	}

	// The server requires an instance id; the plugin must refuse to dial
	// without one rather than loop on a refused handshake.
	{
		FTclEngineIdentity Anonymous = Identity;
		Anonymous.InstanceId = TEXT("   ");
		TestFalse(TEXT("blank instance id is invalid"), Anonymous.IsValid(Problem));
		TestTrue(TEXT("problem names the instance id"), Problem.Contains(TEXT("instance id")));
	}

	return true;
}

/**
 * The reconnect backoff: quick first retries, growth to a ceiling, and jitter
 * that never pushes a delay negative or past the ceiling.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTclReconnectPolicyTest,
	"Tentacle.Engine.ReconnectBackoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTclReconnectPolicyTest::RunTest(const FString& Parameters)
{
	FTclReconnectPolicy Policy;
	Policy.InitialDelaySeconds = 1.0;
	Policy.MaxDelaySeconds = 30.0;
	Policy.Multiplier = 2.0;
	Policy.JitterFraction = 0.25;

	// With the jitter sample centred, the delays are the bare exponential.
	TestEqual(TEXT("attempt 1"), Policy.DelayForAttempt(1, 0.5), 1.0);
	TestEqual(TEXT("attempt 2"), Policy.DelayForAttempt(2, 0.5), 2.0);
	TestEqual(TEXT("attempt 4"), Policy.DelayForAttempt(4, 0.5), 8.0);
	TestEqual(TEXT("attempt 6 hits the ceiling"), Policy.DelayForAttempt(6, 0.5), 30.0);
	TestEqual(TEXT("attempt 1000 stays at the ceiling"), Policy.DelayForAttempt(1000, 0.5), 30.0);

	// Jitter moves the delay a quarter either way, and never past the ceiling.
	TestEqual(TEXT("attempt 2, low jitter"), Policy.DelayForAttempt(2, 0.0), 1.5);
	TestEqual(TEXT("attempt 2, high jitter"), Policy.DelayForAttempt(2, 1.0), 2.5);
	TestEqual(TEXT("ceiling, high jitter"), Policy.DelayForAttempt(10, 1.0), 30.0);
	TestTrue(TEXT("never negative"), Policy.DelayForAttempt(1, 0.0) >= 0.0);

	// Degenerate settings cannot make the loop spin or the delay shrink.
	FTclReconnectPolicy Broken;
	Broken.InitialDelaySeconds = 5.0;
	Broken.MaxDelaySeconds = 1.0;
	Broken.Multiplier = 0.1;
	TestEqual(TEXT("ceiling below initial: initial wins"), Broken.DelayForAttempt(1, 0.5), 5.0);
	TestEqual(TEXT("multiplier below one: no shrink"), Broken.DelayForAttempt(5, 0.5), 5.0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
