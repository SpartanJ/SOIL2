#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

#include "../SOIL2/SOIL2.h"

#define NO_SDL_GLEXT
#if ( ( defined( _MSCVER ) || defined( _MSC_VER ) ) || defined( __APPLE_CC__ ) || defined ( __APPLE__ ) ) && !defined( SOIL2_NO_FRAMEWORKS )
	#include <SDL.h>
	#include <SDL_opengl.h>
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_opengl.h>
#endif

#ifndef GL_RGB16F
#define GL_RGB16F 0x881B
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif

static int check_texture(
	GLenum target,
	GLint expected_internal_format,
	int expected_width,
	int expected_height,
	int mip_level )
{
	GLint internal_format = 0;
	GLint width = 0;
	GLint height = 0;
	glGetTexLevelParameteriv( target, mip_level, GL_TEXTURE_INTERNAL_FORMAT, &internal_format );
	glGetTexLevelParameteriv( target, mip_level, GL_TEXTURE_WIDTH, &width );
	glGetTexLevelParameteriv( target, mip_level, GL_TEXTURE_HEIGHT, &height );
	if( glGetError() != GL_NO_ERROR ||
		internal_format != expected_internal_format ||
		width != expected_width ||
		height != expected_height )
	{
		fprintf(
			stderr,
			"Unexpected texture level: format=%d size=%dx%d\n",
			internal_format, width, height );
		return 0;
	}

	std::vector<float> pixels( (size_t)width * (size_t)height * 3 );
	glGetTexImage( target, mip_level, GL_RGB, GL_FLOAT, pixels.data() );
	if( glGetError() != GL_NO_ERROR )
	{
		fprintf( stderr, "Could not read native HDR texture data\n" );
		return 0;
	}
	if( *std::max_element( pixels.begin(), pixels.end() ) <= 1.0f )
	{
		fprintf( stderr, "Native HDR texture lost values above 1.0\n" );
		return 0;
	}
	return 1;
}

static int load_file_data( const char *path, std::vector<unsigned char> *data )
{
	std::ifstream input( path, std::ios::binary );
	if( !input )
		return 0;
	data->assign(
		std::istreambuf_iterator<char>( input ),
		std::istreambuf_iterator<char>() );
	return !data->empty();
}

int main( int argc, char **argv )
{
	const char *fixture = argc > 1 ? argv[1] : "bin/test_native_hdr.hdr";
	std::vector<unsigned char> encoded_hdr;
	SDL_Window *window;
	SDL_GLContext context;
	GLuint texture;
	int success = 1;

	if( !load_file_data( fixture, &encoded_hdr ) )
	{
		fprintf( stderr, "Could not read %s\n", fixture );
		return 1;
	}
	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError() );
		return 1;
	}

	window = SDL_CreateWindow(
		"SOIL2 native HDR test", 0, 0, 16, 16,
		SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
	context = window ? SDL_GL_CreateContext( window ) : NULL;
	if( context == NULL )
	{
		fprintf( stderr, "OpenGL context creation failed: %s\n", SDL_GetError() );
		if( window )
			SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	texture = SOIL_load_OGL_HDR_texture_f32(
		fixture, SOIL_HDR_TEXTURE_RGB16F, SOIL_CREATE_NEW_ID, 0 );
	if( texture == 0 )
	{
		fprintf( stderr, "RGB16F file load failed: %s\n", SOIL_last_result() );
		success = 0;
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, texture );
		success &= check_texture( GL_TEXTURE_2D, GL_RGB16F, 16, 16, 0 );
		glDeleteTextures( 1, &texture );
	}

	texture = SOIL_load_OGL_HDR_texture_f32_from_memory(
		encoded_hdr.data(), (int)encoded_hdr.size(),
		SOIL_HDR_TEXTURE_RGB32F, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS );
	if( texture == 0 )
	{
		fprintf( stderr, "RGB32F memory load failed: %s\n", SOIL_last_result() );
		success = 0;
	}
	else
	{
		glBindTexture( GL_TEXTURE_2D, texture );
		success &= check_texture( GL_TEXTURE_2D, GL_RGB32F, 16, 16, 0 );
		success &= check_texture( GL_TEXTURE_2D, GL_RGB32F, 8, 8, 1 );
		glDeleteTextures( 1, &texture );
	}

	texture = SOIL_load_OGL_HDR_cubemap_f32(
		fixture, fixture, fixture, fixture, fixture, fixture,
		SOIL_HDR_TEXTURE_RGB16F, SOIL_CREATE_NEW_ID, 0 );
	if( texture == 0 )
	{
		fprintf( stderr, "RGB16F cubemap file load failed: %s\n", SOIL_last_result() );
		success = 0;
	}
	else
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP, texture );
		success &= check_texture(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_RGB16F, 16, 16, 0 );
		glDeleteTextures( 1, &texture );
	}

	texture = SOIL_load_OGL_HDR_cubemap_f32_from_memory(
		encoded_hdr.data(), (int)encoded_hdr.size(),
		encoded_hdr.data(), (int)encoded_hdr.size(),
		encoded_hdr.data(), (int)encoded_hdr.size(),
		encoded_hdr.data(), (int)encoded_hdr.size(),
		encoded_hdr.data(), (int)encoded_hdr.size(),
		encoded_hdr.data(), (int)encoded_hdr.size(),
		SOIL_HDR_TEXTURE_RGB32F, SOIL_CREATE_NEW_ID, 0 );
	if( texture == 0 )
	{
		fprintf( stderr, "RGB32F cubemap memory load failed: %s\n", SOIL_last_result() );
		success = 0;
	}
	else
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP, texture );
		success &= check_texture(
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, GL_RGB32F, 16, 16, 0 );
		glDeleteTextures( 1, &texture );
	}

	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();

	if( success )
		printf( "Native HDR texture tests passed\n" );
	return success ? 0 : 1;
}
