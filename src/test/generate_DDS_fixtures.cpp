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
#ifndef GL_RED
#define GL_RED 0x1903
#endif
#ifndef GL_RG
#define GL_RG 0x8227
#endif
#ifndef GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#endif
#ifndef GL_COMPRESSED_RGBA_BPTC_UNORM
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D
#endif
#ifndef GL_COMPRESSED_RED_RGTC1
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#endif
#ifndef GL_COMPRESSED_SIGNED_RED_RGTC1
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#endif
#ifndef GL_COMPRESSED_RG_RGTC2
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#endif
#ifndef GL_COMPRESSED_SIGNED_RG_RGTC2
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT 0x8C4D
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT 0x8C4E
#endif
#ifndef GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT
#define GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT 0x8C4F
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

static DDS_header make_header(
	unsigned int bytes_per_pixel, unsigned int compressed_block_size, int cubemap,
	unsigned int row_pitch = 0 )
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
		bytes_per_pixel ? ( row_pitch ? row_pitch : FIXTURE_WIDTH * bytes_per_pixel ) :
		                  ( ( FIXTURE_WIDTH + 3 ) / 4 ) *
		                  ( ( FIXTURE_HEIGHT + 3 ) / 4 ) * compressed_block_size;
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
                      unsigned int bytes_per_pixel, unsigned int compressed_block_size,
                      const void* data, size_t data_size,
                      int cubemap, unsigned int row_pitch = 0 )
{
	FILE* output = fopen( path.c_str(), "wb" );
	if( NULL == output )
	{
		fprintf( stderr, "Could not open %s\n", path.c_str() );
		return 0;
	}

	const DDS_header header =
		make_header( bytes_per_pixel, compressed_block_size, cubemap, row_pitch );
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

static int write_legacy_dds(
	const std::string& path, uint32_t fourcc, unsigned int compressed_block_size,
	const void* data, size_t data_size )
{
	FILE* output = fopen( path.c_str(), "wb" );
	if( NULL == output )
	{
		fprintf( stderr, "Could not open %s\n", path.c_str() );
		return 0;
	}

	DDS_header header = make_header( 0, compressed_block_size, 0 );
	header.sPixelFormat.dwFourCC = fourcc;
	const int success =
		fwrite( &header, sizeof( header ), 1, output ) == 1 &&
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
		           DXGI_FORMAT_R16G16B16A16_UNORM, 8, 0,
		           rgba16_unorm.data(), rgba16_unorm.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_rgba16_float.dds",
		           DXGI_FORMAT_R16G16B16A16_FLOAT, 8, 0,
		           rgba16_float.data(), rgba16_float.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_rgba32_float.dds",
		           DXGI_FORMAT_R32G32B32A32_FLOAT, 16, 0,
		           source.data(), source.size() * sizeof( float ), 0 );
}

static uint32_t float_to_ufloat( float value, unsigned int mantissa_bits )
{
	const uint16_t half = float_to_half( std::max( value, 0.0f ) );
	unsigned int exponent = ( half >> 10 ) & 0x1f;
	unsigned int mantissa = half & 0x3ff;
	const unsigned int shift = 10 - mantissa_bits;

	if( exponent == 0x1f )
		return ( exponent << mantissa_bits ) |
		       ( mantissa ? ( 1u << ( mantissa_bits - 1 ) ) : 0 );

	mantissa = ( mantissa + ( 1u << ( shift - 1 ) ) ) >> shift;
	if( mantissa == ( 1u << mantissa_bits ) )
	{
		mantissa = 0;
		if( exponent < 0x1f )
			++exponent;
	}
	return ( exponent << mantissa_bits ) | mantissa;
}

static int write_uncompressed_dxgi_fixtures( const std::string& output_dir )
{
	const std::vector<float> source = make_procedural_rgba( 0 );
	const size_t pixel_count = FIXTURE_WIDTH * FIXTURE_HEIGHT;
	std::vector<uint8_t> rgba8( pixel_count * 4 );
	std::vector<uint8_t> bgra8( pixel_count * 4 );
	std::vector<uint8_t> r8_padded( ( FIXTURE_WIDTH + 4 ) * FIXTURE_HEIGHT, 0xcd );
	std::vector<int8_t> r8_snorm( pixel_count );
	std::vector<uint8_t> rg8( pixel_count * 2 );
	std::vector<int8_t> rg8_snorm( pixel_count * 2 );
	std::vector<uint16_t> r16( pixel_count );
	std::vector<uint16_t> rg16( pixel_count * 2 );
	std::vector<uint16_t> r16f( pixel_count );
	std::vector<uint16_t> rg16f( pixel_count * 2 );
	std::vector<float> r32f( pixel_count );
	std::vector<float> rg32f( pixel_count * 2 );
	std::vector<uint32_t> rgb10a2( pixel_count );
	std::vector<uint32_t> r11g11b10( pixel_count );

	for( size_t pixel = 0; pixel < pixel_count; ++pixel )
	{
		const float r = std::max( 0.0f, std::min( source[pixel * 4 + 0], 1.0f ) );
		const float g = std::max( 0.0f, std::min( source[pixel * 4 + 1], 1.0f ) );
		const float b = std::max( 0.0f, std::min( source[pixel * 4 + 2], 1.0f ) );
		const float a = std::max( 0.0f, std::min( source[pixel * 4 + 3], 1.0f ) );
		const uint8_t r_byte = (uint8_t)std::floor( r * 255.0f + 0.5f );
		const uint8_t g_byte = (uint8_t)std::floor( g * 255.0f + 0.5f );
		const uint8_t b_byte = (uint8_t)std::floor( b * 255.0f + 0.5f );
		const uint8_t a_byte = (uint8_t)std::floor( a * 255.0f + 0.5f );
		const size_t x = pixel % FIXTURE_WIDTH;
		const size_t y = pixel / FIXTURE_WIDTH;

		rgba8[pixel * 4 + 0] = r_byte;
		rgba8[pixel * 4 + 1] = g_byte;
		rgba8[pixel * 4 + 2] = b_byte;
		rgba8[pixel * 4 + 3] = a_byte;
		bgra8[pixel * 4 + 0] = b_byte;
		bgra8[pixel * 4 + 1] = g_byte;
		bgra8[pixel * 4 + 2] = r_byte;
		bgra8[pixel * 4 + 3] = a_byte;
		r8_padded[y * ( FIXTURE_WIDTH + 4 ) + x] = r_byte;
		r8_snorm[pixel] = (int8_t)std::floor( ( r * 2.0f - 1.0f ) * 127.0f );
		rg8[pixel * 2 + 0] = r_byte;
		rg8[pixel * 2 + 1] = g_byte;
		rg8_snorm[pixel * 2 + 0] =
			(int8_t)std::floor( ( r * 2.0f - 1.0f ) * 127.0f );
		rg8_snorm[pixel * 2 + 1] =
			(int8_t)std::floor( ( g * 2.0f - 1.0f ) * 127.0f );
		r16[pixel] = (uint16_t)std::floor( r * 65535.0f + 0.5f );
		rg16[pixel * 2 + 0] = (uint16_t)std::floor( r * 65535.0f + 0.5f );
		rg16[pixel * 2 + 1] = (uint16_t)std::floor( g * 65535.0f + 0.5f );
		r16f[pixel] = float_to_half( r );
		rg16f[pixel * 2 + 0] = float_to_half( r );
		rg16f[pixel * 2 + 1] = float_to_half( g );
		r32f[pixel] = r;
		rg32f[pixel * 2 + 0] = r;
		rg32f[pixel * 2 + 1] = g;
		rgb10a2[pixel] =
			(uint32_t)std::floor( r * 1023.0f + 0.5f ) |
			( (uint32_t)std::floor( g * 1023.0f + 0.5f ) << 10 ) |
			( (uint32_t)std::floor( b * 1023.0f + 0.5f ) << 20 ) |
			( (uint32_t)std::floor( a * 3.0f + 0.5f ) << 30 );
		r11g11b10[pixel] =
			float_to_ufloat( r, 6 ) |
			( float_to_ufloat( g, 6 ) << 11 ) |
			( float_to_ufloat( b, 5 ) << 22 );
	}

	return
		write_dds( output_dir + "/test_dx10_rgba8_unorm.dds",
			DXGI_FORMAT_R8G8B8A8_UNORM, 4, 0, rgba8.data(), rgba8.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_rgba8_srgb.dds",
			DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 4, 0, rgba8.data(), rgba8.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_bgra8_unorm.dds",
			DXGI_FORMAT_B8G8R8A8_UNORM, 4, 0, bgra8.data(), bgra8.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_bgra8_srgb.dds",
			DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, 4, 0, bgra8.data(), bgra8.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_r8_unorm_padded.dds",
			DXGI_FORMAT_R8_UNORM, 1, 0, r8_padded.data(), r8_padded.size(), 0,
			FIXTURE_WIDTH + 4 ) &&
		write_dds( output_dir + "/test_dx10_r8_snorm.dds",
			DXGI_FORMAT_R8_SNORM, 1, 0, r8_snorm.data(), r8_snorm.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_rg8_unorm.dds",
			DXGI_FORMAT_R8G8_UNORM, 2, 0, rg8.data(), rg8.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_rg8_snorm.dds",
			DXGI_FORMAT_R8G8_SNORM, 2, 0, rg8_snorm.data(), rg8_snorm.size(), 0 ) &&
		write_dds( output_dir + "/test_dx10_r16_unorm.dds",
			DXGI_FORMAT_R16_UNORM, 2, 0, r16.data(), r16.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_dx10_rg16_unorm.dds",
			DXGI_FORMAT_R16G16_UNORM, 4, 0, rg16.data(), rg16.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_dx10_r16_float.dds",
			DXGI_FORMAT_R16_FLOAT, 2, 0, r16f.data(), r16f.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_dx10_rg16_float.dds",
			DXGI_FORMAT_R16G16_FLOAT, 4, 0, rg16f.data(), rg16f.size() * sizeof( uint16_t ), 0 ) &&
		write_dds( output_dir + "/test_dx10_r32_float.dds",
			DXGI_FORMAT_R32_FLOAT, 4, 0, r32f.data(), r32f.size() * sizeof( float ), 0 ) &&
		write_dds( output_dir + "/test_dx10_rg32_float.dds",
			DXGI_FORMAT_R32G32_FLOAT, 8, 0, rg32f.data(), rg32f.size() * sizeof( float ), 0 ) &&
		write_dds( output_dir + "/test_dx10_rgb10a2_unorm.dds",
			DXGI_FORMAT_R10G10B10A2_UNORM, 4, 0,
			rgb10a2.data(), rgb10a2.size() * sizeof( uint32_t ), 0 ) &&
		write_dds( output_dir + "/test_dx10_r11g11b10_float.dds",
			DXGI_FORMAT_R11G11B10_FLOAT, 4, 0,
			r11g11b10.data(), r11g11b10.size() * sizeof( uint32_t ), 0 );
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

static int encode_compressed(
	GLenum internal_format, GLenum external_format, const std::vector<float>& rgba,
	std::vector<unsigned char>* blocks )
{
	const int channels = external_format == GL_RED ? 1 : external_format == GL_RG ? 2 :
		external_format == GL_RGB ? 3 : 4;
	std::vector<float> source( FIXTURE_WIDTH * FIXTURE_HEIGHT * channels );
	for( int pixel = 0; pixel < FIXTURE_WIDTH * FIXTURE_HEIGHT; ++pixel )
	{
		for( int channel = 0; channel < channels; ++channel )
			source[pixel * channels + channel] = rgba[pixel * 4 + channel];
	}

	GLuint texture = 0;
	glGenTextures( 1, &texture );
	glBindTexture( GL_TEXTURE_2D, texture );
	glTexImage2D( GL_TEXTURE_2D, 0, internal_format,
	              FIXTURE_WIDTH, FIXTURE_HEIGHT, 0, external_format, GL_FLOAT, source.data() );
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

struct CompressedFixture
{
	const char* name;
	DXGI_FORMAT dxgi_format;
	GLenum internal_format;
	GLenum external_format;
	unsigned int block_size;
	int signed_values;
	int hdr_values;
};

static std::vector<float> make_compressed_source( int signed_values, int hdr_values )
{
	std::vector<float> source = make_procedural_rgba( 0 );
	for( size_t i = 0; i < source.size(); i += 4 )
	{
		if( signed_values )
		{
			source[i + 0] = source[i + 0] * 2.0f - 1.0f;
			source[i + 1] = source[i + 1] * 2.0f - 1.0f;
		}
		if( hdr_values )
		{
			source[i + 0] *= 4.0f;
			source[i + 1] *= 4.0f;
			source[i + 2] *= 4.0f;
		}
	}
	return source;
}

static int write_common_compressed_fixtures( const std::string& output_dir )
{
	static const CompressedFixture fixtures[] = {
		{ "bc1_unorm", DXGI_FORMAT_BC1_UNORM, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_RGBA, 8, 0, 0 },
		{ "bc1_srgb", DXGI_FORMAT_BC1_UNORM_SRGB, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT, GL_RGBA, 8, 0, 0 },
		{ "bc2_unorm", DXGI_FORMAT_BC2_UNORM, GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_RGBA, 16, 0, 0 },
		{ "bc2_srgb", DXGI_FORMAT_BC2_UNORM_SRGB, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT, GL_RGBA, 16, 0, 0 },
		{ "bc3_unorm", DXGI_FORMAT_BC3_UNORM, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_RGBA, 16, 0, 0 },
		{ "bc3_srgb", DXGI_FORMAT_BC3_UNORM_SRGB, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT, GL_RGBA, 16, 0, 0 },
		{ "bc4_unorm", DXGI_FORMAT_BC4_UNORM, GL_COMPRESSED_RED_RGTC1, GL_RED, 8, 0, 0 },
		{ "bc4_snorm", DXGI_FORMAT_BC4_SNORM, GL_COMPRESSED_SIGNED_RED_RGTC1, GL_RED, 8, 1, 0 },
		{ "bc5_unorm", DXGI_FORMAT_BC5_UNORM, GL_COMPRESSED_RG_RGTC2, GL_RG, 16, 0, 0 },
		{ "bc5_snorm", DXGI_FORMAT_BC5_SNORM, GL_COMPRESSED_SIGNED_RG_RGTC2, GL_RG, 16, 1, 0 },
		{ "bc6h_uf16", DXGI_FORMAT_BC6H_UF16, GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, 16, 0, 1 },
		{ "bc6h_sf16", DXGI_FORMAT_BC6H_SF16, GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT, GL_RGB, 16, 1, 1 },
		{ "bc7_unorm", DXGI_FORMAT_BC7_UNORM, GL_COMPRESSED_RGBA_BPTC_UNORM, GL_RGBA, 16, 0, 0 },
		{ "bc7_srgb", DXGI_FORMAT_BC7_UNORM_SRGB, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM, GL_RGBA, 16, 0, 0 }
	};
	std::vector<std::vector<unsigned char> > encoded(
		sizeof( fixtures ) / sizeof( fixtures[0] ) );

	for( size_t i = 0; i < encoded.size(); ++i )
	{
		const std::vector<float> source =
			make_compressed_source( fixtures[i].signed_values, fixtures[i].hdr_values );
		if( !encode_compressed(
				fixtures[i].internal_format, fixtures[i].external_format, source, &encoded[i] ) )
		{
			fprintf( stderr, "OpenGL could not encode %s fixture data\n", fixtures[i].name );
			return 0;
		}
		if( !write_dds(
				output_dir + "/test_dx10_" + fixtures[i].name + ".dds",
				fixtures[i].dxgi_format, 0, fixtures[i].block_size,
				encoded[i].data(), encoded[i].size(), 0 ) )
			return 0;
	}

	enum
	{
		FOURCC_DXT2 = ( 'D' << 0 ) | ( 'X' << 8 ) | ( 'T' << 16 ) | ( '2' << 24 ),
		FOURCC_DXT4 = ( 'D' << 0 ) | ( 'X' << 8 ) | ( 'T' << 16 ) | ( '4' << 24 ),
		FOURCC_ATI1 = ( 'A' << 0 ) | ( 'T' << 8 ) | ( 'I' << 16 ) | ( '1' << 24 ),
		FOURCC_BC4U = ( 'B' << 0 ) | ( 'C' << 8 ) | ( '4' << 16 ) | ( 'U' << 24 ),
		FOURCC_BC4S = ( 'B' << 0 ) | ( 'C' << 8 ) | ( '4' << 16 ) | ( 'S' << 24 ),
		FOURCC_BC5S = ( 'B' << 0 ) | ( 'C' << 8 ) | ( '5' << 16 ) | ( 'S' << 24 )
	};

	return
		write_legacy_dds( output_dir + "/test_legacy_dxt2.dds", FOURCC_DXT2, 16,
			encoded[2].data(), encoded[2].size() ) &&
		write_legacy_dds( output_dir + "/test_legacy_dxt4.dds", FOURCC_DXT4, 16,
			encoded[4].data(), encoded[4].size() ) &&
		write_legacy_dds( output_dir + "/test_legacy_ati1.dds", FOURCC_ATI1, 8,
			encoded[6].data(), encoded[6].size() ) &&
		write_legacy_dds( output_dir + "/test_legacy_bc4u.dds", FOURCC_BC4U, 8,
			encoded[6].data(), encoded[6].size() ) &&
		write_legacy_dds( output_dir + "/test_legacy_bc4s.dds", FOURCC_BC4S, 8,
			encoded[7].data(), encoded[7].size() ) &&
		write_legacy_dds( output_dir + "/test_legacy_bc5s.dds", FOURCC_BC5S, 16,
			encoded[9].data(), encoded[9].size() );
}

static int write_bc6h_cubemap_fixture( const std::string& output_dir )
{
	std::vector<unsigned char> blocks;
	for( int face = 0; face < 6; ++face )
	{
		std::vector<float> source = make_procedural_rgba( face );
		for( size_t i = 0; i < source.size(); i += 4 )
		{
			source[i + 0] *= 4.0f;
			source[i + 1] *= 4.0f;
			source[i + 2] *= 4.0f;
		}
		if( !encode_compressed(
			GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT, GL_RGB, source, &blocks ) )
		{
			fprintf( stderr, "OpenGL could not encode BC6H cubemap fixture data\n" );
			return 0;
		}
	}

	return write_dds( output_dir + "/test_bc6h_uf16_cubemap.dds",
	                  DXGI_FORMAT_BC6H_UF16, 0, 16, blocks.data(), blocks.size(), 1 );
}

int main( int argc, char** argv )
{
	const std::string output_dir = argc > 1 ? argv[1] : ".";

	if( !write_high_precision_fixtures( output_dir ) )
		return 1;
	if( !write_uncompressed_dxgi_fixtures( output_dir ) )
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

	const int success =
		write_common_compressed_fixtures( output_dir ) &&
		write_bc6h_cubemap_fixture( output_dir );
	SDL_GL_DeleteContext( context );
	SDL_DestroyWindow( window );
	SDL_Quit();

	if( success )
		printf( "Generated procedural DDS and HDR fixtures in %s\n", output_dir.c_str() );
	return success ? 0 : 1;
}
