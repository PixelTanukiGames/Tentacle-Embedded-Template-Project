// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclTentacleMode.h"
#include "Misc/Parse.h"
#include "Utilities/TclCommon.h"

bool
TclTentacleMode::ResolvesToEmbedded(ETclTentacleMode EditorMode, ETclTentacleMode BuildMode, bool bPlayingInEditor, const TCHAR* CommandLine)
{
	FString Override;
	if (CommandLine != nullptr && FParse::Value(CommandLine, TEXT("tentacle-mode="), Override))
	{
		// "app" is what Paired mode was called before 1.2.0-alpha.3.
		if (Override.Equals(TEXT("paired"), ESearchCase::IgnoreCase) || Override.Equals(TEXT("app"), ESearchCase::IgnoreCase))
			return false;
		if (Override.Equals(TEXT("embedded"), ESearchCase::IgnoreCase))
			return true;
		tclWarnF("Tentacle: ignoring -tentacle-mode=%s; it takes paired or embedded", *Override);
	}

	return (bPlayingInEditor ? EditorMode : BuildMode) == ETclTentacleMode::Embedded;
}
