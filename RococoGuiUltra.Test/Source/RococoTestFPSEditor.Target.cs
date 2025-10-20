// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class RococoTestFPSEditorTarget : TargetRules
{
    public RococoTestFPSEditorTarget(TargetInfo Target) : base(Target)
	{
    //    BuildEnvironment = TargetBuildEnvironment.Unique;
    //    bUseLoggingInShipping = true;
        Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("RococoTestFPS");
    }
}
