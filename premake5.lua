workspace "ArduinoGamepadEmulator"
	architecture "x86"

	configurations
	{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "ArduinoGamepadEmulator"
	location "ArduinoGamepadEmulator"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		
	}

	libdirs
	{
		
	}

	links 
	{ 
	}

	filter "system:windows"
		cppdialect "C++17"
		systemversion "latest"

		defines
		{
			
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

		links
		{

		}

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

		links
		{

		}