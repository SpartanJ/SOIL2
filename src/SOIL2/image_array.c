
#include "image_array.h"
#include <stdlib.h>
#include <string.h>
#include "image_helper.h"

extern const char *result_string_pointer;

static void image_array_invert_rows(
	void *data,
	int height,
	size_t row_bytes
)
{
	int y;
	unsigned char *image = (unsigned char *)data;
	if( image == NULL )
	{
		return;
	}

	for( y = 0; y < height / 2; ++y )
	{
		size_t x;
		unsigned char *row_top = image + (size_t)y * row_bytes;
		unsigned char *row_bottom =
			image + (size_t)(height - 1 - y) * row_bytes;
		for( x = 0; x < row_bytes; ++x )
		{
			const unsigned char value = row_top[x];
			row_top[x] = row_bottom[x];
			row_bottom[x] = value;
		}
	}
}

void SOIL_image_array_free(SOIL_ImageArray *imgArray){
    if(imgArray == NULL){
        return;
    }

    if(imgArray->data != NULL){
        for(int i = 0; i < imgArray->layers; i++){
            if(imgArray->data[i] != NULL){
                free(imgArray->data[i]);
            }
        }
        free(imgArray->data);
    }

    imgArray->data = NULL;
    imgArray->width = 0;
    imgArray->height = 0;
    imgArray->layers = 0;
    imgArray->channels = 0;
}

void image_array_free_f32( float **data, int layers )
{
	int layer;
	if( data == NULL )
	{
		return;
	}
	for( layer = 0; layer < layers; ++layer )
	{
		free( data[layer] );
		data[layer] = NULL;
	}
}

void image_array_invert_y(SOIL_ImageArray* imgArray){
	int layer;

	if (!imgArray || !imgArray->data)
		return;

	for( layer = 0; layer < imgArray->layers; ++layer )
	{
		image_array_invert_rows(
			imgArray->data[layer],
			imgArray->height,
			(size_t)imgArray->width * (size_t)imgArray->channels );
	}
}

void image_array_invert_y_f32(
	float **data,
	int layers,
	int width,
	int height,
	int channels
)
{
	int layer;
	if( data == NULL || layers < 1 || width < 1 || height < 1 || channels < 1 )
	{
		return;
	}
	for( layer = 0; layer < layers; ++layer )
	{
		image_array_invert_rows(
			data[layer],
			height,
			(size_t)width * (size_t)channels * sizeof(float) );
	}
}

void image_array_premultiply_alpha(SOIL_ImageArray* imgArray){

	if (!imgArray || !imgArray->data)
		return;

	const size_t numPixels = (size_t)(imgArray->width) * (size_t)(imgArray->height);

	for (int layer = 0; layer < imgArray->layers; ++layer) {

		unsigned char *image = imgArray->data[layer];

		if (!image) continue;

		for (size_t i = 0; i < numPixels; ++i) {
			unsigned char *pixel = image + i * (size_t)(imgArray->channels);

			if (imgArray->channels >= 4) {
				float alpha = pixel[3] / 255.0f;
				pixel[0] = (unsigned char)(pixel[0] * alpha);
				pixel[1] = (unsigned char)(pixel[1] * alpha);
				pixel[2] = (unsigned char)(pixel[2] * alpha);
			}
		}
	}
}

void image_array_to_NTSC_safe(SOIL_ImageArray* imgArray){

	if (!imgArray || !imgArray->data)
		return;

	const size_t numPixels = (size_t)(imgArray->width) * (size_t)(imgArray->height);

	for (int layer = 0; layer < imgArray->layers; ++layer) {

		unsigned char *image = imgArray->data[layer];

		if (!image) continue;

		for (size_t i = 0; i < numPixels; ++i) {
			unsigned char *pixel = image + i * (size_t)(imgArray->channels);

			if (imgArray->channels >= 3) {
				unsigned char r = pixel[0];
				unsigned char g = pixel[1];
				unsigned char b = pixel[2];

				unsigned char y  = (unsigned char)(0.299f * r + 0.587f * g + 0.114f * b);
				unsigned char co = (unsigned char)(r - b + 128);
				unsigned char cg = (unsigned char)(-0.168736f * r - 0.331264f * g + 0.5f * b + 128);

				pixel[0] = co;
				pixel[1] = cg;
				pixel[2] = y;
			}
		}
	}

}

void image_array_to_YCoCg(SOIL_ImageArray* imgArray){

	if (!imgArray || !imgArray->data)
		return;

	const size_t numPixels = (size_t)(imgArray->width) * (size_t)(imgArray->height);

	for (int layer = 0; layer < imgArray->layers; ++layer) {

		unsigned char *image = imgArray->data[layer];

		if (!image) continue;

		for (size_t i = 0; i < numPixels; ++i) {
			unsigned char *pixel = image + i * (size_t)(imgArray->channels);

			if (imgArray->channels >= 3) {
				unsigned char r = pixel[0];
				unsigned char g = pixel[1];
				unsigned char b = pixel[2];

				int y  = (  r + ( 2 * g ) +   b ) >> 2;
				int co = (  r           -   b ) >> 1;
				int cg = ( -r + ( 2 * g ) -   b ) >> 2;

				pixel[0] = (unsigned char)(co + 128);
				pixel[1] = (unsigned char)(cg + 128);
				pixel[2] = (unsigned char)(y);
			}
		}
	}
}

int image_array_resize_POT(SOIL_ImageArray *imgArray)
{
    if (!imgArray || !imgArray->data)
        return 1;

    int new_w = image_next_power_of_two(imgArray->width);
    int new_h = image_next_power_of_two(imgArray->height);

    if (new_w == imgArray->width && new_h == imgArray->height)
        return 1;

    for (int layer = 0; layer < imgArray->layers; ++layer) {

        unsigned char *src = imgArray->data[layer];
        if (!src)
            continue;

        unsigned char *dst = (unsigned char*)malloc(
            (size_t)new_w * (size_t)new_h * (size_t)imgArray->channels
        );

        if (!dst)
            return 0;

        up_scale_image(
            src,
            imgArray->width,
            imgArray->height,
            imgArray->channels,
            dst,
            new_w,
            new_h
        );

        free(src);
        imgArray->data[layer] = dst;
    }

    imgArray->width  = new_w;
    imgArray->height = new_h;

    return 1;
}

int image_array_resize_f32(
	float **data,
	int layers,
	int channels,
	int old_width,
	int old_height,
	int new_width,
	int new_height
)
{
	int layer;

	if( data == NULL || layers < 1 || channels < 1 ||
		old_width < 1 || old_height < 1 ||
		new_width < 1 || new_height < 1 )
	{
		return 0;
	}
	if( old_width == new_width && old_height == new_height )
	{
		return 1;
	}

	for( layer = 0; layer < layers; ++layer )
	{
		float *resized;
		if( data[layer] == NULL )
		{
			continue;
		}

		resized = (float *)malloc(
			(size_t)new_width * (size_t)new_height *
			(size_t)channels * sizeof(float) );
		if( resized == NULL )
		{
			return 0;
		}
		if( !resize_image_f32(
			data[layer], old_width, old_height, channels,
			resized, new_width, new_height ) )
		{
			free( resized );
			return 0;
		}

		free( data[layer] );
		data[layer] = resized;
	}

	return 1;
}

int image_array_resize_POT_f32(
	float **data,
	int layers,
	int channels,
	int *width,
	int *height
)
{
	if( width == NULL || height == NULL )
	{
		return 0;
	}

	const int new_width = image_next_power_of_two( *width );
	const int new_height = image_next_power_of_two( *height );

	if( !image_array_resize_f32(
		data, layers, channels, *width, *height, new_width, new_height ) )
	{
		return 0;
	}

	*width = new_width;
	*height = new_height;
	return 1;
}

float *image_array_make_next_mipmap_f32(
	const float *data,
	int channels,
	int width,
	int height,
	int *new_width,
	int *new_height
)
{
	float *mipmap;
	int x, y, channel;

	if( data == NULL || channels < 1 || width < 1 || height < 1 ||
		new_width == NULL || new_height == NULL )
	{
		return NULL;
	}

	*new_width = width > 1 ? (width + 1) / 2 : 1;
	*new_height = height > 1 ? (height + 1) / 2 : 1;
	mipmap = (float *)malloc(
		(size_t)(*new_width) * (size_t)(*new_height) *
		(size_t)channels * sizeof(float) );
	if( mipmap == NULL )
	{
		return NULL;
	}

	for( y = 0; y < *new_height; ++y )
	{
		for( x = 0; x < *new_width; ++x )
		{
			const int x0 = x * 2;
			const int y0 = y * 2;
			const int x1 = x0 + 1 < width ? x0 + 1 : x0;
			const int y1 = y0 + 1 < height ? y0 + 1 : y0;
			for( channel = 0; channel < channels; ++channel )
			{
				mipmap[(y * (*new_width) + x) * channels + channel] =
					(data[(y0 * width + x0) * channels + channel] +
					 data[(y0 * width + x1) * channels + channel] +
					 data[(y1 * width + x0) * channels + channel] +
					 data[(y1 * width + x1) * channels + channel]) * 0.25f;
			}
		}
	}

	return mipmap;
}

int image_array_reduce_to_max(SOIL_ImageArray *imgArray, int max_size)
{
    if (!imgArray || !imgArray->data)
        return 1;

    if (imgArray->width <= max_size &&
        imgArray->height <= max_size)
        return 1;

    int reduce_block_x = 1;
    int reduce_block_y = 1;

    if (imgArray->width > max_size)
        reduce_block_x = imgArray->width / max_size;

    if (imgArray->height > max_size)
        reduce_block_y = imgArray->height / max_size;

    int new_w = imgArray->width  / reduce_block_x;
    int new_h = imgArray->height / reduce_block_y;

    for (int layer = 0; layer < imgArray->layers; ++layer) {

        unsigned char *src = imgArray->data[layer];
        if (!src)
            continue;

        unsigned char *dst = (unsigned char*)malloc(
            (size_t)new_w * (size_t)new_h * (size_t)imgArray->channels
        );

        if (!dst)
            return 0;

        mipmap_image(
            src,
            imgArray->width,
            imgArray->height,
            imgArray->channels,
            dst,
            reduce_block_x,
            reduce_block_y
        );

        free(src);
        imgArray->data[layer] = dst;
    }

    imgArray->width  = new_w;
    imgArray->height = new_h;

    return 1;
}

SOIL_ImageArray extract_image_array_from_atlas_grid(
    const unsigned char *atlasData,
    int atlasW,
    int atlasH,
    int cols,
    int rows,
    int channels
){
    SOIL_ImageArray result = {0};

    if (!atlasData || cols <= 0 || rows <= 0 || channels <= 0)
        return result;

    if (atlasW % cols != 0 || atlasH % rows != 0)
    {
        result_string_pointer = "Atlas cannot be evenly divided by grid";
        return result;
    }

    int tile_w = atlasW / cols;
    int tile_h = atlasH / rows;
    int numLayers = cols * rows;

    result.width    = tile_w;
    result.height   = tile_h;
    result.layers   = numLayers;
    result.channels = channels;

    result.data = (unsigned char**)malloc(sizeof(unsigned char*) * numLayers);
    if (!result.data){
        SOIL_ImageArray empty;
        memset(&empty, 0, sizeof(empty));
        return empty;
    }

    for (int i = 0; i < numLayers; ++i)
        result.data[i] = NULL;

    const size_t bytesPerPixel = (size_t)channels;
    const size_t tileRowBytes  = (size_t)tile_w * bytesPerPixel;
    const size_t atlasRowBytes = (size_t)atlasW * bytesPerPixel;

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int layer = r * cols + c;

            unsigned char *tile =
                (unsigned char*)malloc((size_t)tile_w * tile_h * bytesPerPixel);

            if (!tile) {
                /* cleanup */
                for (int i = 0; i < numLayers; ++i)
                    free(result.data[i]);
                free(result.data);

                SOIL_ImageArray empty;
                memset(&empty, 0, sizeof(empty));

                return empty;
            }

            for (int y = 0; y < tile_h; ++y) {
                const unsigned char *src =
                    atlasData +
                    ((size_t)(r * tile_h + y) * atlasRowBytes) +
                    ((size_t)c * tileRowBytes);

                unsigned char *dst = tile + (size_t)y * tileRowBytes;
                memcpy(dst, src, tileRowBytes);
            }

            result.data[layer] = tile;
        }
    }

    return result;
}
