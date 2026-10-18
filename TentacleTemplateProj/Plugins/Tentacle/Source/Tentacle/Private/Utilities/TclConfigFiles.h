// © 2025 Eldritch One. All rights reserved.

#pragma once
#include "CoreMinimal.h"

// Unity's TentacleConfigFiles and Godot's TclConfigFiles read the same way:
// every .json file in one folder, in name order, so the files the Tentacle app
// exports are picked up with no list to keep. The default folder, Config/Tentacle,
// is ENGINE_CONFIG_FOLDERS.unreal in packages/common/src/general/config-export.ts,
// and Tentacle.Build.cs stages it into a packaged game.

/** Reads the config files an embedded game ships from a folder. */
struct FTclConfigFiles
{
	/**
	 * The folder a ConfigDirectory setting names: relative to the project folder
	 * unless it is a full path, and the default, Config/Tentacle, when empty.
	 */
	static FString Resolve(const FString& ConfigDirectory);

	/**
	 * The text of every .json file in Directory, in name order. Empty when the
	 * folder is not there, which is a game that ships no config. A file that
	 * cannot be read is reported and skipped. Reads a packaged game's pak as
	 * well as the folder on disk.
	 */
	static TArray<FString> Read(const FString& Directory);
};
