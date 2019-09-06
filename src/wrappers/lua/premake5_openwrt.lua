-- A solution
workspace "lua-plctag"
	configurations { "Debug", "Release"}
	location "build"

project "plctag"
	kind "SharedLib"
	language "C++"
	location "build/lua-plctag"
	targetprefix ""
	targetdir "bin/%{cfg.buildcfg}"
	dependson { "plctag-static" }
	cppdialect "C++14"

	includedirs { 
		"/home/cch/mycode/skynet/3rd/lua/",
		--"/usr/include/lua5.3",
		".",
		"../../",
		"../../platform/linux",
	}
	files { 
		"./src/**.hpp",
		"./src/**.cpp",
		"./sol/**.hpp",
	}

	-- buildoptions { '-Wno-unknown-warning', '-Wno-unknown-warning-option', '-Wall', '-Wextra', '-Wpedantic', '-pedantic', '-pedantic-errors', '-Wno-noexcept-type', '-std=c++14', '-ftemplate-depth=1024' }
	buildoptions { '-Wpedantic', '-pedantic', '-pedantic-errors', '-DSOL_NO_EXCEPTIONS=1', '-ftemplate-depth=2048' }

	--libdirs { "../../bin" }
	links { "pthread", "plctag-static" }
	--linkoptions { "" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

project "plctag-static"
	kind "StaticLib"
	language "C++"
	location "build/plctag"
	targetdir "bin/%{cfg.buildcfg}"

	includedirs { 
		"../../",
		"../../platform/linux",
		"../../protocols",
		"../../utils",
	}
	files { 
		"../../lib/**.h",
		"../../lib/**.c",
		"../../platform/linux/**.h",
		"../../platform/linux/**.c",
		"../../protocols/**.h",
		"../../protocols/**.c",
		"../../util/**.h",
		"../../util/**.c",
	}

	buildoptions { '-Wpedantic', '-pedantic', '-pedantic-errors', "-fPIC" }

	--libdirs { "../../bin" }
	links { "pthread"}
	--linkoptions { "" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"


