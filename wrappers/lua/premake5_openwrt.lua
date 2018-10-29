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
		"../../src",
		"../../src/platform/linux",
	}
	files { 
		"./src/**.hpp",
		"./src/**.cpp",
		"./sol/**.hpp",
	}

	-- buildoptions { '-Wno-unknown-warning', '-Wno-unknown-warning-option', '-Wall', '-Wextra', '-Wpedantic', '-pedantic', '-pedantic-errors', '-Wno-noexcept-type', '-std=c++14', '-ftemplate-depth=1024' }
	buildoptions { '-Wpedantic', '-pedantic', '-pedantic-errors', '-DSOL_NO_EXCEPTIONS=1' }

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
		"../../src/",
		"../../src/platform/linux",
		"../../src/protocols",
		"../../src/utils",
	}
	files { 
		"../../src/lib/**.h",
		"../../src/lib/**.c",
		"../../src/platform/linux/**.h",
		"../../src/platform/linux/**.c",
		"../../src/protocols/**.h",
		"../../src/protocols/**.c",
		"../../src/util/**.h",
		"../../src/util/**.c",
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


