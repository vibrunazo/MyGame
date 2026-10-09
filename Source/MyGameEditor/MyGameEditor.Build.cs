using UnrealBuildTool;

public class MyGameEditor : ModuleRules
{
	public MyGameEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GameplayAbilities", "MyGame" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd" });
	}
}
