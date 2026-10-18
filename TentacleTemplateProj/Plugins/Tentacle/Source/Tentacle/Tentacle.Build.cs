// © 2024 Eldritch One. All rights reserved.

using UnrealBuildTool;

public class Tentacle : ModuleRules
{
	public Tentacle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrecompileForTargets = PrecompileTargetsType.Any;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		CppStandard = CppStandardVersion.Cpp20;

		// IncludeOrderVersion was added in UE 5.1; set via reflection so this compiles on UE 5.0.
		var includeOrderProp = this.GetType().GetProperty("IncludeOrderVersion");
		if (includeOrderProp != null)
		{
			var enumType = System.Type.GetType("UnrealBuildTool.EngineIncludeOrderVersion, UnrealBuildTool");
			if (enumType != null)
			{
				includeOrderProp.SetValue(this, System.Enum.Parse(enumType, "Latest"));
			}
		}

		PublicIncludePaths.AddRange(new string[] {
			System.IO.Path.Combine(ModuleDirectory, "Public", "Sdk")
		});

		PrivateIncludePaths.AddRange(new string[] {
			System.IO.Path.Combine(ModuleDirectory, "Private", "Sdk"),
			ModuleDirectory,
			// The Tentacle core's C header, a copy of crates/tentacle-ffi's.
			System.IO.Path.Combine(PluginDirectory, "ThirdParty", "Include", "TentacleCore")
		});

		// The core's library is loaded at runtime from
		// Binaries/ThirdParty/TentacleCore/<platform>, never linked, so the plugin
		// builds without it; `just unreal::native` builds and installs it. When it
		// is there, a packaged game carries it beside the plugin.
		string CoreLibrary =
			Target.Platform == UnrealTargetPlatform.Win64 ? "tentacle_ffi.dll" :
			Target.Platform == UnrealTargetPlatform.Mac ? "libtentacle_ffi.dylib" :
			Target.Platform == UnrealTargetPlatform.Linux ? "libtentacle_ffi.so" : null;
		if (CoreLibrary != null)
		{
			string CoreFolder = "Binaries/ThirdParty/TentacleCore/" + Target.Platform.ToString();
			if (System.IO.File.Exists(System.IO.Path.Combine(PluginDirectory, CoreFolder, CoreLibrary)))
			{
				RuntimeDependencies.Add("$(PluginDir)/" + CoreFolder + "/" + CoreLibrary);
			}
		}

		StageConfigFiles(Target);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"HTTP",
			// The Connections Widget is a UUserWidget, so its public header needs UMG.
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"CoreUObject",
			"Engine",
			"Json",
			"JsonUtilities",
			"MassEntity",
			"Projects",
			"Slate",
			"SlateCore",
			"TentacleVideoTransport",
			"WebSockets"
		});

		// UE 5.8 split FMassFragment (and the other Mass element types) out of the
		// MassEntity module into a new MassCore module. FMassFragment is exported as
		// MASSCORE_API there, so a modular build must link MassCore directly to
		// resolve its reflection symbols; depending on MassEntity alone is not
		// enough. The module does not exist before 5.8.
		if (Target.Version.MajorVersion > 5
			|| (Target.Version.MajorVersion == 5 && Target.Version.MinorVersion >= 8))
		{
			PrivateDependencyModuleNames.Add("MassCore");
		}

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"FunctionalTesting",
				"UnrealEd"
			});
		}

		DynamicallyLoadedModuleNames.AddRange(new string[] { });
	}

	// The names the Tentacle app writes (CONFIG_FILE_NAMES in
	// packages/common/src/server/data-portability/config-files.ts) into the
	// folder it exports to for an Unreal project (ENGINE_CONFIG_FOLDERS.unreal in
	// packages/common/src/general/config-export.ts); the app's config-folders test
	// holds both to these.
	static readonly string[] ConfigFileNames =
	{
		"tentacle-timers.json",
		"tentacle-custom-events.json",
		"tentacle-viewer-properties.json",
		"tentacle-bot.json",
	};

	/// <summary>
	/// Ships the project's Config/Tentacle folder inside a packaged game: the config
	/// files the Tentacle app's Data &gt; Export writes there, which the Tentacle
	/// actor reads by default in Embedded mode. Unreal stages only .ini files from
	/// Config, so each .json file is staged here, into the pak.
	///
	/// The staged files are worked out when UBT builds its makefile, which a later
	/// package reuses; a file exported after that would be left out. So a game
	/// target also watches each of the app's file names, which makes UBT build its
	/// makefile again when one changes, appears, or is missing (a few seconds of a
	/// package). The editor target never stages anything, so it watches nothing.
	/// </summary>
	void StageConfigFiles(ReadOnlyTargetRules Target)
	{
		if (Target.ProjectFile == null || Target.Type == TargetType.Editor)
			return;

		string ConfigFolder = System.IO.Path.Combine(Target.ProjectFile.Directory.FullName, "Config", "Tentacle");
		if (System.IO.Directory.Exists(ConfigFolder))
		{
			foreach (string File in System.IO.Directory.GetFiles(ConfigFolder, "*.json"))
			{
				RuntimeDependencies.Add("$(ProjectDir)/Config/Tentacle/" + System.IO.Path.GetFileName(File), StagedFileType.UFS);
			}
		}
		foreach (string Name in ConfigFileNames)
		{
			ExternalDependencies.Add(System.IO.Path.Combine(ConfigFolder, Name));
		}
	}
}
