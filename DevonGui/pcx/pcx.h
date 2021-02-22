/*
 *  pcx
 *
 *   simple save/loader for pcx images
 *
 * $Id: pcx.h,v 1.7 2003/04/08 14:57:24 jerry Exp $
 *
 * $Log: pcx.h,v $
 * Revision 1.7  2003/04/08 14:57:24  jerry
 * Full load and save of paletted images now.
 * Save does not properly support 24 bit images, but load does.
 *
 * Revision 1.6  2003/04/04 05:54:43  jerry
 * Much of the PCX save routines are mostly done.
 * I still need to:
 * 	write the actual scanline encoder
 * 	test saving out 24 bit and 8 bit paletted images
 *
 * Revision 1.5  2003/04/03 19:46:28  jerry
 * 8bit (paletted) and 24bit (truecolor/multiplanar) image loading complete
 *
 * Revision 1.4  2003/04/02 22:38:12  jerry
 * Test main.c
 * added in the header and 256 color palette reader
 *
 * Revision 1.3  2003/04/02 21:53:20  jerry
 * Updated a lot of stuff.
 * changed 'xxx_ValidFile' to xxx_SupportedFile
 *
 * Revision 1.2  2003/04/02 21:46:32  jerry
 * Fleshed out more of the PCX stuff
 *
 * Revision 1.1  2003/03/31 17:31:40  jerry
 * Added pcx headers
 *
 *
 */

#ifndef __PCX_H__
#define __PCX_H__

#define pcxu8      unsigned char
#define pcxu16     unsigned int

typedef struct{
    pcxu8  manufacturer;   /* 0x0A: ZSoft */
    pcxu8  version;
                        /* 0x02 with palette
                           0x03 without palette
                           0x05 24 bit pcx or paletted
                         */

    pcxu8  encoding;       /* 0x01: PCX RLE */
    pcxu8  bitsPerPixel;   /* number of bits per pixel 1, 2, 4, 8 */
    pcxu16 xMin;           /* window */
    pcxu16 yMin;
    pcxu16 xMax;
    pcxu16 yMax;
    pcxu16 hDpi;
    pcxu16 vDpi;
    pcxu8  colormap[48];   /* first 16 colors, R, G, B as 8 bit values */
    pcxu8  reserved;       /* 0x00 */
    pcxu8  nplanes;        /* number of planes */
    pcxu16 bytesPerLine;   /* number of bytes that represent a scanline plane
                           == must be an even number! */
    pcxu16 paletteInfo;    /* 0x01 = color/bw
                           0x02 = grayscale
                         */
    pcxu16 hScreenSize;    /* horizontal screen size */
    pcxu16 vScreenSize;    /* vertical screen size */
    pcxu8  filler[54];     /* padding to 128 bytes */
} PCX_Header;

typedef struct
{
	pcxu8 r;
	pcxu8 g;
	pcxu8 b;
	pcxu8 a;
} PCX_Color;

struct PCX_Image
{
    PCX_Header hdr;     /* the above header */
    PCX_Color pal[256];     /* the palette at the end of the file (if applicable) */

    unsigned char * bufr = nullptr;	/* the image data */
    long bufrSize;

    int width;
    int height;

	bool PaletteLoaded = false;

	~PCX_Image()
	{
		if(bufr != nullptr)
			delete bufr;
	}

	int encget( int * pbyt, int * pcnt, FILE * fid );
	int LoadHeader(FILE * fp);
	int LoadDecodeData(FILE * fp);
	int LoadPalette(FILE * fp);
	bool Load(const char * filename);
	void Dump();

private:
	unsigned char endian_LittleRead8(FILE * f);
	unsigned short endian_LittleRead16(FILE * f);
};


#endif
