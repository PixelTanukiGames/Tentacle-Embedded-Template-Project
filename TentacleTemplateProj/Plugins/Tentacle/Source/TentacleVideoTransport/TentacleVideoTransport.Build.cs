using System;
using System.IO;
using UnrealBuildTool;

public class TentacleVideoTransport : ModuleRules
{
	public TentacleVideoTransport(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		string thirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/"));
		string spoutLibPath = Path.Combine(thirdPartyPath, "Library/Spout");
		string spoutDllPath = Path.Combine(thirdPartyPath, "Bin/Spout");
		string spoutIncludePath = Path.Combine(thirdPartyPath, "Include/Spout");
		
		PublicIncludePaths.Add(spoutIncludePath);

		LoadLibs(spoutLibPath, new string[]
		{
			"SpoutDX.lib"
		});

		LoadDlls(spoutDllPath, new string[]
		{
			"SpoutDX.dll"
		});
		
		// Add public module dependencies.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
		});	
		
		// Add private module dependencies.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"RenderCore",
			"RHI"
		});
		
		//
		// Helper functions.
		//
		void LoadLibs(string path, string[] fileNames)
		{
			// Add the libraries to the list of publicly linked libraries.
			foreach (string fileName in fileNames)
				PublicAdditionalLibraries.Add(Path.Combine(path, fileName));
		}

		void LoadDlls(string path, string[] fileNames)
		{
			// Copy the DLLs to the Binaries directory.
			string binariesPath = Path.Combine(PluginDirectory, "Binaries/Win64/");
			Directory.CreateDirectory(binariesPath);
			foreach (string fileName in fileNames)
			{
				string srcPath = Path.Combine(path, fileName);
				string destPath = Path.Combine(binariesPath, fileName);
				if (!File.Exists(destPath))
					File.Copy(srcPath, destPath, true);

				RuntimeDependencies.Add($"$(TargetOutputDir)/{fileName}", destPath);
				Console.WriteLine($"TentacleVideoTransport: {srcPath} copied to {destPath}");
			}
		}
	}
}
