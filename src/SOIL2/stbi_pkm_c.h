#include "pkm_helper.h"
#include "etc1_utils.h"

static int stbi_pkm_test(stbi *s)
{
	//	check the magic number
	if (get8(s) != 'P') {
		stbi_rewind(s);
		return 0;
	}

	if (get8(s) != 'K') {
		stbi_rewind(s);
		return 0;
	}

	if (get8(s) != 'M') {
		stbi_rewind(s);
		return 0;
	}

	if (get8(s) != ' ') {
		stbi_rewind(s);
		return 0;
	}

	if (get8(s) != '1') {
		stbi_rewind(s);
		return 0;
	}

	if (get8(s) != '0') {
		stbi_rewind(s);
		return 0;
	}

	stbi_rewind(s);
	return 1;
}

#ifndef STBI_NO_STDIO

int      stbi_pkm_test_filename        		(char const *filename)
{
   int r;
   FILE *f = fopen(filename, "rb");
   if (!f) return 0;
   r = stbi_pkm_test_file(f);
   fclose(f);
   return r;
}

int      stbi_pkm_test_file        (FILE *f)
{
   stbi s;
   int r,n = ftell(f);
   start_file(&s,f);
   r = stbi_pkm_test(&s);
   fseek(f,n,SEEK_SET);
   return r;
}
#endif

int      stbi_pkm_test_memory      (stbi_uc const *buffer, int len)
{
   stbi s;
   start_mem(&s,buffer, len);
   return stbi_pkm_test(&s);
}

int      stbi_pkm_test_callbacks      (stbi_io_callbacks const *clbk, void *user)
{
   stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi_pkm_test(&s);
}

static int stbi_pkm_info(stbi *s, int *x, int *y, int *comp )
{
	PKMHeader header;
	unsigned int width, height;

	getn( s, (stbi_uc*)(&header), sizeof(PKMHeader) );

	if ( 0 != strcmp( header.aName, "PKM 10" ) ) {
		stbi_rewind(s);
		return 0;
	}

	width = (header.iWidthMSB << 8) | header.iWidthLSB;
	height = (header.iHeightMSB << 8) | header.iHeightLSB;

	*x = s->img_x = width;
	*y = s->img_y = height;
	*comp = s->img_n = 3;

	stbi_rewind(s);

	return 1;
}

int stbi_pkm_info_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp )
{
	stbi s;
	start_mem(&s,buffer, len);
	return stbi_pkm_info( &s, x, y, comp );
}

int stbi_pkm_info_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp)
{
	stbi s;
	start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
	return stbi_pkm_info( &s, x, y, comp );
}

#ifndef STBI_NO_STDIO
int stbi_pkm_info_from_path(char const *filename,     int *x, int *y, int *comp)
{
   int res;
   FILE *f = fopen(filename, "rb");
   if (!f) return 0;
   res = stbi_pkm_info_from_file( f, x, y, comp );
   fclose(f);
   return res;
}

int stbi_pkm_info_from_file(FILE *f,                  int *x, int *y, int *comp)
{
   stbi s;
   int res;
   long n = ftell(f);
   start_file(&s, f);
   res = stbi_pkm_info(&s, x, y, comp);
   fseek(f, n, SEEK_SET);
   return res;
}
#endif

static stbi_uc * stbi_pkm_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
	stbi_uc *pkm_data = NULL;
	stbi_uc *pkm_res_data = NULL;
	PKMHeader header;
	unsigned int width;
	unsigned int height;
	unsigned int align = 0;
	unsigned int bpr;
	unsigned int size;
	unsigned int compressedSize;

	int res;

	getn( s, (stbi_uc*)(&header), sizeof(PKMHeader) );

	if ( 0 != strcmp( header.aName, "PKM 10" ) ) {
		return NULL;
	}

	width = (header.iWidthMSB << 8) | header.iWidthLSB;
	height = (header.iHeightMSB << 8) | header.iHeightLSB;

	*x = s->img_x = width;
	*y = s->img_y = height;
	*comp = s->img_n = 3;

	compressedSize = etc1_get_encoded_data_size(width, height);

	pkm_data = (stbi_uc *)malloc(compressedSize);
	getn( s, pkm_data, compressedSize );

	bpr = ((width * 3) + align) & ~align;
	size = bpr * height;
	pkm_res_data = (stbi_uc *)malloc(size);

	res = etc1_decode_image((const etc1_byte*)pkm_data, (etc1_byte*)pkm_res_data, width, height, 3, bpr);

	free( pkm_data );

	if ( 0 == res ) {
		if( (req_comp <= 4) && (req_comp >= 1) ) {
			//	user has some requirements, meet them
			if( req_comp != s->img_n ) {
				pkm_res_data = convert_format( pkm_res_data, s->img_n, req_comp, s->img_x, s->img_y );
				*comp = req_comp;
			}
		}

		return (stbi_uc *)pkm_res_data;
	} else {
		free( pkm_res_data );
	}

	return NULL;
}

#ifndef STBI_NO_STDIO
stbi_uc *stbi_pkm_load_from_file   (FILE *f,                  int *x, int *y, int *comp, int req_comp)
{
	stbi s;
	start_file(&s,f);
	return stbi_pkm_load(&s,x,y,comp,req_comp);
}

stbi_uc *stbi_pkm_load_from_path             (char const*filename,           int *x, int *y, int *comp, int req_comp)
{
   stbi_uc *data;
   FILE *f = fopen(filename, "rb");
   if (!f) return NULL;
   data = stbi_pkm_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return data;
}
#endif

stbi_uc *stbi_pkm_load_from_memory (stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_mem(&s,buffer, len);
   return stbi_pkm_load(&s,x,y,comp,req_comp);
}

stbi_uc *stbi_pkm_load_from_callbacks (stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
	stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi_pkm_load(&s,x,y,comp,req_comp);
}
