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
