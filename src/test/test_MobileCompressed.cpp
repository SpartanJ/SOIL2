#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
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

struct Fixture
{
	const char* filename;
	GLint internal_format;
	GLint alternate_internal_format;
	int astc;
	int flags;
};

static std::vector<unsigned char> read_file( const std::string& path )
{
	std::ifstream input( path.c_str(), std::ios::binary );
	return std::vector<unsigned char>(
		std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() );
}

static int check_texture( GLuint texture, const Fixture& fixture )
{
	GLint internal_format = 0;
	GLint compressed = 0;
	GLint width = 0;
	GLint height = 0;
	glBindTexture( GL_TEXTURE_2D, texture );
	glGetTexLevelParameteriv(
		GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &compressed );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );
	if( glGetError() != GL_NO_ERROR ||
	    ( internal_format != fixture.internal_format &&
	      internal_format != fixture.alternate_internal_format ) ||
	    compressed != GL_TRUE || width != 13 || height != 9 )
	{
		fprintf(
			stderr, "%s: unexpected format=%d compressed=%d size=%dx%d\n",
			fixture.filename, internal_format, compressed, width, height );
		return 0;
	}

	std::vector<float> pixels( (size_t)width * (size_t)height * 4 );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data() );
	if( glGetError() != GL_NO_ERROR )
	{
		fprintf( stderr, "%s: compressed texture readback failed\n", fixture.filename );
		return 0;
	}
	for( size_t i = 0; i < pixels.size(); ++i )
	{
		if( !std::isfinite( pixels[i] ) )
		{
			fprintf( stderr, "%s: readback contains non-finite values\n", fixture.filename );
			return 0;
		}
	}
	return 1;
}

static int test_fixture( const std::string& directory, const Fixture& fixture )
{
	const std::string path = directory + "/" + fixture.filename;
	GLuint texture =
		fixture.astc ?
			SOIL_direct_load_ASTC(
				path.c_str(), SOIL_CREATE_NEW_ID, fixture.flags ) :
			SOIL_direct_load_PKM(
				path.c_str(), SOIL_CREATE_NEW_ID, fixture.flags );
	if( texture == 0 )
	{
		if( fixture.astc &&
		    strstr( SOIL_last_result(), "not supported by this OpenGL context" ) )
		{
			printf( "%s skipped: %s\n", fixture.filename, SOIL_last_result() );
			return 1;
		}
		fprintf( stderr, "%s: %s\n", path.c_str(), SOIL_last_result() );
		return 0;
	}
	int success = check_texture( texture, fixture );
	glDeleteTextures( 1, &texture );

	const std::vector<unsigned char> data = read_file( path );
	texture =
		fixture.astc ?
			SOIL_direct_load_ASTC_from_memory(
				data.data(), (int)data.size(), SOIL_CREATE_NEW_ID, fixture.flags ) :
			SOIL_direct_load_PKM_from_memory(
				data.data(), (int)data.size(), SOIL_CREATE_NEW_ID, fixture.flags );
	if( texture == 0 )
	{
		fprintf( stderr, "%s from memory: %s\n", path.c_str(), SOIL_last_result() );
		return 0;
	}
	success &= check_texture( texture, fixture );
	glDeleteTextures( 1, &texture );
	return success;
}

static int expect_failure(
	const std::vector<unsigned char>& data, int astc, const char* expected_error )
{
	const GLuint texture =
		astc ?
			SOIL_direct_load_ASTC_from_memory(
				data.data(), (int)data.size(), SOIL_CREATE_NEW_ID, 0 ) :
			SOIL_direct_load_PKM_from_memory(
				data.data(), (int)data.size(), SOIL_CREATE_NEW_ID, 0 );
	if( texture != 0 || strstr( SOIL_last_result(), expected_error ) == NULL )
	{
		fprintf(
			stderr, "Expected failure containing '%s', got '%s'\n",
			expected_error, SOIL_last_result() );
		if( texture )
			glDeleteTextures( 1, &texture );
		return 0;
	}
	return 1;
}

int main( int argc, char** argv )
{
	static const Fixture fixtures[] = {
		{ "etc1_rgb8.pkm", 0x8D64, 0x9274, 0, 0 },
		{ "etc2_rgb8.pkm", 0x9274, 0x9274, 0, 0 },
		{ "etc2_rgba8_old.pkm", 0x9278, 0x9278, 0, 0 },
		{ "etc2_rgba8.pkm", 0x9278, 0x9278, 0, 0 },
		{ "etc2_rgb8a1.pkm", 0x9276, 0x9276, 0, 0 },
		{ "eac_r11.pkm", 0x9270, 0x9270, 0, 0 },
		{ "eac_r11_signed.pkm", 0x9271, 0x9271, 0, 0 },
		{ "eac_rg11.pkm", 0x9272, 0x9272, 0, 0 },
		{ "eac_rg11_signed.pkm", 0x9273, 0x9273, 0, 0 },
		{ "astc_4x4.astc", 0x93B0, 0x93B0, 1, 0 },
		{ "astc_6x6.astc", 0x93B4, 0x93B4, 1, 0 },
		{ "astc_12x12.astc", 0x93DD, 0x93DD, 1, SOIL_FLAG_SRGB_COLOR_SPACE }
	};
	const std::string fixture_dir = argc > 1 ? argv[1] : "bin/mobile";
	int success = 1;

	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError() );
		return 1;
	}
	SDL_Window* window = SDL_CreateWindow(
		"SOIL2 mobile compressed texture test", 0, 0, 16, 16,
		SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
	SDL_GLContext context = window ? SDL_GL_CreateContext( window ) : NULL;
	if( context == NULL )
	{
		fprintf( stderr, "OpenGL context creation failed: %s\n", SDL_GetError() );
		if( window )
			SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	for( size_t i = 0; i < sizeof( fixtures ) / sizeof( fixtures[0] ); ++i )
		success &= test_fixture( fixture_dir, fixtures[i] );

	std::vector<unsigned char> invalid = read_file( fixture_dir + "/etc2_rgb8.pkm" );
	invalid.pop_back();
	success &= expect_failure( invalid, 0, "payload size" );
	invalid = read_file( fixture_dir + "/etc2_rgb8.pkm" );
	invalid[7] = 99;
	success &= expect_failure( invalid, 0, "Unsupported PKM texture format" );
	invalid = read_file( fixture_dir + "/astc_4x4.astc" );
	invalid[4] = 7;
	success &= expect_failure( invalid, 1, "block footprint" );
	invalid = read_file( fixture_dir + "/astc_4x4.astc" );
	invalid.pop_back();
	success &= expect_failure( invalid, 1, "payload size" );
	invalid = read_file( fixture_dir + "/astc_4x4.astc" );
	invalid[7] = 0xFF;
	invalid[8] = 0xFF;
	invalid[9] = 0xFF;
	success &= expect_failure( invalid, 1, "payload size" );
	invalid = read_file( fixture_dir + "/etc2_rgb8.pkm" );
	invalid[12] = 0;
	invalid[13] = 0;
	success &= expect_failure( invalid, 0, "Invalid PKM dimensions" );

	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();
	if( success )
		printf( "Mobile compressed texture direct upload tests passed\n" );
	return success ? 0 : 1;
}
