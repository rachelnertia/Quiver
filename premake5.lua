dofile("premake_quiver.lua")

workspace "Quiver"
	SetupQuiverWorkspace()

	Box2dProject()
	ImGuiSFMLProject()
	
	QuiverProject()
	QuiverTestsProject()
	QuiverAppProject()
