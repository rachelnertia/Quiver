dofile("premake_help.lua")

workspace "Quiver"
	newoption {
		trigger = "sfmldir",
		value = "path",
		description = "Specify SFML directory, containing lib, include and bin folders"
	}

	newoption {
		trigger = "sfml-link-dynamic",
		description = "Choose to dynamically link SFML libraries with the executables"
	}

	if not _OPTIONS["sfmldir"] then
		print("Warning: The path to SFML include and lib directories must be specified using --sfmldir=<path>")
	end

	configurations { "Debug", "Development" }

	language "C++"

	location "PremakeGenerated"
	
	targetdir "Build/%{prj.name}"
	targetname "%{prj.name}_%{cfg.longname}"
	objdir "Build/Obj/%{prj.name}/%{cfg.longname}"
	debugdir "Build/%{prj.name}"

	debugformat "c7"

	flags { "MultiProcessorCompile", "C++14" }

	if os.is("linux") then
		buildoptions { "--std=c++1z", "-fpermissive" }
	else
		flags { "FatalCompileWarnings" }
	end

	configuration "vs*"
		warnings "Extra"

	filter "action:vs*"
		defines { "_CRT_SECURE_NO_WARNINGS" }
		buildoptions { "/wd4100", "/permissive-", "/Zc:twoPhase-" }

	filter "Debug"
		symbols "On"
		optimize "Off"
		defines { "DEBUG", "_DEBUG", "DO_GL_CHECK" }

	filter { "Debug", "action:vs*" }
		buildoptions { "/wd4702" }

	filter "Development"
		symbols "On"
		optimize "On"
		defines { "DO_GL_CHECK" }

	filter { "Development", "action:vs*" }
		defines { "_ITERATOR_DEBUG_LEVEL=0" }

	filter()

	project "Box2D"
		kind "StaticLib"
		files 
		{ 
			"External/Box2D/Box2D/**.h", 
			"External/Box2D/Box2D/**.cpp" 
		}
		includedirs 
		{ 
			"External/Box2D" 
		}

	project "ImGui-SFML"
		kind "StaticLib"
		files
		{
			"External/ImGui/**.h", 
			"External/ImGui/**.cpp" 
		}
		IncludeSFML()

	project "Quiver"
		kind "StaticLib"
		files 
		{ 
			"Source/Quiver/**.h", 
			"Source/Quiver/**.cpp"
		}
		includedirs 
		{
			"Source/Quiver",
			"External/json",
			"External/spdlog/include",
			"External/Box2D",
			"External/ImGui",
			"External/cxxopts",
			"External/Optional"
		}
		IncludeSFML()
		configuration "vs*"
			warnings "Extra"

	function IncludeQuiver()
		includedirs 
		{ 
			"Source/Quiver",
			"External/ImGui",
			"External/json", 
			"External/spdlog/include",
			"External/Box2D",
			"External/Optional"
		}
		IncludeSFML()
	end

	function LinkQuiver()
		links
		{
			"Quiver", 
			"Box2D",
			"ImGui-SFML"
		}
		LinkSFML()
		LinkGL()
	end

	project "QuiverTests"
		kind "ConsoleApp"
		files 
		{ 
			"Source/QuiverTests/**.cpp", 
			"Source/QuiverTests/**.h" 
		}
		includedirs
		{
			"External/Catch",
		}
		IncludeQuiver()
		LinkQuiver()
		defines "CATCH_CPP11_OR_GREATER"

	project "QuiverApp"
		kind "ConsoleApp"
		files
		{
			"source/QuiverApp/**"
		}
		IncludeQuiver()
		LinkQuiver()

	project "Quarrel"
		kind "ConsoleApp"
		files 
		{ 
			"Source/Quarrel/**" 
		}
		IncludeQuiver()
		LinkQuiver()