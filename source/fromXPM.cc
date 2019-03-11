//	fromXPM.cc
//
//	implements fromXPM(), which accomplishes the task of scanning
//	XPM image format into a B_TRANSLATOR_BITMAP.  To speed the
//	business of translation, only the double-quoted strings in the
//	XPM file are examined; the 'static char *' declarations and
//	other 'C' language trappings are ignored.

#include "XPM.h"
#include "fromXPM.h"
#include "XPMScanner.h"
#include "XPMColors.h"

//	XPM color space constants, in order of increasing precedence
enum
{
	XPM_MONO,
	XPM_GREY4,
	XPM_GREY,
	XPM_COLOR
};

//	entry in that XPM color hash table
typedef struct
{
	char string[16];
	rgb_color color;
	int next;
}
xpm_clut_entry;

//	Necessary XPM information--header info and color hash table
typedef struct
{
	int width;
	int height;
	int ncolors;
	int pixwidth;
	int xhotspot;
	int yhotspot;
	bool extFlag;
	int clutSize;
	xpm_clut_entry *clut;
}
xpm_info;

status_t handle_value_string(char *, xpm_info *);
status_t handle_color_string(char *, rgb_color *);
status_t handle_hex_color(char *, uint8 *);
uint32 hash_pix_string(char *, xpm_info *);

//	fromXPM()
//	accepts XPM file in "input" stream, and, if all goes well,
//	outputs the B_TRANSLATOR_DATA, in the color space B_RGBA32,
//	into the "output" stream.
status_t fromXPM(BPositionIO *input, BPositionIO *output)
{
	status_t err;
	char pixstr[16];
	char string[10240];
	xpm_info xpmInfo;
	uint8 *data, *t;
	TranslatorBitmap bmap;
	int size, i, j, k, last, length;
	XPMScanner scanner(input);
	
	err = scanner.Setup();
	if (err != B_OK)
		return B_ERROR;

//	first string:  XPM width, height, number of colors, characters-per-pixel
//	populate TranslatorBitmap header, ensuring big-endianness
	err = scanner.GetString(string);
	if (err != B_OK)
		return B_ERROR;
	err = handle_value_string(string,&xpmInfo);
	if (err != B_OK || xpmInfo.width < 0 || xpmInfo.height < 0 || xpmInfo.pixwidth <= 0)
		return B_ERROR;
	bmap.magic = B_TRANSLATOR_BITMAP;
	swap_data(B_INT32_TYPE,&bmap.magic,sizeof(bmap.magic),B_SWAP_HOST_TO_BENDIAN);
	bmap.bounds.Set(0,0,xpmInfo.width-1,xpmInfo.height-1);
	swap_data(B_RECT_TYPE,&bmap.bounds,sizeof(bmap.bounds),B_SWAP_HOST_TO_BENDIAN);
	bmap.rowBytes = 4*xpmInfo.width;
	swap_data(B_INT32_TYPE,&bmap.rowBytes,sizeof(bmap.rowBytes),B_SWAP_HOST_TO_BENDIAN);
	bmap.colors = B_RGB_32_BIT;
	swap_data(B_INT32_TYPE,&bmap.colors,sizeof(bmap.colors),B_SWAP_HOST_TO_BENDIAN);
	size = 4*xpmInfo.width;
	bmap.dataSize = 4*xpmInfo.width*xpmInfo.height;
	swap_data(B_INT32_TYPE,&bmap.dataSize,sizeof(bmap.dataSize),B_SWAP_HOST_TO_BENDIAN);

//	allocate bytes for pixel data--four bytes for each of width*height pixels
//	this pixel data is all initialized to zero, so that any missing or corrupt
//	pixel data will result in _black_ pixels in the resulting bitmap.
	data = (uint8 *)malloc(size);

//	allocate and initialize color hash table.
//	multiplicative hash (see hash_string() below), coalesced chaining.
//	an XPM file represents pixels by fixed-width strings of ASCII characters.
//	associated either with RGB values or with colors specified by the X color
//	named defined in "rgb.txt".	
	xpmInfo.clutSize = 4*xpmInfo.ncolors;
	xpmInfo.clut = (xpm_clut_entry *)calloc(xpmInfo.clutSize,sizeof(xpm_clut_entry));
	for (i = 0; i < xpmInfo.ncolors; i++)
	{
		err = scanner.GetString(string);
		if (err != B_OK)
			goto bail;
		if (strlen(string) < xpmInfo.pixwidth)
			continue;
		strncpy(pixstr,string,xpmInfo.pixwidth);
		j = hash_pix_string(pixstr,&xpmInfo);
		if (strlen(xpmInfo.clut[j].string))
		{
			while (xpmInfo.clut[j].next != -1)
				j = xpmInfo.clut[j].next;
			last = j;
			while (strlen(xpmInfo.clut[j].string))
			{
				j++;
				if (j >= xpmInfo.clutSize)
					j = 0;
			}
			xpmInfo.clut[last].next = j;
		}
		handle_color_string(&string[xpmInfo.pixwidth],&xpmInfo.clut[j].color);
		strncpy(xpmInfo.clut[j].string,pixstr,xpmInfo.pixwidth);
		xpmInfo.clut[j].next = -1;
	}	

//	scan the strings of XPM pixel data, comparing groups of pixels to the
//	strings stored in the color hash table.  Store the pixel values in
//	BGRA order, as specified by the B_RGBA32 color space.
bail:
	output->Write(&bmap,sizeof(bmap));
	for (i = 0; i < xpmInfo.height; i++)
	{
		err = scanner.GetString(string);
		memset(data,0,size);
		if (err == B_OK)
		{
			length = strlen(string);
			t = data;
			for (k = 0; k < length; k += xpmInfo.pixwidth)
			{
				j = hash_pix_string(&string[k],&xpmInfo);
				if (strlen(xpmInfo.clut[j].string))
				{
					do
					{
						if (!strncmp(xpmInfo.clut[j].string,&string[k],xpmInfo.pixwidth))
							break;
						j = xpmInfo.clut[j].next;
					}
					while (j != -1);
					if (j == -1)
						t += 4;
					else
					{
						*t++ = xpmInfo.clut[j].color.blue;
						*t++ = xpmInfo.clut[j].color.green;
						*t++ = xpmInfo.clut[j].color.red;
						*t++ = xpmInfo.clut[j].color.alpha;
					}
				}
			}
		}
		output->Write(data,size);
	}

//	Write out the header and pixel data; free allocated data structures
	free(xpmInfo.clut);
	free(data);
	return B_OK;
}

//	handle_value_string()
//	tokenize the first "value string" of an XPM file, which has
//	the following necessary fields:
//
//	width (integer)
//	height (integer)
//	number of colors (integer)
//	characters per pixel (integer)
//
//	And the following optional fields:
//
//	x-coordinate of "hot spot" (integer)  (here ignored)
//	y-coordinate of "hot spot" (integer)  (here ignored)
//
//	The last token in the value string may be the substring "XPMEXT"
//	denoting the presence, at the end of the file, of extra comments
//	or other extra data, here ignored.
status_t handle_value_string(char *string, xpm_info *info)
{
	char *token;
	int n;
	
	info->extFlag = false;		
	token = strtok(string," \t");
	if (!token)
		return B_ERROR;
	n = sscanf(token,"%d",&info->width);
	if (!n)
		return B_ERROR;
	token = strtok(NULL," \t");
	if (!token)
		return B_ERROR;
	n = sscanf(token,"%d",&info->height);
	if (!n)
		return B_ERROR;
	token = strtok(NULL," \t");
	if (!token)
		return B_ERROR;
	n = sscanf(token,"%d",&info->ncolors);
	if (!n)
		return B_ERROR;
	token = strtok(NULL," \t");
	if (!token)
		return B_ERROR;
	n = sscanf(token,"%d",&info->pixwidth);
	if (!n)
		return B_ERROR;
	token = strtok(NULL," \t");
	if (!token)
		return B_OK;
	else if (!strcmp("XPMEXT",token))
		info->extFlag = true;
	else
	{
		n = sscanf(token,"%d",&info->xhotspot);
		if (!n)
			return B_ERROR;
	}
	token = strtok(NULL," \t");
	if (!token)
	{
		if (info->extFlag)
			return B_OK;
		else
			return B_ERROR;
	}
	else if (info->extFlag)
		return B_ERROR;
	n = sscanf(token,"%d",&info->yhotspot);
	if (!n)
		return B_ERROR;
	token = strtok(NULL," \t");
	if (!token)
		return B_OK;
	else if (!strcmp("XPMEXT",token))
		info->extFlag = true;
	else
		return B_ERROR;
	token = strtok(NULL," \t");
	if (token)
		return B_ERROR;
		
	return B_OK;
}

//	handle_color_string()
//	tokenize a string representing the color of an XPM pixel.
//	This string consists of pairs of tokens, a "type" token and a
//	"value" token.  The "type" may be one of the following:
//
//	"s" - symbolic color (here ignored)
//	"m" - monochrome
//	"g4" - 4-level greyscale
//	"g" - greyscale
//	"c" - color
//
//	pixel values may be represented either as strings, denoting
//	the names of X colors (e. g. "LightSlateGray", "LightSalmon1")
//	or RGB colors, which are hexadecimal strings prefixed with a
//	hash mark "#".  RGB colors may be any size at all.  The special
//	value "none" or "None" denotes a transparent pixel.
status_t handle_color_string(char *string, rgb_color *color)
{
	char *type;
	char *str;
	char *value;
	char hex[16];
	status_t err;
	bool done = false;
	bool gotColor = false;
	bool first = true;
	int i, length, width, depth, maxdepth = -1;

	while (!done)
	{
		if (first)
		{
			first = false;
			str = string;
		}
		else
			str = NULL;
		type = strtok(str," \t");
		value = strtok(NULL," \t");
		if (!type || !value)
		{
			done = true;
			continue;
		}
		if (!strcmp("s",type))
			continue;
		if (!strcmp("m",type))
			depth = XPM_MONO;
		else if (!strcmp("g4",type))
			depth = XPM_GREY4;
		else if (!strcmp("g",type))
			depth = XPM_GREY;
		else if (!strcmp("c",type))
			depth = XPM_COLOR;
		else
			continue;
		if (depth > maxdepth)
		{
			if (value[0] == '#')
			{
				length = strlen(&value[1]);
//	make sure the string can be divided into three equal parts
				if (length % 3)
					continue;
				width = length / 3;
				strncpy(hex,&value[1],width);
				hex[width] = 0;
				err = handle_hex_color(hex,&color->red);
				if (err != B_OK)
					continue;
				strncpy(hex,&value[1+width],width);
				hex[width] = 0;
				err = handle_hex_color(hex,&color->green);
				if (err != B_OK)
					continue;
				strncpy(hex,&value[1+2*width],width);
				hex[width] = 0;
				err = handle_hex_color(hex,&color->blue);
				if (err != B_OK)
					continue;
				color->alpha = 0xff;
				gotColor = true;
			}
//	check for the "none" value...
			else if (!strcmp("none",value) || !strcmp("None",value))
			{
				*color = B_TRANSPARENT_32_BIT;
				gotColor = true;
			}
			else
			{
				i = 0;
//	...otherwise search the list of "named" colors.
				while (named_color[i].name)
				{
					if (!strcmp(named_color[i].name,value))
					{
						*color = named_color[i].color;
						gotColor = true;
						break;
					}
					else
						i++;
				}
			}
		}
	}

//	invalid colors are reset to all-black.
	if (!gotColor)
	{
		memset(color,0,sizeof(rgb_color));
		return B_ERROR;
	}
	else
		return B_OK;
}

//	handle_hex_color()
//	parse the hexadecimal representation of an RGB color
status_t handle_hex_color(char *string, uint8 *foo)
{
	int n, x, w;
	
	n = sscanf(string,"%x%n",&x,&w);
	if (!n || w != strlen(string))
		return B_ERROR;
	*foo = (uint8)(256.0 * (x / (float)(1L << 4*w)));
	
	return B_OK;
}

//	hash_pix_string()
//	to hash a string, collapse the string into a 32-bit unsigned integer.
//	then multiply by the inverse of the golden ratio, extract the
//	fractional part, multiply by the size of the hash table and truncate
//	to obtain the index into the table.
uint32 hash_pix_string(char *string, xpm_info *xpmInfo)
{
	int j;
	uint32 i = 0;
	double f, g;
	
	for (j = 0; j < xpmInfo->pixwidth; j++)
	{
		i <<= 8;
		i += string[j];
	}
	
	f = modf(i*phi,&g);
	return (uint32)f*xpmInfo->clutSize;
}
