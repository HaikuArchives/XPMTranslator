//	toXPM.cc
//	implements toXPM(), which writes out an XPM file given B_TRANSLATOR_BITMAP
//	data.

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "XPM.h"
#include "toXPM.h"
#include "ScanBitmap.h"

//	an XPM file is in the form of a variable declaration; to make some attempt
//	at declaring a variable of unique name, I append the result of time() to
//	this base string, the _nomen_ of the poet P. Ovidius Naso.
#define		XPM_NAME_SEED		"ovidius"

//	XPM pixels are represented by strings of characters.  Here's one set of
//	possibilities.
#define		XPM_CHAR_SET		"qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890!@#$^&*()-=_+|[]{};':,.<>/?`~"

//	entries into the color hash table
typedef struct
{
	char str[16];
	rgb_color color;
	int next;
}
pix_entry;

typedef struct
{
	BPositionIO *output;
	int ptSize;
	pix_entry *pixtable;
	int count;
	int width;
}
traverse_data;

uint32 hash_color(rgb_color *, int);
void traverseHook(int, void *, void *);

//	toXPM()
//	reads B_TRANSLATOR_BITMAP data from the stream "input", and prints an
//	XPM file onto "output".
status_t toXPM(BPositionIO *input, BPositionIO *output)
{
	status_t err;
	rgb_color *pixel;
	int n, i, j, ix;
	bitmap_record br;
	char buffer[10240];
	traverse_data td;

//	first get the bitmap data from the stream, into a bitmap_record data structure
//	as defined in "ScanBitmap.h"	
    
	err = scan_bitmap(input,&br);
	if (err != B_OK)
	{
      return B_ERROR;
    }
	
//	write out the header, a comment string, the variable declaration.		
	sprintf(buffer,"%s\n",XPM_HEADER);
	output->Write(buffer,strlen(buffer));
	sprintf(buffer,"/* XPM file written by E. Tomlinson's XPMTranslator, version 1.1.0 */\n");
	output->Write(buffer,strlen(buffer));
	sprintf(buffer,"static char *%s%ld[] =\n",XPM_NAME_SEED,time(NULL));
	output->Write(buffer,strlen(buffer));
	sprintf(buffer,"{\n");
	output->Write(buffer,strlen(buffer));

//	determine the "width" of the pixel, then
//	write out the value string (width height number-of-colors) characters-per-pixel.	
	n = br.ncolors;
	td.width = 0;
	do
	{
		n /= strlen(XPM_CHAR_SET);
		td.width++;
	}
	while (n);
	sprintf(buffer,"\t\"%d %d %d %d\"",br.width,br.height,br.ncolors,td.width);
	output->Write(buffer,strlen(buffer));
	
//	fill out the color hash table, at the same time writing out strings
//	representing the colors onto the output stream.
//	In this case, colors are hashed by their RGB values, folded into a
//	32-bit integer (see hash_color() below.)
	td.ptSize = 4*br.ncolors;
	td.pixtable = (pix_entry *)calloc(td.ptSize,sizeof(pix_entry));
	
	td.count = 0;
	td.output = output;
	br.ctable->TraverseInOrder(traverseHook,&td);
	
//	go through the pixel data, comparing the pixel values to the values
//	stored in the hash table and writing out the respective strings.
	pixel = br.pix;
	for (i = 0; i < br.height; i++)
	{
		sprintf(buffer,",\n\t\"");
		output->Write(buffer,strlen(buffer));
		for (j = 0; j < br.width; j++)
		{
			ix = hash_color(pixel,td.ptSize);
			if (strlen(td.pixtable[ix].str))
				while (ix != -1)
				{
					if (!memcmp(pixel,&td.pixtable[ix].color,sizeof(rgb_color)))
					{
						sprintf(buffer,"%s",td.pixtable[ix].str);
						break;
					}
					ix = td.pixtable[ix].next;
				}
			output->Write(buffer,strlen(buffer));
			pixel++;
		}
		sprintf(buffer,"\"");
		output->Write(buffer,strlen(buffer));
	}
	sprintf(buffer,"};\n");
	output->Write(buffer,strlen(buffer));
	free(td.pixtable);
	delete br.ctable;
	free(br.pix);
	return B_OK;
}

//	hash_color()
//	fold an RGBA color into an unsigned 32-bit integer; apply the multiplicative
//	hash used in fromXPM().
uint32 hash_color(rgb_color *color, int n)
{
	double f, g;
	
	uint32 t = (color->red << 24) + (color->green << 16) + (color->blue << 8) + color->alpha;
	f = modf(t*phi,&g);
	return (uint32)(f*n);
}

void traverseHook(int key, void *data, void *arg)
{
	char buffer[10240];
	int ix, last, t, j;
	traverse_data *td = (traverse_data *)arg;
	rgb_color *color = (rgb_color *)data;
	rgb_color transp = B_TRANSPARENT_32_BIT;

	sprintf(buffer,".\n\t\"");
	td->output->Write(buffer,strlen(buffer));
	ix = hash_color(color,td->ptSize);
	if (strlen(td->pixtable[ix].str))
	{
		while (td->pixtable[ix].next != -1)
			ix = td->pixtable[ix].next;
		last = ix;
		while (strlen(td->pixtable[ix].str))
		{
			ix++;
			if (ix >= td->ptSize)
				ix = 0;
		}
		td->pixtable[last].next = ix;
	}
	t = td->count;
	for (j = 0; j < td->width; j++)
	{
		td->pixtable[ix].str[j] = XPM_CHAR_SET[t % strlen(XPM_CHAR_SET)];
		t /= strlen(XPM_CHAR_SET);
	}
	td->pixtable[ix].str[j] = 0;
	td->pixtable[ix].color = *color;
	td->pixtable[ix].next = -1;
	td->output->Write(td->pixtable[ix].str,strlen(td->pixtable[ix].str));
	if (!memcmp(color,&transp,sizeof(rgb_color)))
		sprintf(buffer,"\tc\tNone\"");
	else
		sprintf(buffer,"\tc\t#%02x%02x%02x\"",color->red,color->green,color->blue);
	td->output->Write(buffer,strlen(buffer));
	td->count++;
}
