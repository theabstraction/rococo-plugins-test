// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class RococoTestFPS : ModuleRules
{
    public RococoTestFPS(ReadOnlyTargetRules Target) : base(Target)
	{
       	PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "SlateCore", "Slate", "EnhancedInput", "RHI", "ApplicationCore", "RococoGui" });

        if (Target.LinkType == TargetLinkType.Monolithic)
        {
            PublicDefinitions.Add("ROCOCO_BUILD_IS_MONOLITHIC");
        }
    }
}
