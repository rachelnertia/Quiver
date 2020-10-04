dofile("premake_sfml.lua")

function SetupQuiverWorkspace()
    GetSFMLOptions()

    configurations { "Debug", "Development" }

	language "C++"

	location "PremakeGenerated"
	
	targetdir "Build/%{prj.name}"
	targetname "%{prj.name}_%{cfg.longname}"
	objdir "Build/Obj/%{prj.name}/%{cfg.longname}"
	debugdir "Build/%{prj.name}"

	debugformat "c7"

	cppdialect "C++17"

	flags { "MultiProcessorCompile" }

	if os.istarget("linux") then
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
end

QuiverDirectory = ""

function SetQuiverDirectory(directory)
   QuiverDirectory = directory  
end

function Box2dProject()
    project "Box2D"
        kind "StaticLib"
        files 
        { 
            QuiverDirectory .. "External/Box2D/Box2D/**.h", 
            QuiverDirectory .. "External/Box2D/Box2D/**.cpp" 
        }
        includedirs 
        { 
            QuiverDirectory .. "External/Box2D" 
        }
end

function ImGuiSFMLProject()
    project "ImGui-SFML"
        kind "StaticLib"
        files
        {
            QuiverDirectory .. "External/ImGui/**.h", 
            QuiverDirectory .. "External/ImGui/**.cpp" 
        }
        includedirs
        {
            QuiverDirectory .. "External/ImGui/ImGui"
        }
        IncludeSFML()
end

function IncludeQuiver()
    includedirs 
    { 
        QuiverDirectory .. "Source/Quiver",
        QuiverDirectory .. "External/ImGui",
        QuiverDirectory .. "External/json", 
        QuiverDirectory .. "External/spdlog/include",
        QuiverDirectory .. "External/Box2D",
        QuiverDirectory .. "External/Optional",
        QuiverDirectory .. "External/gsl",
        QuiverDirectory .. "External/cxxopts",
        QuiverDirectory .. "External/function2",
        QuiverDirectory .. "External/NamedType"
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
    filter "system:linux"
        links { "pthread" }
    filter ()
end

function QuiverProject()
    project "Quiver"
        kind "StaticLib"
        files 
        { 
            QuiverDirectory .. "Source/Quiver/**.h", 
            QuiverDirectory .. "Source/Quiver/**.cpp"
        }
        IncludeQuiver()
        IncludeSFML()
        configuration "vs*"
            warnings "Extra"
end

function QuiverTestsProject()
    project "QuiverTests"
        kind "ConsoleApp"
        files 
        { 
            QuiverDirectory .. "Source/QuiverTests/**.cpp", 
            QuiverDirectory .. "Source/QuiverTests/**.h" 
        }
        includedirs
        {
            QuiverDirectory .. "External/Catch",
        }
        IncludeQuiver()
        LinkQuiver()
        defines "CATCH_CPP11_OR_GREATER"
end

function QuiverAppProject()
    project "QuiverApp"
		kind "ConsoleApp"
		files
		{
			QuiverDirectory .. "Source/QuiverApp/**"
		}
		IncludeQuiver()
		LinkQuiver()
end