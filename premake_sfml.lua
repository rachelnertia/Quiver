function GetSFMLOptions()
	newoption {
		trigger = "sfmlinc",
		value = "path",
		description = "Specify SFML include directory"
	}

	newoption {
		trigger = "sfmllib",
		value = "path",
		description = "Specify SFML lib directory"
	}

	newoption {
		trigger = "sfml-link-dynamic",
		description = "Choose to dynamically link SFML libraries with the executables"
	}

	if not _OPTIONS["sfmlinc"] then
		print("Warning: The path to SFML include directory must be specified using --sfmlinc=<path>")
	end

	if not _OPTIONS["sfmllib"] then
		print("Warning: The path to SFML include directory must be specified using --sfmllib=<path>")
	end
end

function IncludeSFML()
	includedirs { _OPTIONS["sfmlinc"] }
	if not _OPTIONS["sfml-link-dynamic"] then
		defines { "SFML_STATIC" }
	end
end

function LinkSFML()
	libdirs	{ _OPTIONS["sfmllib"] }

	if _OPTIONS["sfml-link-dynamic"] then
		filter "configurations:Development"
			links { 
				"sfml-system", 
				"sfml-window", 
				"sfml-graphics", 
				"sfml-audio" 
			}
		
		filter "configurations:Debug"
			links { 
				"sfml-system-d", 
				"sfml-window-d", 
				"sfml-graphics-d", 
				"sfml-audio-d" 
			}
	
	else
		filter "configurations:Development"
			links { 
				"sfml-system-s", 
				"sfml-window-s", 
				"sfml-graphics-s", 
				"sfml-audio-s" 
			}
		
		filter "configurations:Debug"
			links { 
				"sfml-system-s-d", 
				"sfml-window-s-d", 
				"sfml-graphics-s-d", 
				"sfml-audio-s-d" 
			}

		filter "configurations:Development or Debug"
			links { 
					"winmm",
					"gdi32", 
					"openal32", 
					"flac", 
					"vorbisenc", 
					"vorbisfile", 
					"vorbis",
					"ogg",
					"freetype",
					"jpeg" 
				}

		filter()
	end

	filter()
end

function LinkGL()
	filter "system:windows"
		links { "OpenGL32" }
	filter "system:not windows"
		links { "GL" }
	filter()
end
