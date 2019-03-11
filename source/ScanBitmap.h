//	ScanBitmap.h
#ifndef SCANBITMAP_H
#define SCANBITMAP_H

#include "UTreeDictionary.h"

//	stores data needed for representing a bitmap.  I might simply
//	have used the "TranslatorBitmap" structure but this is a bit
//	more complete, storing a color table as well in the hash table
//	"ctable".
typedef struct
{
	int width;
	int height;
	UTreeDictionary *ctable;
	int ncolors;
	rgb_color *pix;
}
bitmap_record;

status_t scan_bitmap(BPositionIO *, bitmap_record *);

#endif