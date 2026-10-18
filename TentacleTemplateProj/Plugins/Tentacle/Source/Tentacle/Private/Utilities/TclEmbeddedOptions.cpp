// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclEmbeddedOptions.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString
FTclEmbeddedOptions::ToConfigJson() const
{
	// The core reads camelCase keys and ignores an absent one, so an empty
	// setting is left out rather than sent as an empty string it would take
	// literally.
	const TSharedRef<FJsonObject> Config = MakeShared<FJsonObject>();
	if (!DataDirectory.IsEmpty())
		Config->SetStringField(TEXT("dataDirectory"), DataDirectory);
	if (!TwitchClientId.IsEmpty())
		Config->SetStringField(TEXT("twitchClientId"), TwitchClientId);

	TArray<TSharedPtr<FJsonValue>> Files;
	for (const FString& File : ConfigFiles)
		Files.Add(MakeShared<FJsonValueString>(File));
	Config->SetArrayField(TEXT("configFiles"), Files);

	FString Json;
	const auto Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Json);
	FJsonSerializer::Serialize(Config, Writer);
	return Json;
}
