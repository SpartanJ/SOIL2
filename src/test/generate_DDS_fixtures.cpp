#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../SOIL2/image_DXT.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../SOIL2/stb_image_write.h"

#define NO_SDL_GLEXT
#if ( ( defined( _MSCVER ) || defined( _MSC_VER ) ) || defined( __APPLE_CC__ ) || defined ( __APPLE__ ) ) && !defined( SOIL2_NO_FRAMEWORKS )
	#include <SDL.h>
	#include <SDL_opengl.h>
#else
	#include <SDL2/SDL.h>
	#include <SDL2/SDL_opengl.h>
#endif

#ifndef GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#endif
#ifndef GL_TEXTURE_COMPRESSED_IMAGE_SIZE
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE 0x86A0
#endif
#ifndef GL_TEXTURE_COMPRESSED
#define GL_TEXTURE_COMPRESSED 0x86A1
#endif

typedef void (APIENTRY *P_GLGETCOMPRESSEDTEXIMAGEPROC)( GLenum target, GLint level, void* image );
static P_GLGETCOMPRESSEDTEXIMAGEPROC soilGlGetCompressedTexImage = NULL;

enum
{
	DDS_FOURCC_DX10 = ( 'D' << 0 ) | ( 'X' << 8 ) | ( '1' << 16 ) | ( '0' << 24 )
};

static const int FIXTURE_WIDTH = 16;
static const int FIXTURE_HEIGHT = 16;

static uint16_t float_to_half( float value )
{
	uint32_t bits;
	memcpy( &bits, &value, sizeof( bits ) );

	const uint32_t sign = ( bits >> 16 ) & 0x8000;
	int exponent = (int)( ( bits >> 23 ) & 0xff ) - 127 + 15;
	uint32_t mantissa = bits & 0x7fffff;

	if( exponent <= 0 )
	{
		if( exponent < -10 )
			return (uint16_t)sign;
		mantissa = ( mantissa | 0x800000 ) >> ( 1 - exponent );
		return (uint16_t)( sign | ( ( mantissa + 0x1000 ) >> 13 ) );
	}
	if( exponent >= 31 )
		return (uint16_t)( sign | 0x7c00 );

	return (uint16_t)( sign | ( (uint32_t)exponent << 10 ) | ( ( mantissa + 0x1000 ) >> 13 ) );
}

static std::vector<float> make_procedural_rgba( int face )
{
	static const float face_colors[6][3] = {
		{ 1.0f, 0.2f, 0.1f },
		{ 0.1f, 1.0f, 0.2f },
		{ 0.2f, 0.1f, 1.0f },
		{ 1.0f, 0.8f, 0.1f },
		{ 0.8f, 0.1f, 1.0f },
		{ 0.1f, 0.8f, 1.0f }
	};

	std::vector<float> pixels( FIXTURE_WIDTH * FIXTURE_HEIGHT * 4 );
	for( int y = 0; y < FIXTURE_HEIGHT; ++y )
	{
		for( int x = 0; x < FIXTURE_WIDTH; ++x )
		{
			const float u = (float)x / (float)( FIXTURE_WIDTH - 1 );
			const float v = (float)y / (float)( FIXTURE_HEIGHT - 1 );
			const float checker = ( ( x / 4 + y / 4 ) & 1 ) ? 0.25f : 1.0f;
			const size_t offset = ( (size_t)y * FIXTURE_WIDTH + x ) * 4;
			pixels[offset + 0] = face_colors[face][0] * u * checker;
			pixels[offset + 1] = face_colors[face][1] * v * checker;
			pixels[offset + 2] = face_colors[face][2] * ( 1.0f - u * v ) * checker;
			pixels[offset + 3] = 0.25f + 0.75f * u;
		}
	}
	return pixels;
}

static DDS_header make_header( unsigned int bytes_per_pixel, int cubemap )
{
	DDS_header header;
	memset( &header, 0, sizeof( header ) );
	header.dwMagic = 0x20534444;
	header.dwSize = 124;
	header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT |
	                 ( bytes_per_pixel ? DDSD_PITCH : DDSD_LINEARSIZE );
	header.dwHeight = FIXTURE_HEIGHT;
	header.dwWidth = FIXTURE_WIDTH;
	header.dwPitchOrLinearSize =
		bytes_per_pixel ? FIXTURE_WIDTH * bytes_per_pixel :
		                  ( ( FIXTURE_WIDTH + 3 ) / 4 ) * 16;
	header.sPixelFormat.dwSize = 32;
	header.sPixelFormat.dwFlags = DDPF_FOURCC;
	header.sPixelFormat.dwFourCC = DDS_FOURCC_DX10;
	header.sCaps.dwCaps1 = DDSCAPS_TEXTURE;

	if( cubemap )
	{
		header.sCaps.dwCaps1 |= DDSCAPS_COMPLEX;
		header.sCaps.dwCaps2 = DDSCAPS2_CUBEMAP |
			DDSCAPS2_CUBEMAP_POSITIVEX | DDSCAPS2_CUBEMAP_NEGATIVEX |
			DDSCAPS2_CUBEMAP_POSITIVEY | DDSCAPS2_CUBEMAP_NEGATIVEY |
			DDSCAPS2_CUBEMAP_POSITIVEZ | DDSCAPS2_CUBEMAP_NEGATIVEZ;
	}

	return header;
}

static DDS_HEADER_DXT10 make_dx10_header( DXGI_FORMAT format, int cubemap )
{
	DDS_HEADER_DXT10 header;
	memset( &header, 0, sizeof( header ) );
	header.dxgiFormat = format;
	header.resourceDimension = DDS_DIMENSION_TEXTURE2D;
	header.miscFlag = cubemap ? DDS_RESOURCE_MISC_TEXTURECUBE : 0;
	header.arraySize = 1;
	return header;
}

static int write_dds( const std::string& path, DXGI_FORMAT format,
                      unsigned int bytes_per_pixel, const void* data, size_t data_size,
                      int cubemap )
{
	FILE* output = fopen( path.c_str(), "wb" );
	if( NULL == output )
	{
		fprintf( stderr, "Could not open %s\n", path.c_str() );
		return 0;
	}

	const DDS_header header = make_header( bytes_per_pixel, cubemap );
	const DDS_HEADER_DXT10 dx10_header = make_dx10_header( format, cubemap );
	const int success =
		fwrite( &header, sizeof( header ), 1, output ) == 1 &&
		fwrite( &dx10_header, sizeof( dx10_header ), 1, output ) == 1 &&
		fwrite( data, data_size, 1, output ) == 1;
	fclose( output );

	if( !success )
		fprintf( stderr, "Could not write %s\n", path.c_str() );
	return success;
}

static int write_high_precision_fixtures( const std::string& output_dir )
{
	const std::vector<float> source = make_procedural_rgba( 0 );
	std::vector<uint16_t> rgba16_unorm( source.size() );
	std::vector<uint16_t> rgba16_float( source.size() );

	for( size_t i = 0; i < source.size(); ++i )
	{
		const float normalized = std::max( 0.0f, std::min( source[i], 1.0f ) );
		rgba16_unorm[i] = (uint16_t)std::floor( normalized * 65535.0f + 0.5f );
		rgba16_float[i] = float_to_half( source[i] );
	}

	return
		write_dds( output_dir + "/test_rgba16_unorm.dds",
		           DXGI_FORMAT_R16G16B16A16_UNORM, 8,
		           rgba16_unorm.data(), rgba16_unorm.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_rgba16_float.dds",
		           DXGI_FORMAT_R16G16B16A16_FLOAT, 8,
		           rgba16_float.data(), rgba16_float.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_rgba32_float.dds",
		           DXGI_FORMAT_R32G32B32A32_FLOAT, 16,
		           source.data(), source.size() * sizeof( float ), 0 );
}

static int write_hdr_fixture( const std::string& output_dir )
{
	std::vector<float> rgb( FIXTURE_WIDTH * FIXTURE_HEIGHT * 3 );
	for( int y = 0; y < FIXTURE_HEIGHT; ++y )
	{
		for( int x = 0; x < FIXTURE_WIDTH; ++x )
		{
			const float u = (float)x / (float)( FIXTURE_WIDTH - 1 );
			const float v = (float)y / (float)( FIXTURE_HEIGHT - 1 );
			const float checker = ( ( x / 4 + y / 4 ) & 1 ) ? 0.5f : 1.0f;
			const size_t offset = ( (size_t)y * FIXTURE_WIDTH + x ) * 3;
			rgb[offset + 0] = ( 0.25f + u * 7.75f ) * checker;
			rgb[offset + 1] = ( 0.125f + v * 3.875f ) * checker;
			rgb[offset + 2] = ( 0.5f + ( 1.0f - u * v ) * 1.5f ) * checker;
		}
	}

	const std::string path = output_dir + "/test_native_hdr.hdr";
	if( !stbi_write_hdr( path.c_str(), FIXTURE_WIDTH, FIXTURE_HEIGHT, 3, rgb.data() ) )
	{
		fprintf( stderr, "Could not write %s\n", path.c_str() );
		return 0;
	}
	return 1;
}

static int encode_bc6h_face( const std::vector<float>& rgba, std::vector<unsigned char>* blocks )
{
	std::vector<float> rgb( FIXTURE_WIDTH * FIXTURE_HEIGHT * 3 );
	for( int i = 0; i < FIXTURE_WIDTH * FIXTURE_HEIGHT; ++i )
	{
		rgb[i * 3 + 0] = rgba[i * 4 + 0] * 4.0f;
		rgb[i * 3 + 1] = rgba[i * 4 + 1] * 4.0f;
		rgb[i * 3 + 2] = rgba[i * 4 + 2] * 4.0f;
	}

	GLuint texture = 0;
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
	              FIXTURE_WIDTH, FIXTURE_HEIGHT, 0, GL_RGB, GL_FLOAT, rgb.data() );
	if( glGetError() != GL_NO_ERROR )
	{
		glDeleteTextures( 1, &texture );
		return 0;
	}

	GLint compressed = 0;
	GLint compressed_size = 0;
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED, &compressed );
	glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressed_size );
	if( compressed != GL_TRUE || compressed_size <= 0 )
	{
		glDeleteTextures( 1, &texture );
		return 0;
	}

	const size_t old_size = blocks->size();
	blocks->resize( old_size + compressed_size );
	soilGlGetCompressedTexImage( GL_TEXTURE_2D, 0, blocks->data() + old_size );
	const int success = glGetError() == GL_NO_ERROR;
	glDeleteTextures( 1, &texture );
	return success;
}

static int write_bc6h_fixture( const std::string& output_dir )
{
	std::vector<unsigned char> blocks;
	for( int face = 0; face < 6; ++face )
	{
		if( !encode_bc6h_face( make_procedural_rgba( face ), &blocks ) )
		{
			fprintf( stderr, "OpenGL could not encode BC6H fixture data\n" );
			return 0;
		}
	}

	return write_dds( output_dir + "/test_bc6h_uf16_cubemap.dds",
	                  DXGI_FORMAT_BC6H_UF16, 0, blocks.data(), blocks.size(), 1 );
}

int main( int argc, char** argv )
{
	const std::string output_dir = argc > 1 ? argv[1] : ".";

	if( !write_high_precision_fixtures( output_dir ) )
		return 1;
	if( !write_hdr_fixture( output_dir ) )
		return 1;

	if( SDL_Init( SDL_INIT_VIDEO ) != 0 )
	{
		fprintf( stderr, "SDL initialization failed: %s\n", SDL_GetError() );
		return 1;
	}

	SDL_Window* window = SDL_CreateWindow(
		"DDS fixture generator", 0, 0, 16, 16, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN );
	SDL_GLContext context = window ? SDL_GL_CreateContext( window ) : NULL;
	if( NULL == context )
	{
		fprintf( stderr, "OpenGL context creation failed: %s\n", SDL_GetError() );
		if( window )
			SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	soilGlGetCompressedTexImage = (P_GLGETCOMPRESSEDTEXIMAGEPROC)
		SDL_GL_GetProcAddress( "glGetCompressedTexImage" );
	if( NULL == soilGlGetCompressedTexImage )
	{
		fprintf( stderr, "glGetCompressedTexImage is not available\n" );
		SDL_GL_DeleteContext( context );
		SDL_DestroyWindow( window );
		SDL_Quit();
		return 1;
	}

	const int success = write_bc6h_fixture( output_dir );
	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();

	if( success )
		printf( "Generated procedural DDS and HDR fixtures in %s\n", output_dir.c_str() );
	return success ? 0 : 1;
}
