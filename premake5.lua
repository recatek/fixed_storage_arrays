workspace "tests"
	configurations { "debug", "release" }
	
	language "C++"
	cppdialect "C++17"
	architecture "x64"
	
	warnings "Extra"
	targetdir "build/bin/%{cfg.buildcfg}"
	objdir "!build/obj/%{cfg.buildcfg}/%{prj.name}"
			
	-- Windows-specific platforms and functionality
	filter "action:vs*"
		defines { '__BASE_FILE__="%%(Filename)%%(Extension)"' }
		
	-- Linux-specific platforms and functionality
	filter "action:gmake"
		linkoptions { "-lm" }   
		buildoptions { "-Wno-unknown-pragmas" }
	  
	-- Global debug settings
	filter "configurations:debug"
		defines { "DEBUG" }
		symbols "On"
		rtti "On"
		optimize "Off"

	-- Global release settings
	filter "configurations:release"
		defines { "NDEBUG", "RELEASE" }
		symbols "Off"
		rtti "Off"
		optimize "Speed"

project "tests"
	kind "ConsoleApp"
	location "tests/"
	files { "include/**.h", "tests/**.h", "tests/**.cpp" }
