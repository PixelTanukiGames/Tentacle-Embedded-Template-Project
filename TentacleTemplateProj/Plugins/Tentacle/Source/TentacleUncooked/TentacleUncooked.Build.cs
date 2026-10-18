// © 2026 Eldritch One. All rights reserved.

using UnrealBuildTool;

/// <summary>
/// The plugin's Blueprint editor nodes: the Twitch and Kick API request node,
/// which lays a request's fields out on the node itself. UncookedOnly, so it
/// loads in the editor and when Blueprints are compiled for a cook, and never in
/// a game: a compiled Blueprint calls the Tentacle module's functions directly.
/// </summary>
public class TentacleUncooked : ModuleRules
{
	public TentacleUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		CppStandard = CppStandardVersion.Cpp20;

		// IncludeOrderVersion was added in UE 5.1; set via reflection as the Tentacle module does.
		var includeOrderProp = this.GetType().GetProperty("IncludeOrderVersion");
		if (includeOrderProp != null)
		{
			var enumType = System.Type.GetType("UnrealBuildTool.EngineIncludeOrderVersion, UnrealBuildTool");
			if (enumType != null)
			{
				includeOrderProp.SetValue(this, System.Enum.Parse(enumType, "Latest"));
			}
		}

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"BlueprintGraph",
			"Core",
			"CoreUObject",
			"Engine",
			"GraphEditor",
			"Json",
			"KismetCompiler",
			"Slate",
			"SlateCore",
			"Tentacle",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
