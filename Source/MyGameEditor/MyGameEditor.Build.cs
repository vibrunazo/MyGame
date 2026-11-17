using UnrealBuildTool;

public class MyGameEditor : ModuleRules
{
	public MyGameEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags", "AssCore", "AssCombat", "MyGame" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "Json", "AIModule" });
	}
}
