//	ScanBitmap.cc
//
//	implements scan_bitmap(), which reads an B_TRANSLATOR_BITMAP from a stream.
//	some attempt is made at covering all the possible bitmap color spaces, but only
//	B_RGBA32 has been tested at all, and that only on BeOS R3 Intel.  Endianness
//	issues will doubtless trip me up down the line.

#include "XPM.h"
#include "ScanBitmap.h"

//	scan_bitmap()
//	scans in a B_TRANSLATOR_BITMAP from "stream", and fills out a bitmap_record
//	for use by other code.
status_t scan_bitmap(BPositionIO *stream, bitmap_record *br)
{
	status_t err;
	rgb_color *pixel;
	int i, j, k;
	uint16 word;
	int32 *pixint;
	uint8 *data, *addr, *t;
	const color_map *clut;
	TranslatorBitmap bmap;
	rgb_color white = { 0xff, 0xff, 0xff, 0xff };
	rgb_color black = { 0x00, 0x00, 0x00, 0xff };

//	get the header first
	err = stream->Read(&bmap,sizeof(bmap));
	if (err <= 0)
		return B_ERROR;
	
//	confirm the "magic number"
    bmap.magic = B_BENDIAN_TO_HOST_INT32(bmap.magic);
    
	if (bmap.magic != B_TRANSLATOR_BITMAP)
		return B_ERROR;

	bmap.bounds.left = B_BENDIAN_TO_HOST_FLOAT(bmap.bounds.left);
	bmap.bounds.right = B_BENDIAN_TO_HOST_FLOAT(bmap.bounds.right);
	bmap.bounds.top = B_BENDIAN_TO_HOST_FLOAT(bmap.bounds.top);
	bmap.bounds.bottom = B_BENDIAN_TO_HOST_FLOAT(bmap.bounds.bottom);
	bmap.rowBytes = B_BENDIAN_TO_HOST_INT32(bmap.rowBytes);
	bmap.colors = (color_space)B_BENDIAN_TO_HOST_INT32(bmap.colors);
	bmap.dataSize = B_BENDIAN_TO_HOST_INT32(bmap.dataSize);

// allocate and initialize the pixel data
	data = (uint8 *)calloc(bmap.dataSize,1);
	err = stream->Read(data,bmap.dataSize);
	if (err <= 0)
		return B_ERROR;
	br->width = 1+bmap.bounds.IntegerWidth();
	br->height = 1+bmap.bounds.IntegerHeight();
	br->pix = (rgb_color *)malloc(br->width*br->height*sizeof(rgb_color));
// allocate the ctable, for the purpose of keeping track of all colors used in a particular
// bitmap.  This could get nasty for 32-bit color bitmaps with thousands of distinct colors...
	br->ctable = new UTreeDictionary(sizeof(rgb_color));
	br->ncolors = 0;
	
	addr = data;
	pixel = br->pix;
	clut = system_colors();

	for (i = 0; i < br->height; i++)
	{
		t = addr;
		for (j = 0; j < br->width; j++)
		{
//	attempt to cover all possible formats.  
			switch (bmap.colors)
			{
				case B_RGB32_LITTLE:
				case B_RGBA32_LITTLE:
					pixel->blue = *t++;
					pixel->green = *t++;
					pixel->red = *t++;
					pixel->alpha = *t++;	
					break;			
				case B_RGB32_BIG:
				case B_RGBA32_BIG:
					pixel->alpha = *t++;	
					pixel->red = *t++;
					pixel->green = *t++;
					pixel->blue = *t++;
					break;
				case B_RGB16_BIG:
					word = B_BENDIAN_TO_HOST_INT16(*((uint16*)t));
					goto rgb16biglittle;
				case B_RGB16_LITTLE:
				    word = B_LENDIAN_TO_HOST_INT16(*((uint16*)t));
                rgb16biglittle:    
					pixel->alpha = 0xff;
					pixel->red = (word & 0xf800) >> 8;
					pixel->red |= pixel->red >> 5;
					pixel->green = (word & 0x07e0) >> 3;
					pixel->green |= pixel->green >> 5;
					pixel->blue = (word & 0x1f) << 3;
					pixel->blue |= pixel->blue >> 5;
					t += 2;
					break;
                case B_RGB15_BIG:
					word = B_BENDIAN_TO_HOST_INT16(*((uint16*)t));
					goto rgb15biglittle;
				case B_RGB15_LITTLE:
				    word = B_LENDIAN_TO_HOST_INT16(*((uint16*)t));
                rgb15biglittle:    
					pixel->alpha = (word & 0x8000) ? 0xff : 0;
					pixel->red = (word & 0x7c00) >> 7;
					pixel->red |= pixel->red >> 5;
					pixel->green = (word & 0x03e0) >> 2;
					pixel->green |= pixel->green >> 5;
					pixel->blue = (word & 0x1f) << 3;
					pixel->blue |= pixel->blue >> 5;
					t += 2;
					break;
				case B_CMAP8:
					*pixel = clut->color_list[*t++];
					break;
				case B_GRAY8:
					pixel->red = *t++;
					pixel->green = pixel->red;
					pixel->blue = pixel->red;
					pixel->alpha = 0xff;
					break;
				case B_GRAY1:
					k = i % 8;
					if (*t & (1 << (7-k)))
						*pixel = white;
					else
						*pixel = black;
					if (k == 7)
						t++;
					break;
				default:
					*pixel = black;
					break;
			}
			pixint = (int32 *)pixel;
			if (!br->ctable->Find(NULL,*pixint))
			{
				br->ncolors++;
				br->ctable->Insert(pixel,*pixint);
			}
			pixel++;
		}	
		addr += bmap.rowBytes;
	}
	
	free(data);
	return B_OK;
}
