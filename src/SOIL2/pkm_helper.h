#ifndef PKM_HELPER_H
#define PKM_HELPER_H

typedef struct {
	char aName[6];
	unsigned short iBlank;
	unsigned char iPaddedWidthMSB;
	unsigned char iPaddedWidthLSB;
	unsigned char iPaddedHeightMSB;
	unsigned char iPaddedHeightLSB;
	unsigned char iWidthMSB;
	unsigned char iWidthLSB;
	unsigned char iHeightMSB;
	unsigned char iHeightLSB;
} PKMHeader;

#define PKM_HEADER_SIZE 16

enum
{
	PKM_FORMAT_ETC1_RGB8 = 0,
	PKM_FORMAT_ETC2_RGB8 = 1,
	PKM_FORMAT_ETC2_RGBA8_OLD = 2,
	PKM_FORMAT_ETC2_RGBA8 = 3,
	PKM_FORMAT_ETC2_RGB8A1 = 4,
	PKM_FORMAT_EAC_R11 = 5,
	PKM_FORMAT_EAC_RG11 = 6,
	PKM_FORMAT_EAC_SIGNED_R11 = 7,
	PKM_FORMAT_EAC_SIGNED_RG11 = 8
};

#endif
