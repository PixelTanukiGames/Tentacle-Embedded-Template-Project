// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

class FJsonObject;

/**
 * Who this process is, as told to Tentacle in the opening handshake.
 *
 * Tentacle hosts the engine socket and several sessions are live at once - an
 * editor PIE session next to a packaged build, games made in different engines
 * side by side - so every connection has to say what it is. Custom controls are scoped
 * to the session that registered them, which is why InstanceId must differ
 * between two processes running the same project: without that the app cannot
 * tell their controls apart.
 *
 * Features:
 * - Clamps every field to the 128-character limit the server enforces, so an
 *   over-long label degrades into a truncated one rather than a refused
 *   connection.
 * - Serializes to the exact `Session_Hello` frame.
 */
struct TENTACLE_API FTclEngineIdentity
{
	/** Maximum length of every hello field, matching the server's schema. */
	static constexpr int32 MaxFieldLength = 128;

	/** Engine family. Lowercased by the server; always "unreal" from this plugin. */
	FString Engine = TEXT("unreal");

	/** Host engine version, e.g. "5.7.0". Empty when unknown. */
	FString EngineVersion;

	/** Version of this plugin. */
	FString PluginVersion;

	/** Stable identifier for this process. Two processes running the same project must not share one. */
	FString InstanceId;

	/** Human-readable name shown in Tentacle's session list. */
	FString Label;

	/**
	 * The connection password the user set in Tentacle's Settings > System, sent
	 * as it is, never trimmed or truncated. Empty sends none, which is all
	 * Tentacle asks for until its user sets one.
	 */
	FString Password;

	/**
	 * Whether this identity carries what the server requires. The server closes
	 * the socket on an invalid hello, so the connection checks first and reports
	 * a usable message instead of a reconnect loop.
	 */
	bool
	IsValid(FString& OutProblem) const;

	/** Builds the `data` payload of a `Session_Hello` frame. */
	TSharedRef<FJsonObject>
	ToHelloPayload() const;

	/** Builds the complete `Session_Hello` frame as compact JSON text. */
	FString
	ToHelloMessage() const;

	/** Trims and truncates one field to what the server accepts. */
	static FString
	Clamp(const FString& Value);
};
