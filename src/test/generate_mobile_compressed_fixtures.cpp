#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined( _WIN32 )
	#include <direct.h>
#else
	#include <sys/stat.h>
#endif

static void create_directory( const std::string& path )
{
#if defined( _WIN32 )
	_mkdir( path.c_str() );
#else
	mkdir( path.c_str(), 0777 );
#endif
}

static void write_be16( unsigned char* output, unsigned int value )
{
	output[0] = (unsigned char)( value >> 8 );
	output[1] = (unsigned char)value;
}

static void write_le24( unsigned char* output, unsigned int value )
{
	output[0] = (unsigned char)value;
	output[1] = (unsigned char)( value >> 8 );
	output[2] = (unsigned char)( value >> 16 );
}

static int write_file( const std::string& path, const std::vector<unsigned char>& data )
{
	FILE* output = fopen( path.c_str(), "wb" );
	if( NULL == output )
	{
		fprintf( stderr, "Could not open %s\n", path.c_str() );
		return 0;
	}
	const int success = fwrite( data.data(), data.size(), 1, output ) == 1;
	fclose( output );
	if( !success )
		fprintf( stderr, "Could not write %s\n", path.c_str() );
	return success;
}

static int write_pkm(
	const std::string& path, const char* version, unsigned int format,
	unsigned int width, unsigned int height, unsigned int block_size )
{
	const unsigned int encoded_width = ( width + 3 ) & ~3u;
	const unsigned int encoded_height = ( height + 3 ) & ~3u;
	const size_t payload_size =
		(size_t)( encoded_width / 4 ) * ( encoded_height / 4 ) * block_size;
	std::vector<unsigned char> data( 16 + payload_size );
	memcpy( data.data(), version, 6 );
	write_be16( data.data() + 6, format );
	write_be16( data.data() + 8, encoded_width );
	write_be16( data.data() + 10, encoded_height );
	write_be16( data.data() + 12, width );
	write_be16( data.data() + 14, height );

	for( size_t i = 16; i < data.size(); ++i )
		data[i] = format == 0 ? 0 :
			(unsigned char)( ( i * 37 + format * 19 ) & 0xff );

	/* Exercise every ETC2 RGB mode with deterministic valid mode selectors. */
	if( format >= 1 && format <= 4 )
	{
		const size_t color_offset = ( format == 2 || format == 3 ) ? 8 : 0;
		const size_t stride = block_size;
		for( int mode = 0; mode < 5; ++mode )
		{
			unsigned char* block = data.data() + 16 + (size_t)mode * stride + color_offset;
			block[0] = 0x80;
			block[1] = 0x80;
			block[2] = 0x80;
			if( mode == 0 && format != 4 )
				block[3] &= (unsigned char)~2;
			else
			{
				block[3] |= 2;
				if( mode == 2 )
					block[0] = 0x04;
				else if( mode == 3 )
					block[1] = 0x04;
				else if( mode == 4 )
					block[2] = 0x04;
			}
		}
	}
	return write_file( path, data );
}

static int write_astc(
	const std::string& path, unsigned int block_x, unsigned int block_y,
	unsigned int width, unsigned int height )
{
	const size_t blocks_x = ( width + block_x - 1 ) / block_x;
	const size_t blocks_y = ( height + block_y - 1 ) / block_y;
	std::vector<unsigned char> data( 16 + blocks_x * blocks_y * 16 );
	data[0] = 0x13;
	data[1] = 0xAB;
	data[2] = 0xA1;
	data[3] = 0x5C;
	data[4] = (unsigned char)block_x;
	data[5] = (unsigned char)block_y;
	data[6] = 1;
	write_le24( data.data() + 7, width );
	write_le24( data.data() + 10, height );
	write_le24( data.data() + 13, 1 );

	/* Void-extent blocks encode a deterministic opaque color without an encoder. */
	for( size_t offset = 16; offset < data.size(); offset += 16 )
	{
		data[offset + 0] = 0xFC;
		data[offset + 1] = 0x01;
		data[offset + 8] = 0x00;
		data[offset + 9] = 0x80;
		data[offset + 10] = 0x00;
		data[offset + 11] = 0x40;
		data[offset + 12] = 0x00;
		data[offset + 13] = 0x20;
		data[offset + 14] = 0xFF;
		data[offset + 15] = 0xFF;
	}
	return write_file( path, data );
}

int main( int argc, char** argv )
{
	const std::string output_dir = argc > 1 ? argv[1] : "bin/mobile";
	int success = 1;
	create_directory( output_dir );
	success &= write_pkm( output_dir + "/etc1_rgb8.pkm", "PKM 10", 0, 13, 9, 8 );
	success &= write_pkm( output_dir + "/etc2_rgb8.pkm", "PKM 20", 1, 13, 9, 8 );
	success &= write_pkm( output_dir + "/etc2_rgba8_old.pkm", "PKM 20", 2, 13, 9, 16 );
	success &= write_pkm( output_dir + "/etc2_rgba8.pkm", "PKM 20", 3, 13, 9, 16 );
	success &= write_pkm( output_dir + "/etc2_rgb8a1.pkm", "PKM 20", 4, 13, 9, 8 );
	success &= write_pkm( output_dir + "/eac_r11.pkm", "PKM 20", 5, 13, 9, 8 );
	success &= write_pkm( output_dir + "/eac_rg11.pkm", "PKM 20", 6, 13, 9, 16 );
	success &= write_pkm( output_dir + "/eac_r11_signed.pkm", "PKM 20", 7, 13, 9, 8 );
	success &= write_pkm( output_dir + "/eac_rg11_signed.pkm", "PKM 20", 8, 13, 9, 16 );
	success &= write_astc( output_dir + "/astc_4x4.astc", 4, 4, 13, 9 );
	success &= write_astc( output_dir + "/astc_6x6.astc", 6, 6, 13, 9 );
	success &= write_astc( output_dir + "/astc_12x12.astc", 12, 12, 13, 9 );
	return success ? 0 : 1;
}
