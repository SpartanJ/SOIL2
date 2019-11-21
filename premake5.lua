newoption { trigger = "use-frameworks", description = "In macOS it will try to link the external libraries from its frameworks. For example, instead of linking against SDL2 it will link against SDL2.framework." }

function string.starts(String,Start)
	if ( _ACTION ) then
		return string.sub(String,1,string.len(Start))==Start
	end

	return false
end

function is_vs()
	return ( string.starts(_ACTION,"vs") )
end

function is_xcode()
	return ( string.starts(_ACTION,"xcode") )
end

function os_findlib( name )
	if os.istarget("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		local path = "/Library/Frameworks/" .. name .. ".framework"

		if os.isdir( path ) then
			return path
		end
	end

	return os.findlib( name )
end

function get_backend_link_name( name )
	if os.istarget("macosx") and ( is_xcode() or _OPTIONS["use-frameworks"] ) then
		local fname = name .. ".framework"

		if os_findlib( name ) then -- Search for the framework
			return fname
		end
	end

	return name
end

workspace "SOIL2"
	location("./make/" .. os.target() .. "/")
	targetdir("./bin")
	configurations { "debug", "release" }
	objdir("obj/" .. os.target() .. "/")

	project "soil2-static-lib"
		kind "StaticLib"

		if is_vs() then
			language "C++"
			buildoptions { "/TP" }
			defines { "_CRT_SECURE_NO_WARNINGS" }
		else
			language "C"
		end

		targetdir("lib/" .. os.target() .. "/")
		files { "src/SOIL2/*.c" }

		configuration "debug"
			defines { "DEBUG" }
			symbols "On"
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-debug"

		configuration "release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2"

	project "soil2-shared-lib"
		kind "SharedLib"

		if is_vs() then
			language "C++"
			buildoptions { "/TP" }
			defines { "_CRT_SECURE_NO_WARNINGS" }
		else
			language "C"
		end

		targetdir("lib/" .. os.target() .. "/")
		files { "src/SOIL2/*.c" }

		if os.istarget("windows") and not is_vs() then
			links { "mingw32" }
		end

		configuration "mingw32"
			links { "mingw32" }

		configuration "windows"
			links {"opengl32","SDL2"}

		configuration "linux"
			links {"GL","SDL2"}

		configuration "macosx"
			links { "OpenGL.framework", "CoreFoundation.framework", get_backend_link_name("SDL2") }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			includedirs { "/Library/Frameworks/SDL2.framework/Headers" }

		configuration "haiku"
			links {"GL","SDL2"}

		configuration "freebsd"
			links {"GL","SDL2"}

		configuration "debug"
			defines { "DEBUG" }
			symbols "On"
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-debug"

		configuration "release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2"

	project "soil2-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/test/*.cpp", "src/common/*.cpp" }

		if os.istarget("windows") and not is_vs() then
			links { "mingw32" }
		end

		configuration "mingw32"
			links { "mingw32" }

		configuration "windows"
			links {"opengl32","SDL2main","SDL2"}

		configuration "linux"
			links {"GL","SDL2"}

		configuration "macosx"
			links { "OpenGL.framework", "CoreFoundation.framework", get_backend_link_name("SDL2") }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			includedirs { "/Library/Frameworks/SDL2.framework/Headers" }

		configuration "haiku"
			links {"GL","SDL2"}

		configuration "freebsd"
			links {"GL","SDL2"}

		configuration "debug"
			defines { "DEBUG" }
			symbols "On"
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-test-debug"

		configuration "release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2-test-release"

	project "soil2-perf-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/perf_test/*.cpp", "src/common/*.cpp" }

		if os.istarget("windows") and not is_vs() then
			links { "mingw32" }
		end

		configuration "mingw32"
			links { "mingw32" }

		configuration "windows"
			links {"opengl32","SDL2main","SDL2"}

		configuration "linux"
			links {"GL","SDL2"}

		configuration "macosx"
			links { "OpenGL.framework", "CoreFoundation.framework", get_backend_link_name("SDL2") }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			includedirs { "/Library/Frameworks/SDL2.framework/Headers" }

		configuration "haiku"
			links {"GL","SDL2"}

		configuration "freebsd"
			links {"GL","SDL2"}

		configuration "debug"
			defines { "DEBUG" }
			symbols "On"
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-perf-test-debug"

		configuration "release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2-perf-test-release"
