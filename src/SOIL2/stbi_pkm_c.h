#include "pkm_helper.h"
#include "wfETC.h"

typedef struct
{
	int format;
	int width;
	int height;
	int encoded_width;
	int encoded_height;
	int channels;
	int block_size;
} stbi__pkm_header;

static int stbi__pkm_read_be16( const stbi_uc* data )
{
	return ( data[0] << 8 ) | data[1];
}

static int stbi__pkm_parse_header(
	const stbi_uc* data, stbi__pkm_header* parsed )
{
	const int pkm_format = stbi__pkm_read_be16( data + 6 );
	const int is_pkm_10 = memcmp( data, "PKM 10", 6 ) == 0;
	const int is_pkm_20 = memcmp( data, "PKM 20", 6 ) == 0;

	if( !is_pkm_10 && !is_pkm_20 )
		return 0;
	if( is_pkm_10 && pkm_format != PKM_FORMAT_ETC1_RGB8 )
		return 0;

	parsed->encoded_width = stbi__pkm_read_be16( data + 8 );
	parsed->encoded_height = stbi__pkm_read_be16( data + 10 );
	parsed->width = stbi__pkm_read_be16( data + 12 );
	parsed->height = stbi__pkm_read_be16( data + 14 );
	if( parsed->width <= 0 || parsed->height <= 0 ||
	    parsed->encoded_width != ( ( parsed->width + 3 ) & ~3 ) ||
	    parsed->encoded_height != ( ( parsed->height + 3 ) & ~3 ) )
		return 0;

	switch( pkm_format )
	{
	case PKM_FORMAT_ETC1_RGB8:
		parsed->format = WF_ETC_FORMAT_ETC1_RGB8;
		parsed->channels = 3;
		parsed->block_size = 8;
		break;
	case PKM_FORMAT_ETC2_RGB8:
		parsed->format = WF_ETC_FORMAT_ETC2_RGB8;
		parsed->channels = 3;
		parsed->block_size = 8;
		break;
	case PKM_FORMAT_ETC2_RGBA8_OLD:
	case PKM_FORMAT_ETC2_RGBA8:
		parsed->format = WF_ETC_FORMAT_ETC2_RGBA8;
		parsed->channels = 4;
		parsed->block_size = 16;
		break;
	case PKM_FORMAT_ETC2_RGB8A1:
		parsed->format = WF_ETC_FORMAT_ETC2_RGB8A1;
		parsed->channels = 4;
		parsed->block_size = 8;
		break;
	case PKM_FORMAT_EAC_R11:
		parsed->format = WF_ETC_FORMAT_EAC_R11;
		parsed->channels = 1;
		parsed->block_size = 8;
		break;
	case PKM_FORMAT_EAC_RG11:
		parsed->format = WF_ETC_FORMAT_EAC_RG11;
		parsed->channels = 3;
		parsed->block_size = 16;
		break;
	case PKM_FORMAT_EAC_SIGNED_R11:
		parsed->format = WF_ETC_FORMAT_EAC_SIGNED_R11;
		parsed->channels = 1;
		parsed->block_size = 8;
		break;
	case PKM_FORMAT_EAC_SIGNED_RG11:
		parsed->format = WF_ETC_FORMAT_EAC_SIGNED_RG11;
		parsed->channels = 3;
		parsed->block_size = 16;
		break;
	default:
		return 0;
	}
	return 1;
}

static int stbi__pkm_test( stbi__context* s )
{
	const int p = stbi__get8( s );
	const int k = stbi__get8( s );
	const int m = stbi__get8( s );
	const int space = stbi__get8( s );
	const int version = stbi__get8( s );
	const int zero = stbi__get8( s );
	stbi__rewind( s );
	return p == 'P' && k == 'K' && m == 'M' && space == ' ' &&
	       ( version == '1' || version == '2' ) && zero == '0';
}

#ifndef STBI_NO_STDIO
int stbi__pkm_test_filename( char const* filename )
{
	int result;
	FILE* file = fopen( filename, "rb" );
	if( !file )
		return 0;
	result = stbi__pkm_test_file( file );
	fclose( file );
	return result;
}

int stbi__pkm_test_file( FILE* file )
{
	stbi__context context;
	int result;
	const long position = ftell( file );
	stbi__start_file( &context, file );
	result = stbi__pkm_test( &context );
	fseek( file, position, SEEK_SET );
	return result;
}
#endif

int stbi__pkm_test_memory( stbi_uc const* buffer, int length )
{
	stbi__context context;
	if( length < 6 )
		return 0;
	stbi__start_mem( &context, buffer, length );
	return stbi__pkm_test( &context );
}

int stbi__pkm_test_callbacks( stbi_io_callbacks const* callbacks, void* user )
{
	stbi__context context;
	stbi__start_callbacks( &context, (stbi_io_callbacks*)callbacks, user );
	return stbi__pkm_test( &context );
}

static int stbi__pkm_info( stbi__context* s, int* x, int* y, int* comp )
{
	stbi_uc header[PKM_HEADER_SIZE];
	stbi__pkm_header parsed;

	if( !stbi__getn( s, header, PKM_HEADER_SIZE ) ||
	    !stbi__pkm_parse_header( header, &parsed ) )
	{
		stbi__rewind( s );
		return 0;
	}
	*x = s->img_x = parsed.width;
	*y = s->img_y = parsed.height;
	*comp = s->img_n = parsed.channels;
	stbi__rewind( s );
	return 1;
}

int stbi__pkm_info_from_memory(
	stbi_uc const* buffer, int length, int* x, int* y, int* comp )
{
	stbi__context context;
	stbi__start_mem( &context, buffer, length );
	return stbi__pkm_info( &context, x, y, comp );
}

int stbi__pkm_info_from_callbacks(
	stbi_io_callbacks const* callbacks, void* user, int* x, int* y, int* comp )
{
	stbi__context context;
	stbi__start_callbacks( &context, (stbi_io_callbacks*)callbacks, user );
	return stbi__pkm_info( &context, x, y, comp );
}

#ifndef STBI_NO_STDIO
int stbi__pkm_info_from_path(
	char const* filename, int* x, int* y, int* comp )
{
	int result;
	FILE* file = fopen( filename, "rb" );
	if( !file )
		return 0;
	result = stbi__pkm_info_from_file( file, x, y, comp );
	fclose( file );
	return result;
}

int stbi__pkm_info_from_file( FILE* file, int* x, int* y, int* comp )
{
	stbi__context context;
	int result;
	const long position = ftell( file );
	stbi__start_file( &context, file );
	result = stbi__pkm_info( &context, x, y, comp );
	fseek( file, position, SEEK_SET );
	return result;
}
#endif

static void* stbi__pkm_load(
	stbi__context* s, int* x, int* y, int* comp, int req_comp )
{
	stbi_uc header[PKM_HEADER_SIZE];
	stbi__pkm_header parsed;
	stbi_uc* compressed;
	stbi_uc* decoded;
	size_t compressed_size;

	if( !stbi__getn( s, header, PKM_HEADER_SIZE ) ||
	    !stbi__pkm_parse_header( header, &parsed ) )
		return stbi__errpuc( "bad PKM", "Invalid or unsupported PKM header" );
	if( parsed.width > STBI_MAX_DIMENSIONS || parsed.height > STBI_MAX_DIMENSIONS )
		return stbi__errpuc( "too large", "Very large PKM image (corrupt?)" );
	if( !stbi__mad3sizes_valid(
			parsed.width, parsed.height, parsed.channels, 0 ) )
		return stbi__errpuc( "too large", "PKM image too large to decode" );

	compressed_size =
		(size_t)( parsed.encoded_width / 4 ) *
		(size_t)( parsed.encoded_height / 4 ) * parsed.block_size;
	if( compressed_size > 0x7fffffffu )
		return stbi__errpuc( "too large", "PKM payload is too large" );

	compressed = (stbi_uc*)STBI_MALLOC( compressed_size );
	if( NULL == compressed )
		return stbi__errpuc( "outofmem", "Out of memory" );
	if( !stbi__getn( s, compressed, (int)compressed_size ) )
	{
		STBI_FREE( compressed );
		return stbi__errpuc( "truncated PKM", "PKM file is truncated" );
	}

	decoded = (stbi_uc*)stbi__malloc_mad3(
		parsed.width, parsed.height, parsed.channels, 0 );
	if( NULL == decoded )
	{
		STBI_FREE( compressed );
		return stbi__errpuc( "outofmem", "Out of memory" );
	}
	if( !wfETC_DecodeImage(
			compressed, decoded, (uint32_t)parsed.width, (uint32_t)parsed.height,
			(uint32_t)parsed.encoded_width, (uint32_t)parsed.encoded_height,
			parsed.format ) )
	{
		STBI_FREE( compressed );
		STBI_FREE( decoded );
		return stbi__errpuc( "bad PKM", "PKM texture data could not be decoded" );
	}
	STBI_FREE( compressed );

	*x = s->img_x = parsed.width;
	*y = s->img_y = parsed.height;
	*comp = s->img_n = parsed.channels;
	if( req_comp >= 1 && req_comp <= 4 && req_comp != parsed.channels )
	{
		decoded = stbi__convert_format(
			decoded, parsed.channels, req_comp, parsed.width, parsed.height );
	}
	return decoded;
}

#ifndef STBI_NO_STDIO
void* stbi__pkm_load_from_file(
	FILE* file, int* x, int* y, int* comp, int req_comp )
{
	stbi__context context;
	stbi__start_file( &context, file );
	return stbi__pkm_load( &context, x, y, comp, req_comp );
}

void* stbi__pkm_load_from_path(
	char const* filename, int* x, int* y, int* comp, int req_comp )
{
	void* data;
	FILE* file = fopen( filename, "rb" );
	if( !file )
		return NULL;
	data = stbi__pkm_load_from_file( file, x, y, comp, req_comp );
	fclose( file );
	return data;
}
#endif

void* stbi__pkm_load_from_memory(
	stbi_uc const* buffer, int length, int* x, int* y, int* comp, int req_comp )
{
	stbi__context context;
	stbi__start_mem( &context, buffer, length );
	return stbi__pkm_load( &context, x, y, comp, req_comp );
}

void* stbi__pkm_load_from_callbacks(
	stbi_io_callbacks const* callbacks, void* user,
	int* x, int* y, int* comp, int req_comp )
{
	stbi__context context;
	stbi__start_callbacks( &context, (stbi_io_callbacks*)callbacks, user );
	return stbi__pkm_load( &context, x, y, comp, req_comp );
}
