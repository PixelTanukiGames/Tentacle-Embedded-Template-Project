// © 2025 Eldritch One. All rights reserved.

#include "Utilities/TclConfigFiles.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Utilities/TclCommon.h"

FString
FTclConfigFiles::Resolve(const FString& ConfigDirectory)
{
	const FString Directory = ConfigDirectory.TrimStartAndEnd().IsEmpty() ? FString(TEXT("Config/Tentacle")) : ConfigDirectory.TrimStartAndEnd();
	return FPaths::IsRelative(Directory) ? FPaths::Combine(FPaths::ProjectDir(), Directory) : Directory;
}

TArray<FString>
FTclConfigFiles::Read(const FString& Directory)
{
	TArray<FString> Texts;
	TArray<FString> Names;
	IFileManager::Get().FindFiles(Names, *FPaths::Combine(Directory, TEXT("*.json")), true, false);
	Names.Sort();

	for (const FString& Name : Names)
	{
		const FString Path = FPaths::Combine(Directory, Name);
		FString Text;
		if (FFileHelper::LoadFileToString(Text, *Path))
			Texts.Add(MoveTemp(Text));
		else
			tclWarnF("Tentacle: could not read the config file %s; starting without it", *Path);
	}
	return Texts;
}
