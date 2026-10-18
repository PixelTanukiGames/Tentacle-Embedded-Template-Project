// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Sdk/TclStreamPlatform.h"

class UTclPlatformConnection;
class UTclPlatformConnectionPrompt;
class UTclPlatformConnectionResult;

/** What a button in a Tentacle Connections Widget row does when pressed. */
enum class ETclConnectionsWidgetAction : uint8
{
	None,
	Connect,
	Disconnect,
	OpenPage,
	Cancel,
};

// Unity's ConnectionsWidgetRow and Godot's TclConnectionsWidgetRow, with the
// same rules and the same words, so a player sees the same sign-in on every
// engine. Not reflected: the widget is the surface, and a game wanting a
// sign-in screen of its own uses the Connections component directly.
/**
 * What one platform's row in a Tentacle Connections Widget shows: its status, a
 * line of detail, and its buttons, worked out from what the Connections
 * component last reported.
 */
struct TENTACLE_API FTclConnectionsWidgetRow
{
	FString Title;
	FString Status;
	FString Detail;
	ETclConnectionsWidgetAction PrimaryAction = ETclConnectionsWidgetAction::None;
	FString PrimaryLabel;
	ETclConnectionsWidgetAction SecondaryAction = ETclConnectionsWidgetAction::None;
	FString SecondaryLabel;

	/**
	 * The row for Platform, given its Connection (null before Tentacle has
	 * reported one), the sign-in Prompt in progress (null for none) with
	 * SecondsLeft on it, the Result of the last sign-in (null for none), and
	 * whether the widget opens the sign-in page by itself.
	 */
	static FTclConnectionsWidgetRow Describe(
		ETclStreamPlatform Platform,
		const UTclPlatformConnection* Connection,
		const UTclPlatformConnectionPrompt* Prompt,
		double SecondsLeft,
		const UTclPlatformConnectionResult* Result,
		bool bOpenBrowserAutomatically);

	/** The platform's name as a player knows it. */
	static FString PlatformName(ETclStreamPlatform Platform);

	/** Seconds as minutes and seconds, rounded up: 4:05. */
	static FString FormatTimeLeft(double Seconds);

	/** Why the last sign-in did not connect, or empty when it did, was cancelled, or there was none. */
	static FString ResultDetail(const UTclPlatformConnectionResult* Result);
};
