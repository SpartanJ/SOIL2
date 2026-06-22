#include <cmath>
#include <cstdlib>
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
	int cpu_channels;
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

static int compare_cpu_decode(
	const std::string& path, GLuint texture, const Fixture& fixture )
{
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* cpu = SOIL_load_image(
		path.c_str(), &width, &height, &channels, SOIL_LOAD_AUTO );
	if( cpu == NULL )
	{
		fprintf( stderr, "%s CPU decode: %s\n", path.c_str(), SOIL_last_result() );
		return 0;
	}
	if( width != 13 || height != 9 || channels != fixture.cpu_channels )
	{
		fprintf(
			stderr, "%s CPU decode returned channels=%d size=%dx%d\n",
			path.c_str(), channels, width, height );
		free( cpu );
		return 0;
	}

	const std::vector<unsigned char> encoded = read_file( path );
	int memory_width = 0;
	int memory_height = 0;
	int memory_channels = 0;
	unsigned char* memory_cpu = SOIL_load_image_from_memory(
		encoded.data(), (int)encoded.size(), &memory_width, &memory_height,
		&memory_channels, SOIL_LOAD_AUTO );
	if( memory_cpu == NULL || memory_width != width || memory_height != height ||
	    memory_channels != channels ||
	    memcmp(
			memory_cpu, cpu, (size_t)width * height * channels ) != 0 )
	{
		fprintf( stderr, "%s memory CPU decode differed from file decode\n", path.c_str() );
		free( memory_cpu );
		free( cpu );
		return 0;
	}
	free( memory_cpu );

	std::vector<float> gpu( (size_t)width * (size_t)height * 4 );
	glBindTexture( GL_TEXTURE_2D, texture );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, gpu.data() );
	if( glGetError() != GL_NO_ERROR )
	{
		fprintf( stderr, "%s GPU readback failed\n", path.c_str() );
		free( cpu );
		return 0;
	}

	const bool signed_eac = path.find( "_signed" ) != std::string::npos;
	const bool eac_r = path.find( "eac_r11" ) != std::string::npos;
	const bool eac_rg = path.find( "eac_rg11" ) != std::string::npos;
	int success = 1;
	for( int y = 0; y < height && success; ++y )
	{
		for( int x = 0; x < width && success; ++x )
		{
			const size_t pixel = (size_t)y * width + x;
			if( eac_r || eac_rg )
			{
				const int color_channels = eac_rg ? 2 : 1;
				for( int channel = 0; channel < color_channels; ++channel )
				{
					const float normalized = signed_eac ?
						gpu[pixel * 4 + channel] * 0.5f + 0.5f :
						gpu[pixel * 4 + channel];
					const int expected =
						(int)std::floor( normalized * 255.0f + 0.5f );
					if( std::abs(
							expected - (int)cpu[pixel * channels + channel] ) > 1 )
						success = 0;
				}
				if( eac_rg && cpu[pixel * channels + 2] != 0 )
					success = 0;
			}
			else
			{
				for( int channel = 0; channel < channels; ++channel )
				{
					const int expected =
						(int)std::floor( gpu[pixel * 4 + channel] * 255.0f + 0.5f );
					if( std::abs(
							expected - (int)cpu[pixel * channels + channel] ) > 1 )
						success = 0;
				}
			}
		}
	}
	if( !success )
		fprintf( stderr, "%s CPU decode did not match GPU decompression\n", path.c_str() );
	free( cpu );
	return success;
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
	if( !fixture.astc )
		success &= compare_cpu_decode( path, texture, fixture );
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

static int expect_cpu_failure( const std::vector<unsigned char>& data )
{
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char* pixels = SOIL_load_image_from_memory(
		data.data(), (int)data.size(), &width, &height, &channels,
		SOIL_LOAD_AUTO );
	if( pixels != NULL )
	{
		fprintf( stderr, "Malformed PKM unexpectedly passed CPU decoding\n" );
		free( pixels );
		return 0;
	}
	return 1;
}

int main( int argc, char** argv )
{
	static const Fixture fixtures[] = {
		{ "etc1_rgb8.pkm", 0x8D64, 0x9274, 0, 0, 3 },
		{ "etc2_rgb8.pkm", 0x9274, 0x9274, 0, 0, 3 },
		{ "etc2_rgba8_old.pkm", 0x9278, 0x9278, 0, 0, 4 },
		{ "etc2_rgba8.pkm", 0x9278, 0x9278, 0, 0, 4 },
		{ "etc2_rgb8a1.pkm", 0x9276, 0x9276, 0, 0, 4 },
		{ "eac_r11.pkm", 0x9270, 0x9270, 0, 0, 1 },
		{ "eac_r11_signed.pkm", 0x9271, 0x9271, 0, 0, 1 },
		{ "eac_rg11.pkm", 0x9272, 0x9272, 0, 0, 3 },
		{ "eac_rg11_signed.pkm", 0x9273, 0x9273, 0, 0, 3 },
		{ "astc_4x4.astc", 0x93B0, 0x93B0, 1, 0, 0 },
		{ "astc_6x6.astc", 0x93B4, 0x93B4, 1, 0, 0 },
		{ "astc_12x12.astc", 0x93DD, 0x93DD, 1, SOIL_FLAG_SRGB_COLOR_SPACE, 0 }
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
	success &= expect_cpu_failure( invalid );
	invalid = read_file( fixture_dir + "/etc2_rgb8.pkm" );
	invalid[7] = 99;
	success &= expect_failure( invalid, 0, "Unsupported PKM texture format" );
	success &= expect_cpu_failure( invalid );
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
	success &= expect_cpu_failure( invalid );

	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();
	if( success )
		printf( "Mobile compressed texture tests passed\n" );
	return success ? 0 : 1;
}
