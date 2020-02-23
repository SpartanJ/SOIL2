newoption { trigger = "use-frameworks", description = "In macOS it will try to link the external libraries from its frameworks. For example, instead of linking against SDL2 it will link against SDL2.framework." }
newoption { trigger = "windows-vc-build", description = "This is used to build the framework in Visual Studio downloading its external dependencies and making them available to the VS project without having to install them manually." }

function string.starts(String,Start)
	if ( _ACTION ) then
		return string.sub(String,1,string.len(Start))==Start
	end

	return false
end

function is_xcode()
	return ( string.starts(_ACTION,"xcode") )
end

function incdirs( dirs )
	if is_xcode() then
		sysincludedirs { dirs }
	end
	includedirs { dirs }
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

remote_sdl2_version = "SDL2-2.0.10"
remote_sdl2_devel_vc_url = "https://www.libsdl.org/release/SDL2-devel-2.0.10-VC.zip"

function download_and_extract_dependencies()
	if _OPTIONS["windows-vc-build"] and not os.isdir("./" .. remote_sdl2_version) then
		print("Downloading: " .. remote_sdl2_devel_vc_url)
		local dest_dir = "./"
		local local_file = dest_dir .. remote_sdl2_version .. ".zip"
		local result_str, response_code = http.download(remote_sdl2_devel_vc_url, local_file)
		if response_code == 200 then
			print("Downloaded successfully to: " .. local_file)
			zip.extract(local_file, dest_dir)
			print("Extracted " .. local_file .. " into " .. dest_dir)
			ok = os.copyfile(dest_dir .. remote_sdl2_version .. "/lib/x86/SDL2.dll", "./bin/SDL2.dll")
			if not ok then
				print("Failed to copy SDL2.dll.")
			end
		else
			print("Failed to download:  " .. remote_sdl2_rul)
			exit(1)
		end
	end
end

workspace "SOIL2"
	location("./make/" .. os.target() .. "/")
	targetdir("./bin")
	configurations { "debug", "release" }
	platforms { "x86_64", "x86" }
	download_and_extract_dependencies()
	objdir("obj/" .. os.target() .. "/")

	filter "platforms:x86"
		architecture "x86"

	filter "platforms:x86_64"
		architecture "x86_64"

	project "soil2-static-lib"
		kind "StaticLib"
		targetdir("lib/" .. os.target() .. "/")
		files { "src/SOIL2/*.c" }

		filter "action:vs*"
			buildoptions { "/TP" }
			defines { "_CRT_SECURE_NO_WARNINGS" }

		filter "action:not vs*"
			language "C"
			buildoptions { "-Wall" }

		filter "configurations:debug"
			defines { "DEBUG" }
			symbols "On"
			targetname "soil2-debug"

		filter "configurations:release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2"

		filter "system:macosx"
			defines { "GL_SILENCE_DEPRECATION" }

	project "soil2-shared-lib"
		kind "SharedLib"

		targetdir("lib/" .. os.target() .. "/")
		files { "src/SOIL2/*.c" }

		filter "action:vs*"
			buildoptions { "/TP" }
			defines { "_CRT_SECURE_NO_WARNINGS" }

		filter { "system:windows", "action:not vs*" }
			links { "mingw32" }
			vectorextensions "SSE2"
			defines { "STBI_MINGW_ENABLE_SSE2" }

		filter "system:windows"
			links {"opengl32"}

		filter "system:linux"
			links {"GL"}

		filter "system:macosx"
			links { "OpenGL.framework", "CoreFoundation.framework" }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			defines { "GL_SILENCE_DEPRECATION" }

		filter "system:haiku"
			links {"GL"}

		filter "system:bsd"
			links {"GL"}

		filter "action:not vs*"
			language "C"
			buildoptions { "-Wall" }

		filter "configurations:debug"
			defines { "DEBUG" }
			symbols "On"
			targetname "soil2-debug"

		filter "configurations:release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2"

	project "soil2-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/test/*.cpp", "src/common/*.cpp" }

		filter { "system:windows", "action:not vs*" }
			links { "mingw32" }

		filter "system:windows"
			links {"opengl32","SDL2main","SDL2"}

		filter "system:linux"
			links {"GL","SDL2"}

		filter "system:macosx"
			links { "OpenGL.framework", "CoreFoundation.framework", get_backend_link_name("SDL2") }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
			defines { "GL_SILENCE_DEPRECATION" }
			if not _OPTIONS["use-frameworks"] then
				defines { "SOIL2_NO_FRAMEWORKS" }
			end

		filter "system:haiku"
			links {"GL","SDL2"}

		filter "system:bsd"
			links {"GL","SDL2"}

		filter "action:not vs*"
			buildoptions { "-Wall" }
			vectorextensions "SSE2"
			defines { "STBI_MINGW_ENABLE_SSE2" }

		filter "configurations:debug"
			defines { "DEBUG" }
			symbols "On"
			targetname "soil2-test-debug"

		filter "configurations:release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2-test-release"

		filter { "options:windows-vc-build", "system:windows", "platforms:x86" }
			syslibdirs { "./" .. remote_sdl2_version .."/lib/x86" }

		filter { "options:windows-vc-build", "system:windows", "platforms:x86_64" }
			syslibdirs { "./" .. remote_sdl2_version .."/lib/x64" }

		filter { "options:windows-vc-build", "system:windows" }
			incdirs { "./" .. remote_sdl2_version .. "/include" }

	project "soil2-perf-test"
		kind "ConsoleApp"
		language "C++"
		links { "soil2-static-lib" }
		files { "src/perf_test/*.cpp", "src/common/*.cpp" }

		filter { "system:windows", "action:not vs*" }
			links { "mingw32" }
			vectorextensions "SSE2"
			defines { "STBI_MINGW_ENABLE_SSE2" }

		filter "system:windows"
			links {"opengl32","SDL2main","SDL2"}

		filter "system:linux"
			links {"GL","SDL2"}

		filter "system:macosx"
			links { "OpenGL.framework", "CoreFoundation.framework", get_backend_link_name("SDL2") }
			buildoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			linkoptions {"-F /Library/Frameworks", "-F ~/Library/Frameworks"}
			includedirs { "/Library/Frameworks/SDL2.framework/Headers" }
			defines { "GL_SILENCE_DEPRECATION" }
			if not _OPTIONS["use-frameworks"] then
				defines { "SOIL2_NO_FRAMEWORKS" }
			end

		filter "system:haiku"
			links {"GL","SDL2"}

		filter "system:bsd"
			links {"GL","SDL2"}

		filter "action:not vs*"
			buildoptions { "-Wall" }

		filter "configurations:debug"
			defines { "DEBUG" }
			symbols "On"
			targetname "soil2-perf-test-debug"

		filter "configurations:release"
			defines { "NDEBUG" }
			optimize "On"
			targetname "soil2-perf-test-release"

		filter { "options:windows-vc-build", "system:windows", "platforms:x86" }
			syslibdirs { "./" .. remote_sdl2_version .."/lib/x86" }

		filter { "options:windows-vc-build", "system:windows", "platforms:x86_64" }
			syslibdirs { "./" .. remote_sdl2_version .."/lib/x64" }

		filter { "options:windows-vc-build", "system:windows" }
			incdirs { "./" .. remote_sdl2_version .. "/include" }
