function newplatform(plf)
	local name = plf.name
	local description = plf.description

	-- Register new platform
	premake.platforms[name] = {
		cfgsuffix = "_"..name,
		iscrosscompiler = true
	}

	-- Allow use of new platform in --platfroms
	table.insert(premake.option.list["platform"].allowed, { name, description })
	table.insert(premake.fields.platforms.allowed, name)

	-- Add compiler support
	premake.gcc.platforms[name] = plf.gcc
end

function newgcctoolchain(toolchain)
	newplatform {
		name = toolchain.name,
		description = toolchain.description,
		gcc = {
			cc = toolchain.prefix .. "gcc",
			cxx = toolchain.prefix .. "g++",
			ar = toolchain.prefix .. "ar",
			cppflags = "-MMD " .. toolchain.cppflags
		}
	}
end

newplatform {
	name = "clang",
	description = "Clang",
	gcc = {
		cc = "clang",
		cxx = "clang++",
		ar = "ar",
		cppflags = "-MMD "
	}
}

newgcctoolchain {
	name = "mingw32",
	description = "Mingw32 to cross-compile windows binaries from *nix",
	prefix = "i686-w64-mingw32-",
	cppflags = ""
}

if _OPTIONS.platform then
	-- overwrite the native platform with the options::platform
	premake.gcc.platforms['Native'] = premake.gcc.platforms[_OPTIONS.platform]
end

function string.starts(String,Start)
	if ( _ACTION ) then
		return string.sub(String,1,string.len(Start))==Start
	end

	return false
end

function is_vs()
	return ( string.starts(_ACTION,"vs") )
end

function os.is_real( os_name )
	return os.get() == os_name
end

function os_findlib( name )
	if os.is_real("macosx") then
		local path = "/Library/Frameworks/" .. name .. ".framework"

		if os.isdir( path ) then
			return path
		end
	end

	return os.findlib( name )
end

function get_backend_link_name( name )
	if os.is_real("macosx") then
		local fname = name .. ".framework"

		if os_findlib( name ) then -- Search for the framework
			return fname
		end
	end

	return name
end

solution "SOIL2"
	location("./make/" .. os.get() .. "/")
	targetdir("./bin")
	configurations { "debug", "release" }
	objdir("obj/" .. os.get() .. "/")

	project "soil2-static-lib"
		kind "StaticLib"
		language "C"

		if is_vs() then
			buildoptions { "/TP" }
			defines { "_CRT_SECURE_NO_WARNINGS" }
		end

		targetdir("lib/" .. os.get() .. "/")
		files { "src/SOIL2/*.c" }

		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "soil2"

	project "soil2-shared-lib"
		kind "SharedLib"
		language "C"

		if is_vs() then
			buildoptions { "/TP" }
			defines { "_CRT_SECURE_NO_WARNINGS" }
		end

		targetdir("lib/" .. os.get() .. "/")
		files { "src/SOIL2/*.c" }

		if os.is("windows") and not is_vs() then
			links { "mingw32" }
		end

		configuration "mingw32"
			flags { "EnableSSE", "EnableSSE2" }
			defines { "STBI_MINGW_ENABLE_SSE2" }
			links { "mingw32" }

		configuration "windows"
			links {"opengl32"}

		configuration "linux"
			links {"GL"}

		configuration "macosx"
			links { "OpenGL.framework", "CoreFoundation.framework" }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}

		configuration "haiku"
			links {"GL"}

		configuration "freebsd"
			links {"GL"}

		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "soil2"

	project "soil2-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/test/*.cpp", "src/common/*.cpp" }

		if os.is("windows") and not is_vs() then
			links { "mingw32" }
		end

		configuration "mingw32"
			flags { "EnableSSE", "EnableSSE2" }
			defines { "STBI_MINGW_ENABLE_SSE2" }
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
			flags { "Symbols" }
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-test-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "soil2-test-release"

	project "soil2-perf-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/perf_test/*.cpp", "src/common/*.cpp" }

		if os.is("windows") and not is_vs() then
			links { "mingw32" }
		end

		configuration "mingw32"
			flags { "EnableSSE", "EnableSSE2" }
			defines { "STBI_MINGW_ENABLE_SSE2" }
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
			flags { "Symbols" }
			if not is_vs() then
				buildoptions{ "-Wall" }
			end
			targetname "soil2-perf-test-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "soil2-perf-test-release"
