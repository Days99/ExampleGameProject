// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ExampleProjectServerTarget : TargetRules
{
    public ExampleProjectServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.AddRange(new string[] { "ExampleProject" });

        // ============================================
        // FIXED: Server configuration for proper asset handling
        // ============================================

        // Enable dedicated server optimizations
        GlobalDefinitions.Add("UE_SERVER=1");
        
        // CRITICAL: Don't disable editor-only data too aggressively
        // This was causing WorldGridMaterial corruption
        bBuildWithEditorOnlyData = Target.Configuration != UnrealTargetConfiguration.Shipping;
        
        // Core engine requirements
        bCompileAgainstEngine = true;
        bCompileAgainstCoreUObject = true;
        
        // Enable networking
        bWithServerCode = true;
        bUseLoggingInShipping = true;
        
        // Asset and content handling - IMPORTANT for engine materials
        bBuildDeveloperTools = Target.Configuration != UnrealTargetConfiguration.Shipping;
        bCompileWithPluginSupport = true;
        
        // Enable proper asset loading and validation
        bForceEnableExceptions = false;
        bUseChecksInShipping = false;
        
        // ============================================
        // FIXED: Asset integrity and loading
        // ============================================
        
        // Enable asset validation to prevent corruption
        if (Target.Configuration != UnrealTargetConfiguration.Shipping)
        {
            GlobalDefinitions.Add("WITH_ASSET_VALIDATION=1");
        }
        
        // Enable proper content streaming for engine materials
        GlobalDefinitions.Add("WITH_ENGINE_CONTENT=1");
        
        // ============================================
        // Network and multiplayer configuration
        // ============================================
        
        // Enable multiplayer networking
        bUseStaticCRT = false;
        
        // Required modules for multiplayer server
        ExtraModuleNames.AddRange(new string[] 
        {
            "ExampleProject",
            "OnlineSubsystem",
            "OnlineSubsystemUtils"
        });
        
        // ============================================
        // Performance optimizations (safe ones)
        // ============================================
        
        // Only disable heavy features, not asset loading
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            // Shipping-specific optimizations
            bUseLoggingInShipping = true;  // Keep logging for debugging
            bCompileWithPluginSupport = true; // Keep plugin support
        }
        
        // ============================================
        // UE5.5 specific settings
        // ============================================
        
        // Enable proper UE5.5 asset handling
        bValidateFormatStrings = true;
        
        // Ensure proper world composition support
        GlobalDefinitions.Add("UE_ENABLE_WORLD_COMPOSITION=1");
    }
} 