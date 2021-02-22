/*
 *  pcx
 *
 *   simple save/loader for pcx images 
 *
 * $Id: pcx.c,v 1.13 2003/04/09 02:08:31 jerry Exp $
 *
 * $Log: pcx.c,v $
 * Revision 1.13  2003/04/09 02:08:31  jerry
 * Hopefully fixed this so that it will output the data block on MS-DOS?
 *
 * Revision 1.12  2003/04/08 14:57:24  jerry
 * Full load and save of paletted images now.
 * Save does not properly support 24 bit images, but load does.
 *
 * Revision 1.11  2003/04/04 05:54:43  jerry
 * Much of the PCX save routines are mostly done.
 * I still need to:
 * 	write the actual scanline encoder
 * 	test saving out 24 bit and 8 bit paletted images
 *
 * Revision 1.10  2003/04/03 19:54:18  jerry
 * removed the PCX_Dump line from PCX_Load()
 * removed the test files
 *
 * Revision 1.9  2003/04/03 19:46:28  jerry
 * 8bit (paletted) and 24bit (truecolor/multiplanar) image loading complete
 *
 * Revision 1.8  2003/04/02 22:38:12  jerry
 * Test main.c
 * added in the header and 256 color palette reader
 *
 * Revision 1.7  2003/04/02 21:53:20  jerry
 * Updated a lot of stuff.
 * changed 'xxx_ValidFile' to xxx_SupportedFile
 *
 * Revision 1.6  2003/04/02 21:46:32  jerry
 * Fleshed out more of the PCX stuff
 *
 * Revision 1.5  2003/04/02 21:24:56  jerry
 * Added endian.[ch] for use with the PCX encoding and decoding.
 *
 * Revision 1.4  2003/04/01 04:15:06  jerry
 * Added info about the header
 *
 * Revision 1.3  2003/04/01 03:13:12  jerry
 * Shoved in the ZSoft source code.
 *
 * Revision 1.2  2003/03/31 17:38:21  jerry
 * updated the structure
 * changed ppmio to ppm
 * added pcx
 *
 * Revision 1.1  2003/03/31 17:31:40  jerry
 * Added pcx headers
 *
 */

/*
 * NOTE: 

	Some of the code in this .c file has been swiped from the
	"ZSoft PCX File Format Technical Reference Manual", from
	the section titled "Sample 'C' Routines".  It is my
	understanding that this provided source code is free to
	use without restriction, although I will provide this note
	and the copyright for that document in here anyway.  This
	referenced document is available upon request.

	The "ZSoft PCX File Format Technical Reference Manual" is
	Copyright c 1985, 1987, 1988, 1990, 1991, ZSoft Corporation.
	All Rights Reserved.

	Reformatted from K&R C into ANSI C by Jerry Carpenter, 2003
	(Basically just some function prototypes. meh.)
 */


/*
 * KNOWN BUGS FOR THIS IMPLEMENTATION:

	- Probably doesn't read in 1,2, or 4 bit-per-pixel images properly
	    (completely untested, but it shouldn't matter for Turaco CL)
	- only saves out single-plane type 5
	- only reads in types 2, 3, and 5
	- probably doesn't save out non-even width files properly

 * DESIGN OOPS'ES:

	- palette structure inconsistancies. oops.

 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pcx.h"


/*
 Decoding .PCX Files

	First, find the pixel dimensions of the image by calculating
	[XSIZE = Xmax - Xmin + 1] and [YSIZE = Ymax - Ymin + 1].
	Then calculate how many bytes are required to hold one
	complete uncompressed scan line:

	TotalBytes = NPlanes * BytesPerLine

	Note that since there are always an even number of bytes
	per scan line, there will probably be unused data at the
	end of each scan line.  TotalBytes shows how much storage
	must be available to decode each scan line, including any
	blank area on the right side of the image.  You can now
	begin decoding the first scan line - read the first byte
	of data from the file.  If the top two bits are set, the
	remaining six bits in the byte show how many times to
	duplicate the next byte in the file.  If the top two bits
	are not set, the first byte is the data itself, with a
	count of one.

	Continue decoding the rest of the line.  Keep a running
	subtotal of how many bytes are moved and duplicated into
	the output buffer.  When the subtotal equals TotalBytes,
	the scan line is complete.  There should always be a decoding
	break at the end of each scan line.  But there will not be
	a decoding break at the end of each plane within each scan
	line.  When the scan line is completed, there may be extra
	blank data at the end of each plane within the scan line.
	Use the XSIZE and YSIZE values to find where the valid
	image data is.  If the data is multi-plane, BytesPerLine
	shows where each plane ends within the scan line.

	Continue decoding the remainder of the scan lines (do not
	just read to end-of-file).  There may be additional data
	after the end of the image (palette, etc.)
 */

////////////////////////////////////////////////////////////////////////////////
void PCX_Image::Dump()
{
    int c, d, e;

    printf( "PCX Image:\n");
    printf( "  Manufacturer: 0x%02x (0x0a)\n", hdr.manufacturer );
    printf( "       Version: %d\n", hdr.version );
    printf( "      Encoding: %d\n", hdr.encoding );
    printf( "  BitsPerPixel: %d\n", hdr.bitsPerPixel );
    printf( "        Window: (%d %d)-(%d %d)\n", 
		    hdr.xMin, hdr.yMin, hdr.xMax, hdr.yMax );

    printf( "                %d x %d\n", width, height ); 

    printf( "         H DPI: %d\n", hdr.hDpi );
    printf( "         V DPI: %d\n", hdr.vDpi );
    printf( "      reserved: %d\n", hdr.reserved );
    printf( "      n planes: %d\n", hdr.nplanes );
    printf( "  bytesPerLine: %d\n", hdr.bytesPerLine );
    printf( "   paletteInfo: %d\n", hdr.paletteInfo );
    printf( "   hScreenSize: %d\n", hdr.hScreenSize );
    printf( "   vScreenSize: %d\n", hdr.vScreenSize );

	printf( " 16 color palette header data:\n ");
	for( c=0, d=3, e=0 ; c<48 ; c++, d++, e++ )
	{
		if( e == 12 )
		{
			d=0; e=0;
			printf( "\n    [%02x]", c/3 );
		} else if( d == 3 )
		{
			d=0;
			printf( "   [%02x]", c/3 );
		}
		printf( " %02x", hdr.colormap[c] );
	}
	printf("\n");

	printf( " 256 color palette data:\n ");
	for( c=0, d=0 ; c<256 ; c++, d++ )
	{
		if( d == 4 )
		{
			d=0; 
			printf( "\n " );
		}
		printf( "   [%02x] ", c );
		printf( "%02x %02x %02x", pal[c].r, pal[c].g, pal[c].b );
	}
	printf("\n");

	printf( "%6ld bytes\n", bufrSize );
	printf( "%6d pixels\n", width * height );
    
}

////////////////////////////////////////////////////////////////////////////////
/* 
 * PCX_encget
 *
 *  This procedure reads one encoded block from the image 
 *  file and stores a count and data byte.
 */
int PCX_Image::encget(int * pbyt, int * pcnt, FILE * fid)
{
    int i;
    *pcnt = 1;        /* assume a "run" length of one */

    /* check for EOF */
    if (EOF == (i = getc(fid)))
	return (EOF);

    if (0xC0 == (0xC0 & i)) /* is it a RLE repeater */
    {
	/* YES.  set the repeat count */
	*pcnt = 0x3F & i;

	/* doublecheck the next byte for EOF */
	if (EOF == (i = getc(fid)))
	    return (EOF);
    }
    /* set the byte */
    *pbyt = i;

    /* return an 'OK' */
    return( 0 );
}


unsigned char PCX_Image::endian_LittleRead8(FILE * f)
{
	unsigned char ret = 0;
	fread(&ret, sizeof(ret), 1, f);
	return ret;
}

unsigned short PCX_Image::endian_LittleRead16(FILE * f)
{
	unsigned short ret = 0;
	fread(&ret, sizeof(ret), 1, f);
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
int PCX_Image::LoadHeader(FILE * fp)
{
    int c;
    if(!fp )  
		return( -1 );

    /* go to the beginning of the file */
    rewind( fp );

    /* read in the header blocks. */
    hdr.manufacturer = endian_LittleRead8( fp );
    hdr.version      = endian_LittleRead8( fp );
    hdr.encoding     = endian_LittleRead8( fp );
    hdr.bitsPerPixel = endian_LittleRead8( fp );

    hdr.xMin = endian_LittleRead16( fp );
    hdr.yMin = endian_LittleRead16( fp );
    hdr.xMax = endian_LittleRead16( fp );
    hdr.yMax = endian_LittleRead16( fp );

    /* do a little precomputing... */
    width  = hdr.xMax - hdr.xMin + 1;
    height = hdr.yMax - hdr.yMin + 1;

    hdr.hDpi = endian_LittleRead16( fp );
    hdr.vDpi = endian_LittleRead16( fp );

    /* read in 16 color colormap */
    for( c=0 ; c<48 ; c++ )
    {
	hdr.colormap[c] = endian_LittleRead8( fp );
    }

    hdr.reserved     = endian_LittleRead8( fp );
    hdr.nplanes      = endian_LittleRead8( fp );
    hdr.bytesPerLine = endian_LittleRead16( fp );
    hdr.paletteInfo  = endian_LittleRead16( fp );
    hdr.hScreenSize  = endian_LittleRead16( fp );
    hdr.vScreenSize  = endian_LittleRead16( fp );

    /* ignore the filler */

    return( 0 );
}

int PCX_defaultPalette[48] = {
	0x00, 0x00, 0x00,    0x00, 0x00, 0x80,    0x00, 0x80, 0x00,
	0x00, 0x80, 0x80,    0x80, 0x00, 0x00,    0x80, 0x00, 0x80,
	0x80, 0x80, 0x00,    0x80, 0x80, 0x80,    0xc0, 0xc0, 0xc0,
	0x00, 0x00, 0xff,    0x00, 0xff, 0x00,    0x00, 0xff, 0xff,
	0xff, 0x00, 0x00,    0xff, 0x00, 0xff,    0xff, 0xff, 0x00,
	0xff, 0xff, 0xff 
};

int PCX_Image::LoadPalette(FILE * fp)
{
	int c;
	int checkbyte;

	if( !fp ) 
		return( -1 );

	if( hdr.version == 3 )
	{
		/* copy over the default palette to the header */
		for( c=0 ; c<48 ; c++ )
			hdr.colormap[c] = PCX_defaultPalette[c];

		/* copy over the default palette to the palette structure */
		for( c=0 ; c<16 ; c++ )
		{
			pal[c].r = PCX_defaultPalette[c*3 + 0];
			pal[c].g = PCX_defaultPalette[c*3 + 1];
			pal[c].b = PCX_defaultPalette[c*3 + 2];
		}

		return( 0 );
	}

	if( hdr.bitsPerPixel != 8 )
	{
		/* no palette in the image, just return */
		return( 0 );
	}

	/* first seek to the end of the file -769 */
	fseek( fp, -769, SEEK_END );
	checkbyte = endian_LittleRead8( fp );

	if( checkbyte != 12 ) /* magic value */
	{
		printf("Expected a 256 color palette, didn't find it.\n");
		return( -1 );
	}

	/* okay. so we're at the right part of the file now, we just need
	to populate our palette! */
	for( c=0 ; c<256 ; c++ )
	{
		pal[c].r = endian_LittleRead8( fp );
		pal[c].g = endian_LittleRead8( fp );
		pal[c].b = endian_LittleRead8( fp );
	}

	/* now copy over the first 16 colors into the header */
	for( c=0 ; c<16 ; c++ )
	{
		hdr.colormap[c*3 + 0] = pal[c].r;
		hdr.colormap[c*3 + 1] = pal[c].g;
		hdr.colormap[c*3 + 2] = pal[c].b;
	}

	PaletteLoaded = true;

	return( 0 );
}


int PCX_Image::LoadDecodeData(FILE * fp)
{
    int i;
    long l;
    int chr, cnt;
    unsigned char * bpos;

    if(!fp )  
		return( -1 );

    /* first, we need to go to the right point in the file */
    fseek( fp, 128, SEEK_SET ); /* oh yeah. this is important */

    /* Here's a program fragment using PCX_encget.  This reads an 
	entire file and stores it in a (large) buffer, pointed 
	to by the variable "bufr". "fp" is the file pointer for 
	the image */

    bufrSize = (long )  hdr.bytesPerLine * hdr.nplanes * (1 + hdr.yMax - hdr.yMin);

    if(bufr)
		free(bufr);

    bufr = (unsigned char *) malloc(sizeof( unsigned char ) * bufrSize );
    bpos = bufr;

    for (l = 0; l < bufrSize; )  /* increment by cnt below */
    {
		if (EOF == encget(&chr, &cnt, fp))
			break;

		for (i = 0; i < cnt && l+i < bufrSize; i++)
			*bpos++ = chr;

		l += cnt;
    }

    return 0;
}

bool PCX_Image::Load(const char * filename)
{
    FILE * in_fp;

	if(filename == nullptr)
    {
		printf("Unable to open PCX: No filename.\n");
		return false;
    }

    fopen_s(&in_fp, filename, "r");
    if(in_fp == nullptr)
    {
		printf ("%s: Unable to open file\n", filename);
		return false;
    }

    if(bufr != nullptr)
		delete bufr;

	/* make sure this is nullptr'ed */
    bufr = nullptr;

    LoadHeader(in_fp);
    LoadPalette(in_fp);
    LoadDecodeData(in_fp);
    fclose( in_fp );

    return true;
}

