#include "BMPSave.h"
#include <stdio.h>
#include <memory.h>
#include <wchar.h>
#include <malloc.h>


typedef struct {
	char id[2];
	long filesize;
	short reserved[2];
	long headersize;
	long infoSize;
	long width;
	long depth;
	short biPlanes;
	short bits;
	long biCompression;
	long biSizeImage;
	long biXPelsPerMeter;
	long biYPelsPerMeter;
	long biClrUsed;
	long biClrImportant;
} BMPHEAD;


void writeBitmap(char* filename, char** bitmap){
	//Declare an object of this type and fill the members according to your file. Use the following as a guide

	BMPHEAD bh;

	memset ((char *)&bh,0,sizeof(BMPHEAD)); /* sets everything to 0 */
	memcpy (bh.id,"BM",2);
	bh.filesize  =	0;			//calculated size of your file (see below)
	bh.headersize  = 54L;		//  (for 24 bit images)
	bh.infoSize  =  0x28L;		//  (for 24 bit images)
	bh.width     = 640;			// width in pixels of your image
	bh.depth     = 480;			// depth in pixels of your image
	bh.biPlanes  =  1;			// (for 24 bit images)
	bh.bits      = 24;			// (for 24 bit images)
	bh.biCompression = 0L;		//(no compression)
	//The number of bytes in each line of a .BMP file is always a multiple of 4 as Windows does DoubleWord Alignment on line boundaries. You need a variable bytesperline that specifies how many bytes there are in a line.

	int bytesPerLine;

	bytesPerLine = bh.width * 3;  /* (for 24 bit images) */
	/* round up to a dword boundary */
	if (bytesPerLine & 0x0003) 
	{
		bytesPerLine |= 0x0003;
		++bytesPerLine;
	}
	// Next fill in the filesize

	bh.filesize=bh.headersize+(long)bytesPerLine*(bh.depth-1);
	// Now you have a valid BMPHEAD.

	// Open your bmp file and write the header to it

	FILE * bmpfile;

	bmpfile = fopen("myimage.bmp", "wb");
	if (bmpfile == NULL)
	{
		printf("Error opening output file\n");
		/* -- close all open files and free any allocated memory -- */
		//exit (1);
		return;
	}


	char * cabezal = (char*)malloc(sizeof(char)*54);
	memset(cabezal,0,54);
	cabezal[0] = 66;
	cabezal[1] = 77;
	cabezal[2] = 54;
	cabezal[3] = 16;
	cabezal[4] = 14;
	cabezal[10] = 54;
	cabezal[14] = 40;
	cabezal[18] = 127;//128;
	cabezal[19] = 2;
	cabezal[22] = 127;//224;
	cabezal[23] = 1;
	cabezal[26] = 1;
	cabezal[28] = 24;
	cabezal[35] = 16;
	cabezal[36] = 14;
	fwrite(cabezal, 1, bh.headersize, bmpfile);



	


	//fwrite(&bh, 1, bh.headersize, bmpfile);
//	unsigned char* br = (unsigned char*)&bh;
	//Next you have to write your 24-bit image data out to disk one line at a time. Remember that you have to write the lowest line first. First we allocate a buffer that can hold one line of the image

	char *linebuf;

	linebuf = (char *) calloc(1, bytesPerLine);
	memset(linebuf, 0, bytesPerLine);
	if (linebuf == NULL)
	{
		printf ("Error allocating memory\n");
		/* -- close all open files and free any allocated memory -- */
		//exit (1);   
		return;
	}


	int line;

	for (line = bh.depth-1; line >= 0; line --)
	{
		/* fill line linebuf with the image data for that line */
			memcpy(linebuf, bitmap[line],bh.width*3);


			/* remember that the order is BGR and if width is not a multiple
			of 4 then the last few bytes may be unused
			*/
			fwrite(linebuf, 1, bytesPerLine, bmpfile);
	}

	fclose(bmpfile);

}