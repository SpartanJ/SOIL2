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

solution "SOIL2"
	location("./make/" .. os.get() .. "/")
	targetdir("./bin")
	configurations { "debug", "release" }
	objdir("obj/" .. os.get() .. "/")
	
	project "soil2-static-lib"
		kind "StaticLib"
		if is_vs() then
			language "C++"
			buildoptions { "/TP" }
		else
			language "C"
		end
		targetdir("./lib")
		files { "src/SOIL2/*.c" }

		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "soil2-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			targetname "soil2"

	project "soil2-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/test/*.cpp" }

		configuration "windows"
			links {"gdi32","winmm","user32","glfw","glu32","opengl32"}

		configuration "linux"
			links {"GL","glfw"}
		
		configuration "macosx"
			links {"OpenGL.framework","CoreFoundation.framework","glfw"}
		
		configuration "haiku"
			links {"GL","glfw"}

		configuration "freebsd"
			links {"GL","glfw"}
		
		configuration "debug"
			defines { "DEBUG" }
			flags { "Symbols" }
			buildoptions{ "-Wall" }
			targetname "soil2-test-debug"

		configuration "release"
			defines { "NDEBUG" }
			flags { "Optimize" }
			buildoptions{ "-Wall" }
			targetname "soil2-test-release"
