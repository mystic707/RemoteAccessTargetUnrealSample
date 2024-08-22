// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RemoteAccessTest003 : ModuleRules
{
	public RemoteAccessTest003(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// TODO : 
		PrivateDependencyModuleNames.AddRange(new string[] { "Networking", "Sockets" });
	}
}
