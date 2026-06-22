
#include "wfETC.h"
#include <stddef.h>

// specification: http://www.khronos.org/registry/gles/extensions/OES/OES_compressed_ETC1_RGB8_texture.txt

#ifdef WF_LITTLE_ENDIAN
	#define WF_ETC1_COLOR_OFFSET( offset ) ( (64-(offset)) - (4-((offset)%8))*2 )
	#define WF_ETC1_PIXEL_OFFSET( offset ) ( (32-(offset)) - (4-((offset)%8))*2 )
#else
	#define WF_ETC1_COLOR_OFFSET( offset ) (offset-32)
	#define WF_ETC1_PIXEL_OFFSET( offset ) offset
#endif

#define WF_INLINE
#define WF_ASSUME( expr )

#ifndef WF_EXPECT
	#if defined __GNUC__ && __GNUC__
		#define WF_EXPECT( expr, val ) __builtin_expect( expr, val )
	#else
		#define WF_EXPECT( expr, val ) expr
	#endif
#endif

// this table is rearranged from the specification so we do not have to add any logic to index into it
const int32_t wfETC_IntensityTables[8][4] =
{
	{  2,   8,  -2, -8   },
	{  5,  17,  -5, -17  },
	{  9,  29,  -9, -29  },
	{ 13,  42, -13, -42  },
	{ 18,  60, -18, -60  },
	{ 24,  80, -24, -80  },
	{ 33, 106, -33, -106 },
	{ 47, 183, -47, -183 }
};

WF_INLINE
int32_t wfETC_ClampColor( const int32_t x )
{
	if( x < 0   ) { return 0; }
	if( x > 255 ) { return 255; }
	return x;
}

#define WF_ETC1_BUILD_COLOR( dst, blockIdx, colorIdx, intensityTable, baseColor ) \
	dst[ blockIdx ][ colorIdx ] = WF_ETC_RGBA( \
		wfETC_ClampColor( intensityTable[ blockIdx ][ colorIdx ] + baseColor[ blockIdx ][ 0 ] ), \
		wfETC_ClampColor( intensityTable[ blockIdx ][ colorIdx ] + baseColor[ blockIdx ][ 1 ] ), \
		wfETC_ClampColor( intensityTable[ blockIdx ][ colorIdx ] + baseColor[ blockIdx ][ 2 ] ), \
		255 \
	);

typedef struct _wfETC1_Block
{
	int32_t baseColorsAndFlags;
	int32_t pixels;
} wfETC1_Block;

#define WF_ETC1_CHECK_DIFF_BIT( block ) ( block->baseColorsAndFlags & (1<<WF_ETC1_COLOR_OFFSET(33)) )
#define WF_ETC1_CHECK_FLIP_BIT( block ) ( block->baseColorsAndFlags & (1<<WF_ETC1_COLOR_OFFSET(32)) )

WF_INLINE
int32_t wfETC1_ReadColor4( const wfETC1_Block* block, const uint32_t offset )
{
	const uint32_t bitOffset = WF_ETC1_COLOR_OFFSET(offset);
	const int32_t color = ( block->baseColorsAndFlags >> bitOffset ) & 0xf;
	return color | (color<<4);
}

const int32_t wfETC1_Color3IdxLUT[] = { 0, 1, 2, 3, -4, -3, -2, -1 };

WF_INLINE
void wfETC1_ReadColor53( const wfETC1_Block* block, const uint32_t offset3, int32_t* WF_RESTRICT dst5, int32_t* WF_RESTRICT dst3 )
{
	// read the 5 bit color and expand to 8 bit
	{
		const uint32_t bitOffset = WF_ETC1_COLOR_OFFSET((offset3+3));
		const int32_t color = ( block->baseColorsAndFlags >> (bitOffset-3) ) & 0xf8;
		*dst5 = color | (color>>5);
	}

	// read the 3 bit color and expand to 8 bit
	{
		const uint32_t bitOffset = WF_ETC1_COLOR_OFFSET(offset3);
		const int32_t offset = wfETC1_Color3IdxLUT[ ( block->baseColorsAndFlags >> bitOffset ) & 0x7 ];
		int32_t color = (*dst5>>3) + offset;
		color <<= 3;
		color |= (color>>5) & 0x7;
		*dst3 = color;
	}
}

WF_INLINE
int32_t wfETC1_ReadPixel( const wfETC1_Block* block, const uint32_t offset )
{
	return
		(  (block->pixels>>WF_ETC1_PIXEL_OFFSET(offset)) & 0x1  )
		|
		(  ( (block->pixels>>WF_ETC1_PIXEL_OFFSET(16+offset)) & 0x1 ) << 1 )
	;
}

void wfETC1_DecodeBlock( const void* WF_RESTRICT src, void* WF_RESTRICT pDst, const uint32_t dstStride )
{
	const wfETC1_Block* WF_RESTRICT block = ( wfETC1_Block* )src;
	int32_t* WF_RESTRICT dst = (int32_t*)pDst;

	int32_t baseColors[2][3]; // [sub-block][r,g,b]
	int32_t colors[2][4]; // [sub-block][colorIdx]

	// individual mode
	if( WF_ETC1_CHECK_DIFF_BIT( block ) == 0 )
	{
		baseColors[0][0] = wfETC1_ReadColor4( block, 60 );
		baseColors[0][1] = wfETC1_ReadColor4( block, 52 );
		baseColors[0][2] = wfETC1_ReadColor4( block, 44 );
		baseColors[1][0] = wfETC1_ReadColor4( block, 56 );
		baseColors[1][1] = wfETC1_ReadColor4( block, 48 );
		baseColors[1][2] = wfETC1_ReadColor4( block, 40 );
	}
	// differential mode
	else
	{
		wfETC1_ReadColor53( block, 56, &baseColors[0][0], &baseColors[1][0] );
		wfETC1_ReadColor53( block, 48, &baseColors[0][1], &baseColors[1][1] );
		wfETC1_ReadColor53( block, 40, &baseColors[0][2], &baseColors[1][2] );
	}

	// build color tables
	{
		const int32_t* intensityTable[2] = // [sub-block]
		{
			wfETC_IntensityTables[ ( block->baseColorsAndFlags >> WF_ETC1_COLOR_OFFSET(37) ) & 0x7 ],
			wfETC_IntensityTables[ ( block->baseColorsAndFlags >> WF_ETC1_COLOR_OFFSET(34) ) & 0x7 ]
		};
		WF_ETC1_BUILD_COLOR( colors, 0, 0, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 0, 1, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 0, 2, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 0, 3, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 1, 0, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 1, 1, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 1, 2, intensityTable, baseColors );
		WF_ETC1_BUILD_COLOR( colors, 1, 3, intensityTable, baseColors );
	}

	// vertical split
	if( WF_ETC1_CHECK_FLIP_BIT( block ) == 0 )
	{
		// row 0
		dst[0] = colors[0][ wfETC1_ReadPixel( block, 0  ) ];
		dst[1] = colors[0][ wfETC1_ReadPixel( block, 4  ) ];
		dst[2] = colors[1][ wfETC1_ReadPixel( block, 8  ) ];
		dst[3] = colors[1][ wfETC1_ReadPixel( block, 12 ) ];
		dst += dstStride;

		// row 1
		dst[0] = colors[0][ wfETC1_ReadPixel( block, 1  ) ];
		dst[1] = colors[0][ wfETC1_ReadPixel( block, 5  ) ];
		dst[2] = colors[1][ wfETC1_ReadPixel( block, 9  ) ];
		dst[3] = colors[1][ wfETC1_ReadPixel( block, 13 ) ];
		dst += dstStride;

		// row 2
		dst[0] = colors[0][ wfETC1_ReadPixel( block, 2  ) ];
		dst[1] = colors[0][ wfETC1_ReadPixel( block, 6  ) ];
		dst[2] = colors[1][ wfETC1_ReadPixel( block, 10 ) ];
		dst[3] = colors[1][ wfETC1_ReadPixel( block, 14 ) ];
		dst += dstStride;

		// row 3
		dst[0] = colors[0][ wfETC1_ReadPixel( block, 3  ) ];
		dst[1] = colors[0][ wfETC1_ReadPixel( block, 7  ) ];
		dst[2] = colors[1][ wfETC1_ReadPixel( block, 11 ) ];
		dst[3] = colors[1][ wfETC1_ReadPixel( block, 15 ) ];
		//dst += dstStride;
	}
	// horizontal split
	else
	{
		// row 0
		dst[0] = colors[0][ wfETC1_ReadPixel( block, 0  ) ];
		dst[1] = colors[0][ wfETC1_ReadPixel( block, 4  ) ];
		dst[2] = colors[0][ wfETC1_ReadPixel( block, 8  ) ];
		dst[3] = colors[0][ wfETC1_ReadPixel( block, 12 ) ];
		dst += dstStride;

		// row 1
		dst[0] = colors[0][ wfETC1_ReadPixel( block, 1  ) ];
		dst[1] = colors[0][ wfETC1_ReadPixel( block, 5  ) ];
		dst[2] = colors[0][ wfETC1_ReadPixel( block, 9  ) ];
		dst[3] = colors[0][ wfETC1_ReadPixel( block, 13 ) ];
		dst += dstStride;

		// row 2
		dst[0] = colors[1][ wfETC1_ReadPixel( block, 2  ) ];
		dst[1] = colors[1][ wfETC1_ReadPixel( block, 6  ) ];
		dst[2] = colors[1][ wfETC1_ReadPixel( block, 10 ) ];
		dst[3] = colors[1][ wfETC1_ReadPixel( block, 14 ) ];
		dst += dstStride;

		// row 3
		dst[0] = colors[1][ wfETC1_ReadPixel( block, 3  ) ];
		dst[1] = colors[1][ wfETC1_ReadPixel( block, 7  ) ];
		dst[2] = colors[1][ wfETC1_ReadPixel( block, 11 ) ];
		dst[3] = colors[1][ wfETC1_ReadPixel( block, 15 ) ];
		//dst += dstStride;
	}
}

void wfETC1_DecodeImage( const void* WF_RESTRICT pSrc, void* WF_RESTRICT pDst, const uint32_t width, const uint32_t height )
{
	const uint8_t* WF_RESTRICT src = (uint8_t*)pSrc;
	uint8_t* WF_RESTRICT dst = (uint8_t*)pDst;
	const uint32_t widthBlocks  = width/4;
	const uint32_t heightBlocks = height/4;
	uint32_t x, y;
	for( y = 0; y != heightBlocks; ++y )
	{
		for( x = 0; x != widthBlocks; ++x )
		{
			wfETC1_DecodeBlock( src, dst, width );
			src += 8;
			dst += 16;
		}
		dst += width*4*3;
	}
}

/*
	ETC2/EAC decoding below is derived from detex.

	Copyright (c) 2015 Harm Hanemaaijer <fgenfb@yahoo.com>

	Permission to use, copy, modify, and/or distribute this software for any
	purpose with or without fee is hereby granted, provided that the above
	copyright notice and this permission notice appear in all copies.

	THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
	WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
	merchantability and fitness. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
	ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
	WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
	ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
	OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

static const int wfETC2_complement3_table[8] =
{
	0, 1, 2, 3, -4, -3, -2, -1
};

static const int wfETC2_distance_table[8] =
{
	3, 6, 11, 16, 23, 32, 41, 64
};

static const int8_t wfEAC_modifier_table[16][8] =
{
	{ -3, -6, -9, -15, 2, 5, 8, 14 },
	{ -3, -7, -10, -13, 2, 6, 9, 12 },
	{ -2, -5, -8, -13, 1, 4, 7, 12 },
	{ -2, -4, -6, -13, 1, 3, 5, 12 },
	{ -3, -6, -8, -12, 2, 5, 7, 11 },
	{ -3, -7, -9, -11, 2, 6, 8, 10 },
	{ -4, -7, -8, -11, 3, 6, 7, 10 },
	{ -3, -5, -8, -11, 2, 4, 7, 10 },
	{ -2, -6, -8, -10, 1, 5, 7, 9 },
	{ -2, -5, -8, -10, 1, 4, 7, 9 },
	{ -2, -4, -8, -10, 1, 3, 7, 9 },
	{ -2, -5, -7, -10, 1, 4, 6, 9 },
	{ -3, -4, -7, -10, 2, 3, 6, 9 },
	{ -1, -2, -3, -10, 0, 1, 2, 9 },
	{ -4, -6, -8, -9, 3, 5, 7, 8 },
	{ -3, -5, -7, -9, 2, 4, 6, 8 }
};

static int wfETC2_clamp_byte( int value )
{
	if( value < 0 )
		return 0;
	if( value > 255 )
		return 255;
	return value;
}

static int wfETC2_pixel_index( const uint8_t* block, int index )
{
	const uint32_t indices =
		( (uint32_t)block[4] << 24 ) | ( (uint32_t)block[5] << 16 ) |
		( (uint32_t)block[6] << 8 ) | block[7];
	return (int)( ( indices >> index ) & 1 ) |
	       (int)( ( ( indices >> ( index + 16 ) ) & 1 ) << 1 );
}

static int wfETC2_pixel_offset( int index )
{
	return ( index & 3 ) * 4 + ( ( index & 12 ) >> 2 );
}

static void wfETC2_write_rgba(
	uint8_t* pixels, int offset, int red, int green, int blue, int alpha )
{
	pixels[offset * 4 + 0] = (uint8_t)wfETC2_clamp_byte( red );
	pixels[offset * 4 + 1] = (uint8_t)wfETC2_clamp_byte( green );
	pixels[offset * 4 + 2] = (uint8_t)wfETC2_clamp_byte( blue );
	pixels[offset * 4 + 3] = (uint8_t)alpha;
}

static void wfETC2_decode_subblocks(
	const uint8_t* block, uint8_t* pixels, int punchthrough )
{
	int base[2][3];
	int table[2];
	int flip = block[3] & 1;
	int differential = block[3] & 2;
	int i;

	if( differential )
	{
		int channel;
		for( channel = 0; channel < 3; ++channel )
		{
			const int first = block[channel] >> 3;
			int second = first + wfETC2_complement3_table[block[channel] & 7];
			if( second < 0 )
				second = 0;
			else if( second > 31 )
				second = 31;
			base[0][channel] = ( first << 3 ) | ( first >> 2 );
			base[1][channel] = ( second << 3 ) | ( second >> 2 );
		}
	}
	else
	{
		int channel;
		for( channel = 0; channel < 3; ++channel )
		{
			base[0][channel] = ( block[channel] & 0xF0 ) |
			                   ( block[channel] >> 4 );
			base[1][channel] = ( ( block[channel] & 0x0F ) << 4 ) |
			                   ( block[channel] & 0x0F );
		}
	}
	table[0] = ( block[3] >> 5 ) & 7;
	table[1] = ( block[3] >> 2 ) & 7;

	for( i = 0; i < 16; ++i )
	{
		const int x = i >> 2;
		const int y = i & 3;
		const int subblock = flip ? ( y >= 2 ) : ( x >= 2 );
		const int pixel_index = wfETC2_pixel_index( block, i );
		const int modifier = punchthrough ?
			( pixel_index == 0 || pixel_index == 2 ? 0 :
				( pixel_index == 1 ? wfETC_IntensityTables[table[subblock]][1] :
					wfETC_IntensityTables[table[subblock]][3] ) ) :
			wfETC_IntensityTables[table[subblock]][pixel_index];
		const int alpha = punchthrough && pixel_index == 2 ? 0 : 255;
		const int offset = wfETC2_pixel_offset( i );
		if( alpha == 0 )
			wfETC2_write_rgba( pixels, offset, 0, 0, 0, 0 );
		else
			wfETC2_write_rgba(
				pixels, offset,
				base[subblock][0] + modifier,
				base[subblock][1] + modifier,
				base[subblock][2] + modifier, 255 );
	}
}

static void wfETC2_decode_t_or_h(
	const uint8_t* block, uint8_t* pixels, int h_mode, int punchthrough )
{
	int color1[3];
	int color2[3];
	int paint[4][3];
	int distance;
	int i;

	if( !h_mode )
	{
		color1[0] = ( ( block[0] & 0x18 ) >> 1 ) | ( block[0] & 3 );
		color1[1] = block[1] >> 4;
		color1[2] = block[1] & 0x0F;
		color2[0] = block[2] >> 4;
		color2[1] = block[2] & 0x0F;
		color2[2] = block[3] >> 4;
		for( i = 0; i < 3; ++i )
		{
			color1[i] |= color1[i] << 4;
			color2[i] |= color2[i] << 4;
		}
		distance = wfETC2_distance_table[
			( ( block[3] & 0x0C ) >> 1 ) | ( block[3] & 1 )];
		for( i = 0; i < 3; ++i )
		{
			paint[0][i] = color1[i];
			paint[1][i] = wfETC2_clamp_byte( color2[i] + distance );
			paint[2][i] = color2[i];
			paint[3][i] = wfETC2_clamp_byte( color2[i] - distance );
		}
	}
	else
	{
		int value1;
		int value2;
		color1[0] = ( block[0] & 0x78 ) >> 3;
		color1[1] = ( ( block[0] & 7 ) << 1 ) | ( ( block[1] >> 4 ) & 1 );
		color1[2] = ( block[1] & 8 ) | ( ( block[1] & 3 ) << 1 ) |
		            ( block[2] >> 7 );
		color2[0] = ( block[2] & 0x78 ) >> 3;
		color2[1] = ( ( block[2] & 7 ) << 1 ) | ( block[3] >> 7 );
		color2[2] = ( block[3] & 0x78 ) >> 3;
		for( i = 0; i < 3; ++i )
		{
			color1[i] |= color1[i] << 4;
			color2[i] |= color2[i] << 4;
		}
		value1 = ( color1[0] << 16 ) | ( color1[1] << 8 ) | color1[2];
		value2 = ( color2[0] << 16 ) | ( color2[1] << 8 ) | color2[2];
		distance = wfETC2_distance_table[
			( block[3] & 4 ) | ( ( block[3] & 1 ) << 1 ) |
			( value1 >= value2 )];
		for( i = 0; i < 3; ++i )
		{
			paint[0][i] = wfETC2_clamp_byte( color1[i] + distance );
			paint[1][i] = wfETC2_clamp_byte( color1[i] - distance );
			paint[2][i] = wfETC2_clamp_byte( color2[i] + distance );
			paint[3][i] = wfETC2_clamp_byte( color2[i] - distance );
		}
	}

	for( i = 0; i < 16; ++i )
	{
		const int index = wfETC2_pixel_index( block, i );
		const int offset = wfETC2_pixel_offset( i );
		if( punchthrough && index == 2 )
			wfETC2_write_rgba( pixels, offset, 0, 0, 0, 0 );
		else
			wfETC2_write_rgba(
				pixels, offset, paint[index][0], paint[index][1],
				paint[index][2], 255 );
	}
}

static void wfETC2_decode_planar( const uint8_t* block, uint8_t* pixels )
{
	int origin[3];
	int horizontal[3];
	int vertical[3];
	int x;
	int y;

	origin[0] = ( block[0] & 0x7E ) >> 1;
	origin[1] = ( ( block[0] & 1 ) << 6 ) | ( ( block[1] & 0x7E ) >> 1 );
	origin[2] = ( ( block[1] & 1 ) << 5 ) | ( block[2] & 0x18 ) |
	            ( ( block[2] & 3 ) << 1 ) | ( block[3] >> 7 );
	horizontal[0] = ( ( block[3] & 0x7C ) >> 1 ) | ( block[3] & 1 );
	horizontal[1] = block[4] >> 1;
	horizontal[2] = ( ( block[4] & 1 ) << 5 ) | ( block[5] >> 3 );
	vertical[0] = ( ( block[5] & 7 ) << 3 ) | ( block[6] >> 5 );
	vertical[1] = ( ( block[6] & 0x1F ) << 2 ) | ( block[7] >> 6 );
	vertical[2] = block[7] & 0x3F;

	origin[0] = ( origin[0] << 2 ) | ( origin[0] >> 4 );
	origin[1] = ( origin[1] << 1 ) | ( origin[1] >> 6 );
	origin[2] = ( origin[2] << 2 ) | ( origin[2] >> 4 );
	horizontal[0] = ( horizontal[0] << 2 ) | ( horizontal[0] >> 4 );
	horizontal[1] = ( horizontal[1] << 1 ) | ( horizontal[1] >> 6 );
	horizontal[2] = ( horizontal[2] << 2 ) | ( horizontal[2] >> 4 );
	vertical[0] = ( vertical[0] << 2 ) | ( vertical[0] >> 4 );
	vertical[1] = ( vertical[1] << 1 ) | ( vertical[1] >> 6 );
	vertical[2] = ( vertical[2] << 2 ) | ( vertical[2] >> 4 );

	for( y = 0; y < 4; ++y )
	{
		for( x = 0; x < 4; ++x )
		{
			const int red = ( x * ( horizontal[0] - origin[0] ) +
				y * ( vertical[0] - origin[0] ) + 4 * origin[0] + 2 ) >> 2;
			const int green = ( x * ( horizontal[1] - origin[1] ) +
				y * ( vertical[1] - origin[1] ) + 4 * origin[1] + 2 ) >> 2;
			const int blue = ( x * ( horizontal[2] - origin[2] ) +
				y * ( vertical[2] - origin[2] ) + 4 * origin[2] + 2 ) >> 2;
			wfETC2_write_rgba( pixels, y * 4 + x, red, green, blue, 255 );
		}
	}
}

static void wfETC2_decode_rgb_block(
	const uint8_t* block, uint8_t* pixels, int punchthrough )
{
	int second;
	if( !punchthrough && ( block[3] & 2 ) == 0 )
	{
		wfETC2_decode_subblocks( block, pixels, 0 );
		return;
	}

	second = ( block[0] >> 3 ) + wfETC2_complement3_table[block[0] & 7];
	if( second < 0 || second > 31 )
	{
		wfETC2_decode_t_or_h(
			block, pixels, 0, punchthrough && ( block[3] & 2 ) == 0 );
		return;
	}
	second = ( block[1] >> 3 ) + wfETC2_complement3_table[block[1] & 7];
	if( second < 0 || second > 31 )
	{
		wfETC2_decode_t_or_h(
			block, pixels, 1, punchthrough && ( block[3] & 2 ) == 0 );
		return;
	}
	second = ( block[2] >> 3 ) + wfETC2_complement3_table[block[2] & 7];
	if( second < 0 || second > 31 )
	{
		wfETC2_decode_planar( block, pixels );
		return;
	}
	wfETC2_decode_subblocks(
		block, pixels, punchthrough && ( block[3] & 2 ) == 0 );
}

static uint64_t wfETC2_read_be64( const uint8_t* data )
{
	uint64_t value = 0;
	int i;
	for( i = 0; i < 8; ++i )
		value = ( value << 8 ) | data[i];
	return value;
}

static int wfEAC_index( uint64_t block, int index )
{
	return (int)( ( block >> ( 45 - index * 3 ) ) & 7 );
}

static void wfEAC_decode_alpha_block( const uint8_t* block, uint8_t* pixels )
{
	const uint64_t bits = wfETC2_read_be64( block );
	const int base = block[0];
	const int multiplier = block[1] >> 4;
	const int8_t* modifiers = wfEAC_modifier_table[block[1] & 0x0F];
	int i;
	for( i = 0; i < 16; ++i )
	{
		const int offset = wfETC2_pixel_offset( i );
		pixels[offset * 4 + 3] = (uint8_t)wfETC2_clamp_byte(
			base + modifiers[wfEAC_index( bits, i )] * multiplier );
	}
}

static int wfEAC_decode_11( const uint8_t* block, int index, int signed_format )
{
	const uint64_t bits = wfETC2_read_be64( block );
	const int multiplier = block[1] >> 4;
	const int scale = multiplier ? multiplier * 8 : 1;
	const int modifier =
		wfEAC_modifier_table[block[1] & 0x0F][wfEAC_index( bits, index )];
	if( signed_format )
	{
		int base = (int)(int8_t)block[0];
		int value;
		if( base == -128 )
			base = -127;
		value = base * 8 + modifier * scale;
		if( value < -1023 )
			value = -1023;
		if( value > 1023 )
			value = 1023;
		return ( value + 1023 ) * 255 / 2046;
	}
	else
	{
		int value = block[0] * 8 + 4 + modifier * scale;
		if( value < 0 )
			value = 0;
		if( value > 2047 )
			value = 2047;
		return value * 255 / 2047;
	}
}

static int wfETC_format_channels( int format )
{
	switch( format )
	{
	case WF_ETC_FORMAT_ETC1_RGB8:
	case WF_ETC_FORMAT_ETC2_RGB8:
		return 3;
	case WF_ETC_FORMAT_ETC2_RGBA8:
	case WF_ETC_FORMAT_ETC2_RGB8A1:
		return 4;
	case WF_ETC_FORMAT_EAC_R11:
	case WF_ETC_FORMAT_EAC_SIGNED_R11:
		return 1;
	case WF_ETC_FORMAT_EAC_RG11:
	case WF_ETC_FORMAT_EAC_SIGNED_RG11:
		return 3;
	default:
		return 0;
	}
}

static int wfETC_format_block_size( int format )
{
	return format == WF_ETC_FORMAT_ETC2_RGBA8 ||
	       format == WF_ETC_FORMAT_EAC_RG11 ||
	       format == WF_ETC_FORMAT_EAC_SIGNED_RG11 ? 16 : 8;
}

int wfETC_DecodeImage(
	const void* WF_RESTRICT pSrc,
	void* WF_RESTRICT pDst,
	uint32_t width,
	uint32_t height,
	uint32_t encoded_width,
	uint32_t encoded_height,
	int format )
{
	const uint8_t* src = (const uint8_t*)pSrc;
	uint8_t* dst = (uint8_t*)pDst;
	const int channels = wfETC_format_channels( format );
	const int block_size = wfETC_format_block_size( format );
	const uint32_t blocks_x = encoded_width / 4;
	const uint32_t blocks_y = encoded_height / 4;
	uint32_t block_x;
	uint32_t block_y;

	if( NULL == src || NULL == dst || channels == 0 || width == 0 || height == 0 ||
	    encoded_width < width || encoded_height < height ||
	    ( encoded_width & 3 ) != 0 || ( encoded_height & 3 ) != 0 )
		return 0;

	for( block_y = 0; block_y < blocks_y; ++block_y )
	{
		for( block_x = 0; block_x < blocks_x; ++block_x )
		{
			uint8_t rgba[64];
			int i;
			for( i = 0; i < 16; ++i )
			{
				rgba[i * 4 + 0] = 0;
				rgba[i * 4 + 1] = 0;
				rgba[i * 4 + 2] = 0;
				rgba[i * 4 + 3] = 255;
			}

			if( format == WF_ETC_FORMAT_ETC1_RGB8 )
				wfETC2_decode_subblocks( src, rgba, 0 );
			else if( format == WF_ETC_FORMAT_ETC2_RGB8 )
				wfETC2_decode_rgb_block( src, rgba, 0 );
			else if( format == WF_ETC_FORMAT_ETC2_RGBA8 )
			{
				wfETC2_decode_rgb_block( src + 8, rgba, 0 );
				wfEAC_decode_alpha_block( src, rgba );
			}
			else if( format == WF_ETC_FORMAT_ETC2_RGB8A1 )
				wfETC2_decode_rgb_block( src, rgba, 1 );

			for( i = 0; i < 16; ++i )
			{
				const uint32_t x = block_x * 4 + (uint32_t)( i & 3 );
				const uint32_t y = block_y * 4 + (uint32_t)( i >> 2 );
				uint8_t* output;
				if( x >= width || y >= height )
					continue;
				output = &dst[( (size_t)y * width + x ) * channels];
				if( format == WF_ETC_FORMAT_EAC_R11 ||
				    format == WF_ETC_FORMAT_EAC_SIGNED_R11 )
				{
					output[0] = (uint8_t)wfEAC_decode_11(
						src, ( ( i & 3 ) << 2 ) | ( i >> 2 ),
						format == WF_ETC_FORMAT_EAC_SIGNED_R11 );
				}
				else if( format == WF_ETC_FORMAT_EAC_RG11 ||
				         format == WF_ETC_FORMAT_EAC_SIGNED_RG11 )
				{
					const int source_index = ( ( i & 3 ) << 2 ) | ( i >> 2 );
					const int signed_format =
						format == WF_ETC_FORMAT_EAC_SIGNED_RG11;
					output[0] = (uint8_t)wfEAC_decode_11(
						src, source_index, signed_format );
					output[1] = (uint8_t)wfEAC_decode_11(
						src + 8, source_index, signed_format );
					output[2] = 0;
				}
				else
				{
					int channel;
					for( channel = 0; channel < channels; ++channel )
						output[channel] = rgba[i * 4 + channel];
				}
			}
			src += block_size;
		}
	}
	return 1;
}
