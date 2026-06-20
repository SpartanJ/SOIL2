#include <algorithm>
#include <cmath>
#include <cstdio>
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

struct DDSFixture
{
	const char* filename;
	GLint internal_format;
	int compressed;
};

static int check_bound_texture(
	GLenum target, GLint expected_internal_format, int expected_compressed,
	const char* filename )
{
	GLint internal_format = 0;
	GLint compressed = 0;
	GLint width = 0;
	GLint height = 0;
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format );
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_COMPRESSED, &compressed );
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_WIDTH, &width );
	glGetTexLevelParameteriv( target, 0, GL_TEXTURE_HEIGHT, &height );
	if( glGetError() != GL_NO_ERROR || internal_format != expected_internal_format ||
		compressed != expected_compressed || width != 16 || height != 16 )
	{
		fprintf(
			stderr,
			"%s: unexpected texture format=%d compressed=%d size=%dx%d\n",
			filename, internal_format, compressed, width, height );
		return 0;
	}

	std::vector<float> pixels( (size_t)width * (size_t)height * 4 );
	glGetTexImage( target, 0, GL_RGBA, GL_FLOAT, pixels.data() );
	if( glGetError() != GL_NO_ERROR )
	{
		fprintf( stderr, "%s: could not read decompressed texture data\n", filename );
		return 0;
	}
	for( size_t i = 0; i < pixels.size(); ++i )
	{
		if( !std::isfinite( pixels[i] ) )
		{
			fprintf( stderr, "%s: decompressed texture contains non-finite values\n", filename );
			return 0;
		}
	}
	const std::pair<std::vector<float>::iterator, std::vector<float>::iterator> range =
		std::minmax_element( pixels.begin(), pixels.end() );
	if( *range.second - *range.first < 0.01f )
	{
		fprintf( stderr, "%s: decompressed texture has no meaningful variation\n", filename );
		return 0;
	}
	if( std::string( filename ).find( "r8_unorm_padded" ) != std::string::npos &&
	    std::fabs( pixels[(size_t)width * 4] ) > 0.01f )
	{
		fprintf( stderr, "%s: padded DDS rows were not unpacked correctly\n", filename );
		return 0;
	}
	if( ( std::string( filename ).find( "rgba8_unorm" ) != std::string::npos ||
	      std::string( filename ).find( "bgra8_unorm" ) != std::string::npos ||
	      std::string( filename ).find( "rgb10a2_unorm" ) != std::string::npos ||
	      std::string( filename ).find( "r11g11b10_float" ) != std::string::npos ) )
	{
		const size_t offset = 15 * 4;
		if( std::fabs( pixels[offset + 0] - 0.25f ) > 0.02f ||
		    std::fabs( pixels[offset + 1] ) > 0.02f ||
		    std::fabs( pixels[offset + 2] - 0.025f ) > 0.02f )
		{
			fprintf( stderr, "%s: representative RGB values were incorrect\n", filename );
			return 0;
		}
	}
	return 1;
}

static int test_file_fixture(
	const std::string& path, GLint expected_internal_format, int expected_compressed )
{
	const GLuint texture = SOIL_load_OGL_texture(
		path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_DDS_LOAD_DIRECT );
	if( texture == 0 )
	{
		fprintf( stderr, "%s: %s\n", path.c_str(), SOIL_last_result() );
		return 0;
	}

	glBindTexture( GL_TEXTURE_2D, texture );
	const int success = check_bound_texture(
		GL_TEXTURE_2D, expected_internal_format, expected_compressed, path.c_str() );
	glDeleteTextures( 1, &texture );
	return success;
}

static int test_memory_fixture(
	const std::string& path, GLint expected_internal_format, int expected_compressed )
{
	std::ifstream input( path.c_str(), std::ios::binary );
	if( !input )
	{
		fprintf( stderr, "Could not read %s\n", path.c_str() );
		return 0;
	}
	const std::vector<unsigned char> data{
		std::istreambuf_iterator<char>( input ),
		std::istreambuf_iterator<char>() };
	const GLuint texture = SOIL_load_OGL_texture_from_memory(
		data.data(), (int)data.size(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
		SOIL_FLAG_DDS_LOAD_DIRECT );
	if( texture == 0 )
	{
		fprintf( stderr, "%s from memory: %s\n", path.c_str(), SOIL_last_result() );
		return 0;
	}

	glBindTexture( GL_TEXTURE_2D, texture );
	const int success = check_bound_texture(
		GL_TEXTURE_2D, expected_internal_format, expected_compressed, path.c_str() );
	glDeleteTextures( 1, &texture );
	return success;
}

int main( int argc, char** argv )
{
	static const DDSFixture fixtures[] = {
		{ "test_dx10_rgba8_unorm.dds", 0x8058, 0 },
		{ "test_dx10_rgba8_srgb.dds", 0x8C43, 0 },
		{ "test_dx10_bgra8_unorm.dds", 0x8058, 0 },
		{ "test_dx10_bgra8_srgb.dds", 0x8C43, 0 },
		{ "test_dx10_r8_unorm_padded.dds", 0x8229, 0 },
		{ "test_dx10_r8_snorm.dds", 0x8F94, 0 },
		{ "test_dx10_rg8_unorm.dds", 0x822B, 0 },
		{ "test_dx10_rg8_snorm.dds", 0x8F95, 0 },
		{ "test_dx10_r16_unorm.dds", 0x822A, 0 },
		{ "test_dx10_rg16_unorm.dds", 0x822C, 0 },
		{ "test_dx10_r16_float.dds", 0x822D, 0 },
		{ "test_dx10_rg16_float.dds", 0x822F, 0 },
		{ "test_dx10_r32_float.dds", 0x822E, 0 },
		{ "test_dx10_rg32_float.dds", 0x8230, 0 },
		{ "test_dx10_rgb10a2_unorm.dds", 0x8059, 0 },
		{ "test_dx10_r11g11b10_float.dds", 0x8C3A, 0 },
		{ "test_dx10_bc1_unorm.dds", 0x83F1, 1 },
		{ "test_dx10_bc1_srgb.dds", 0x8C4D, 1 },
		{ "test_dx10_bc2_unorm.dds", 0x83F2, 1 },
		{ "test_dx10_bc2_srgb.dds", 0x8C4E, 1 },
		{ "test_dx10_bc3_unorm.dds", 0x83F3, 1 },
		{ "test_dx10_bc3_srgb.dds", 0x8C4F, 1 },
		{ "test_dx10_bc4_unorm.dds", 0x8DBB, 1 },
		{ "test_dx10_bc4_snorm.dds", 0x8DBC, 1 },
		{ "test_dx10_bc5_unorm.dds", 0x8DBD, 1 },
		{ "test_dx10_bc5_snorm.dds", 0x8DBE, 1 },
		{ "test_dx10_bc6h_uf16.dds", 0x8E8F, 1 },
		{ "test_dx10_bc6h_sf16.dds", 0x8E8E, 1 },
		{ "test_dx10_bc7_unorm.dds", 0x8E8C, 1 },
		{ "test_dx10_bc7_srgb.dds", 0x8E8D, 1 },
		{ "test_legacy_dxt2.dds", 0x83F2, 1 },
		{ "test_legacy_dxt4.dds", 0x83F3, 1 },
		{ "test_legacy_ati1.dds", 0x8DBB, 1 },
		{ "test_legacy_bc4u.dds", 0x8DBB, 1 },
		{ "test_legacy_bc4s.dds", 0x8DBC, 1 },
		{ "test_legacy_bc5s.dds", 0x8DBE, 1 }
	};
	const std::string fixture_dir = argc > 1 ? argv[1] : "bin";
	int success = 1;

	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError() );
		return 1;
	}
	SDL_Window* window = SDL_CreateWindow(
		"SOIL2 DDS test", 0, 0, 16, 16, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
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
	{
		const std::string path = fixture_dir + "/" + fixtures[i].filename;
		success &= test_file_fixture(
			path, fixtures[i].internal_format, fixtures[i].compressed );
	}
	success &= test_memory_fixture(
		fixture_dir + "/test_dx10_bc7_unorm.dds", 0x8E8C, 1 );
	success &= test_memory_fixture(
		fixture_dir + "/test_dx10_r8_unorm_padded.dds", 0x8229, 0 );

	const std::string cubemap_path = fixture_dir + "/test_bc6h_uf16_cubemap.dds";
	const GLuint cubemap = SOIL_load_OGL_single_cubemap(
		cubemap_path.c_str(), SOIL_DDS_CUBEMAP_FACE_ORDER, SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_DDS_LOAD_DIRECT );
	if( cubemap == 0 )
	{
		fprintf( stderr, "%s: %s\n", cubemap_path.c_str(), SOIL_last_result() );
		success = 0;
	}
	else
	{
		glBindTexture( GL_TEXTURE_CUBE_MAP, cubemap );
		success &= check_bound_texture(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0x8E8F, 1, cubemap_path.c_str() );
		glDeleteTextures( 1, &cubemap );
	}

	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();

	if( success )
		printf( "DDS direct upload tests passed\n" );
	return success ? 0 : 1;
}
