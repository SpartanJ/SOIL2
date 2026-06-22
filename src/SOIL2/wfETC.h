#ifndef WF_ETC_H
#define WF_ETC_H

#define WF_LITTLE_ENDIAN

// Qt ordering
#define WF_ETC_RGBA( r, g, b, a ) ( ((a)<<24) | ((r)<<16) | ((g)<<8) | (b) ) // colors have been clamped by the time we hit here, so there should be no need to mask

#ifdef _MSC_VER
	#if _MSC_VER < 1300
	   typedef signed   char  int8_t;
	   typedef unsigned char  uint8_t;
	   typedef signed   short int16_t;
	   typedef unsigned short uint16_t;
	   typedef signed   int   int32_t;
	   typedef unsigned int   uint32_t;
	#else
	   typedef signed   __int8  int8_t;
	   typedef unsigned __int8  uint8_t;
	   typedef signed   __int16 int16_t;
	   typedef unsigned __int16 uint16_t;
	   typedef signed   __int32 int32_t;
	   typedef unsigned __int32 uint32_t;
	#endif
	typedef signed   __int64 int64_t;
	typedef unsigned __int64 uint64_t;
#else
	#include <stdint.h>
#endif

#ifndef WF_RESTRICT
	#if defined MSC_VER
		#define WF_RESTRICT __restrict
	#elif defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
		#define WF_RESTRICT restrict
	#else
		#define WF_RESTRICT
	#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void wfETC1_DecodeBlock( const void* WF_RESTRICT src, void* WF_RESTRICT dst, const uint32_t dstStride /*=4*/ ); //!< stride in pixels; must be a multiple of four

extern void wfETC1_DecodeImage( const void* WF_RESTRICT src, void* WF_RESTRICT dst, const uint32_t width, const uint32_t height ); //!< width/height in pixels; must be multiples of four

enum
{
	WF_ETC_FORMAT_ETC1_RGB8 = 0,
	WF_ETC_FORMAT_ETC2_RGB8 = 1,
	WF_ETC_FORMAT_ETC2_RGBA8 = 2,
	WF_ETC_FORMAT_ETC2_RGB8A1 = 3,
	WF_ETC_FORMAT_EAC_R11 = 4,
	WF_ETC_FORMAT_EAC_SIGNED_R11 = 5,
	WF_ETC_FORMAT_EAC_RG11 = 6,
	WF_ETC_FORMAT_EAC_SIGNED_RG11 = 7
};

/**
 * Decode a PKM-compatible ETC/EAC image into tightly packed 8-bit components.
 * Signed EAC values are mapped from [-1, 1] to [0, 255].
 * EAC RG11 is returned as RGB with a zero blue component because stb_image's
 * two-component convention means luminance-alpha rather than red-green.
 * Returns non-zero on success.
 */
extern int wfETC_DecodeImage(
	const void* WF_RESTRICT src,
	void* WF_RESTRICT dst,
	uint32_t width,
	uint32_t height,
	uint32_t encoded_width,
	uint32_t encoded_height,
	int format );

#ifdef __cplusplus
}
#endif

#endif // WF_ETC_H
