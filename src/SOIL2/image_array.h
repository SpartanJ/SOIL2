/*
    Francisco Zavala

    soil_image_array.h

    Internal image array utilities for SOIL.
    This header is NOT part of the public SOIL API.

    Responsibilities:
    - SOIL_ImageArray structure
    - CPU-side image array manipulation
    - Extraction of image arrays from grid-based atlases
    - Image preprocessing before GPU upload

    Ownership rules:
    - Functions that return SOIL_ImageArray allocate memory.
    - The caller is responsible for calling soil_image_array_free().
*/

#ifndef SOIL_IMAGE_ARRAY_H
#define SOIL_IMAGE_ARRAY_H

#include "image_array_helper.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Frees all memory associated with an image array.
   Safe to call with NULL or partially-initialized arrays. */
void SOIL_image_array_free(SOIL_ImageArray *imgArray);

/* Frees floating-point image layers and clears their pointers. */
void image_array_free_f32( float **data, int layers );

/* Vertically flips all layers in-place.
   Does nothing if imgArray is NULL or invalid. */
void image_array_invert_y(SOIL_ImageArray *imgArray);

/* Vertically flips floating-point image layers in-place. */
void image_array_invert_y_f32(
	float **data,
	int layers,
	int width,
	int height,
	int channels
);

/* Premultiplies RGB channels by alpha (if present).
   Requires channels >= 4; otherwise does nothing. */
void image_array_premultiply_alpha(SOIL_ImageArray *imgArray);

/* Converts RGB data to NTSC-safe color range.
   Requires channels >= 3; otherwise does nothing. */
void image_array_to_NTSC_safe(SOIL_ImageArray *imgArray);

/* Converts RGB data to YCoCg color space.
   Requires channels >= 3; otherwise does nothing. */
void image_array_to_YCoCg(SOIL_ImageArray *imgArray);

/* Resizes all layers to the nearest power-of-two dimensions.
   Returns non-zero on success, zero on failure. */
int image_array_resize_POT(SOIL_ImageArray *imgArray);

/* Resizes floating-point image layers to explicit dimensions.
   The function replaces and frees each non-NULL layer on success. */
int image_array_resize_f32(
	float **data,
	int layers,
	int channels,
	int old_width,
	int old_height,
	int new_width,
	int new_height
);

/* Resizes floating-point image layers to power-of-two dimensions. */
int image_array_resize_POT_f32(
	float **data,
	int layers,
	int channels,
	int *width,
	int *height
);

/* Creates the next floating-point mip level using a box filter.
   The caller owns the returned buffer. */
float *image_array_make_next_mipmap_f32(
	const float *data,
	int channels,
	int width,
	int height,
	int *new_width,
	int *new_height
);

/* Reduces all layers so that width and height do not exceed max_size.
   Aspect ratio is preserved.
   Returns non-zero on success, zero on failure. */
int image_array_reduce_to_max(SOIL_ImageArray *imgArray, int max_size);

/*
    Extracts an image array from a grid-based atlas.

    Assumptions:
    - The atlas is perfectly divisible by cols × rows.
    - All tiles have identical dimensions.
    - No padding, no rotation, no metadata.

    On failure, returns an empty SOIL_ImageArray (all fields zeroed).
    The returned array must be freed with soil_image_array_free().
*/
SOIL_ImageArray extract_image_array_from_atlas_grid(
    const unsigned char *atlasData,
    int atlasW,
    int atlasH,
    int cols,
    int rows,
    int channels
);

#ifdef __cplusplus
}
#endif

#endif /* SOIL_IMAGE_ARRAY_H */
