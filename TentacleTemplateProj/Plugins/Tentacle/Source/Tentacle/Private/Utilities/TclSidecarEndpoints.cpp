// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclSidecarEndpoints.h"
#include "Utilities/TclCommon.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

FString
FTclSidecarEndpoints::DefaultDataDirectory()
{
#if PLATFORM_WINDOWS
	const FString AppData = FPlatformMisc::GetEnvironmentVariable(TEXT("APPDATA"));
	if (AppData.IsEmpty())
		return FString();
	return FPaths::Combine(AppData, TEXT("Tentacle"));
#elif PLATFORM_MAC
	const FString Home = FPlatformProcess::UserHomeDir();
	if (Home.IsEmpty())
		return FString();
	return FPaths::Combine(Home, TEXT("Library"), TEXT("Application Support"), TEXT("Tentacle"));
#else
	return FString();
#endif
}

FTclSidecarEndpoints
FTclSidecarEndpoints::Resolve(const FString& DataDirectory)
{
	const FString Directory = DataDirectory.IsEmpty() ? DefaultDataDirectory() : DataDirectory;
	if (Directory.IsEmpty())
		return FTclSidecarEndpoints();

	const FString Path = FPaths::Combine(Directory, DiscoveryFileName);
	if (!FPaths::FileExists(Path))
		return FTclSidecarEndpoints();

	// The sidecar rewrites this file while it starts, so a read failing on a
	// half-written or locked file is a normal race, not an error.
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		tclWarnF("Tentacle: could not read %s; using the default ports", *Path);
		return FTclSidecarEndpoints();
	}

	return ParseDiscoveryJson(Text);
}

FTclSidecarEndpoints
FTclSidecarEndpoints::ParseDiscoveryJson(const FString& Json)
{
	FTclSidecarEndpoints Endpoints;

	TSharedPtr<FJsonObject> Root;
	const auto Reader = TJsonReaderFactory<>::Create(Json);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		tclWarn("Tentacle: sidecar.json is not a JSON object; using the default ports");
		return Endpoints;
	}

	// Each field falls back independently: the sidecar omits the engine keys
	// entirely when its listener could not start, and the HTTP half is still
	// usable in that state.
	FString HttpBaseUrl;
	double HttpPort = 0;
	if (Root->TryGetStringField(TEXT("baseUrl"), HttpBaseUrl) && !HttpBaseUrl.IsEmpty())
	{
		Endpoints.HttpBaseUrl = HttpBaseUrl;
		Endpoints.bFromDiscoveryFile = true;
	}
	else if (Root->TryGetNumberField(TEXT("port"), HttpPort) && HttpPort > 0)
	{
		Endpoints.HttpBaseUrl = FString::Printf(TEXT("http://127.0.0.1:%d"), static_cast<int32>(HttpPort));
		Endpoints.bFromDiscoveryFile = true;
	}

	FString EngineUrl;
	double EnginePort = 0;
	if (Root->TryGetStringField(TEXT("engineUrl"), EngineUrl) && !EngineUrl.IsEmpty())
	{
		Endpoints.EngineSocketUrl = EngineUrl;
		Endpoints.bFromDiscoveryFile = true;
	}
	else if (Root->TryGetNumberField(TEXT("enginePort"), EnginePort) && EnginePort > 0)
	{
		Endpoints.EngineSocketUrl = FString::Printf(TEXT("ws://127.0.0.1:%d"), static_cast<int32>(EnginePort));
		Endpoints.bFromDiscoveryFile = true;
	}

	return Endpoints;
}
