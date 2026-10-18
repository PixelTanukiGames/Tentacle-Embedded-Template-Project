// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "TclTentacleMode.generated.h"

/** Where the game finds Tentacle. */
UENUM(BlueprintType)
enum class ETclTentacleMode : uint8
{
	/** Paired with the Tentacle app running on this machine, which the streamer signs in to. */
	Paired,

	/**
	 * Tentacle running inside the game itself, with no app. The game signs the
	 * player in to Twitch and Kick (see the Connections component), and ships
	 * its config as files exported from the Tentacle app.
	 */
	Embedded,
};

namespace TclTentacleMode
{
	/**
	 * Whether Tentacle runs inside the game: EditorMode while playing in the
	 * editor, BuildMode anywhere else (a packaged game, or one run standalone).
	 * A -tentacle-mode=paired or -tentacle-mode=embedded argument on
	 * CommandLine wins over both, so a packaged game can be tested against the
	 * app; any other value is ignored with a warning.
	 */
	TENTACLE_API bool ResolvesToEmbedded(ETclTentacleMode EditorMode, ETclTentacleMode BuildMode, bool bPlayingInEditor, const TCHAR* CommandLine);
}
