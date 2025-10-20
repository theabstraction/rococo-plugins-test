// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using UnrealBuildTool;

public class RococoTestFPSTarget : TargetRules
{
    public RococoTestFPSTarget(TargetInfo Target) : base(Target)
	{
		// Comment out TargetBuildEnvironment.Unique and bUseLoggingInShipping if you want to run with the stand-alone, non-locally-compiled version of UE.
		// If you have synched with some branch other than a UE5-GR-RC, you may find it was not commented out by default, as working copies in the master branch
		// are used with a locally-compiled version of UE. Switch branch to fix or add comment characters.
		// TODO - investigate possibility of detecting type of UE5 installation automatically and apply procedurally.
       // BuildEnvironment = TargetBuildEnvironment.Unique;
        //bUseLoggingInShipping = true;
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
        ExtraModuleNames.Add("RococoTestFPS");
    }
}
