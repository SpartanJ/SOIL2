#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <string>
#include <iostream>
#include <vector>
#include "../common/common.hpp"
#include "../SOIL2/SOIL2.h"

#define NO_SDL_GLEXT
#if ( ( defined( _MSCVER ) || defined( _MSC_VER ) ) || defined( __APPLE_CC__ ) || defined ( __APPLE__ ) ) && !defined( SOIL2_NO_FRAMEWORKS )
	#include <SDL.h>
	#include <SDL_opengl.h>
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_opengl.h>
#endif

#ifndef GL_REFLECTION_MAP
#define GL_REFLECTION_MAP 0x8512
#endif

#ifndef GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP 0x8513
#endif

double get_total_ms( Uint64 time_start, Uint64 time_end )
{
	return ( (double)(time_end - time_start) / (double)SDL_GetPerformanceFrequency() ) * 1000;
}

void DoTest(std::vector<std::string> args);

int main( int argc, char* argv[] )
{
	// Initialise SDL2
	if( 0 != SDL_Init( SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS ) )
	{
		fprintf( stderr, "Failed to initialize SDL2: %s\n", SDL_GetError() );
		exit( EXIT_FAILURE );
	}

	// Open OpenGL window
	SDL_Window * window = SDL_CreateWindow( "SOIL2 Perf Test",SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );

	if( NULL == window )
	{
		fprintf( stderr, "Failed to open SDL2 window: %s\n", SDL_GetError() );
		exit( EXIT_FAILURE );
	}

	SDL_GLContext context = SDL_GL_CreateContext( window );

	if ( NULL == context )
	{
		fprintf( stderr, "Failed to create SDL2 OpenGL Context: %s\n", SDL_GetError() );

		SDL_DestroyWindow( window );

		exit( EXIT_FAILURE );
	}

	SDL_GL_SetSwapInterval( 1 );

	SDL_GL_MakeCurrent( window, context );

	std::vector<std::string> args;
	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	DoTest(args);

	// Close OpenGL window and terminate SDL2
	SDL_GL_DeleteContext( context );

	SDL_DestroyWindow( window );

	exit( EXIT_SUCCESS );
}

void CalculateTextureObjectPixels(GLuint texID, unsigned long *pixelCount, unsigned long *memoryUsed)
{
	int baseLevel = 0, maxLevel = 0;
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, &baseLevel);
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, &maxLevel);

	long pixels = 0, bytes = 0, bpp = 0;
	int texW = 0, texH = 0, texFmt = 0, compressed = 0, compressedBytes = 0;
	for (int level = baseLevel; level <= maxLevel; ++level)
	{
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_WIDTH, &texW);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_HEIGHT, &texH);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_INTERNAL_FORMAT, &texFmt);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED, &compressed);
		pixels += texW*texH;

		if (compressed == GL_TRUE)
		{
			glGetTexLevelParameteriv(GL_TEXTURE_2D, level, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressedBytes);
			bytes += compressedBytes;
		}
		else
		{
			if (texFmt == GL_RGB || texFmt == GL_RGB8)
				bpp = 3;
			else if (texFmt == GL_RGBA || texFmt == GL_RGBA8)
				bpp = 4;
			else
				bpp = 0;

			bytes += texW*texH*bpp;
		}

		if (texW <= 1 && texH <= 1) // break when size is also 0
			break;
	}
	*pixelCount += pixels;
	*memoryUsed += bytes;
}

struct TestParams
{
	std::vector<std::string> files;
	std::string testName;
	int numberOfLoads;
	unsigned int soilFlags;
};

struct TestOutputs
{
	TestOutputs() : durationInMsec(0.0), totalPixels(0), totalMemoryInBytes(0) { }

	double durationInMsec;
	unsigned long totalPixels;
	unsigned long totalMemoryInBytes;
};

TestOutputs LoadTest(const TestParams &params)
{
	const size_t numFiles = params.files.size();
	std::vector<GLuint> texID(params.numberOfLoads);
  
	TestOutputs outputs;

	for (int i = 0; i < params.numberOfLoads; ++i)
	{
		Uint64 t_start = SDL_GetPerformanceCounter();
		{
			texID[i] = SOIL_load_OGL_texture(params.files[i%numFiles].c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, params.soilFlags);
		}
		Uint64 t_end = SDL_GetPerformanceCounter();
		outputs.durationInMsec += get_total_ms(t_start, t_end);

		if (texID[i] == 0)
			std::cout << "error!, could not load " << params.files[i%numFiles] << std::endl;

		CalculateTextureObjectPixels(texID[i], &outputs.totalPixels, &outputs.totalMemoryInBytes);
	}

	for (int i = 0; i < params.numberOfLoads; ++i)
		glDeleteTextures(1, &texID[i]);

	return outputs;
}

void LoadFiles(const std::vector<std::string> &files)
{
	for (size_t i = 0; i < files.size(); ++i)
	{
		GLuint tex = SOIL_load_OGL_texture(files[i].c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);
		glDeleteTextures(1, &tex);
	}
}

void CalcaAndPrintTestResult(const TestParams &params, const TestOutputs &results)
{
	double durationInSec = (double)(results.durationInMsec*0.001);
	double memInMB  = results.totalMemoryInBytes/(double)(1024.0*1024);
	double memSpeed = memInMB/durationInSec;

	printf("%s: %2.2fsec, memory: %3.2f MB, speed %3.3f MB/s\n", params.testName.c_str(),
			(float)(durationInSec), (float)memInMB, (float)memSpeed);
}

void DoTest(std::vector<std::string> args)
{
	const int NUM_FILES = 3;
	const char *defaultFiles[NUM_FILES] = { "lenna1.jpg", "lenna2.jpg", "lenna3.jpg" };
	std::vector<std::string> files;

	const int NUM_LOADS = args.size() > 0 ? atoi(args[0].c_str()) : 50;
	printf("soil2_perf_test\n");
	printf("number of texture loads: %d\n", NUM_LOADS);

	for (size_t i = 1; i < args.size(); ++i)
		files.push_back(args[i]);

	if (files.empty())
	{
		for (int i = 0; i < NUM_FILES; ++i)
			files.push_back(ResourcePath(defaultFiles[i]));
	}

	// load files so that there can be cached... this way first test and the second
	// will have same background.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	LoadFiles(files);

	for (size_t i = 0; i < files.size(); ++i)
	{
		printf("Texture to load: %s\n", files[i].c_str());
	}

	// GL_MIPMAP
	{
		TestParams paramsMipGL;
		paramsMipGL.files = files;
		paramsMipGL.numberOfLoads = NUM_LOADS;
		paramsMipGL.soilFlags = SOIL_FLAG_GL_MIPMAPS;
		paramsMipGL.testName = "FLAG_GL_MIPMAPS";

		TestOutputs resultMipGL = LoadTest(paramsMipGL);

		CalcaAndPrintTestResult(paramsMipGL, resultMipGL);
	}

	// MIPMAP soil version
	{
		TestParams paramsMip;
		paramsMip.files = files;
		paramsMip.numberOfLoads = NUM_LOADS;
		paramsMip.soilFlags = SOIL_FLAG_MIPMAPS;
		paramsMip.testName = "FLAG_MIPMAPS";

		TestOutputs resultMip = LoadTest(paramsMip);

		CalcaAndPrintTestResult(paramsMip, resultMip);
	}
}
