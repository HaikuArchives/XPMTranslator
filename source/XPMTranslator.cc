//	XPMTranslator.cc
//	implements the constants and functions necessary for a Datatypes
//	addon.

#include <stdio.h>
#include <string.h>
#include "XPM.h"
#include "toXPM.h"
#include "fromXPM.h"
#include <TranslatorAddOn.h>

//	completely arbitrary of course
#define		XPM_TYPE_CODE		'XPM '

//	necessary
char translatorName[] = "XPMTranslator x86";
char translatorInfo[] = "XPMTranslator x86 version 1.1.1, by C.B.Larsen, original by E. Tomlinson)";
int32 translatorVersion = 111;

//  optional.  Covers both read and write translation.
translation_format inputFormats[] =
{
	{
		XPM_TYPE_CODE,
		B_TRANSLATOR_BITMAP,
		0.1,
		0.1,
		"image/x-xpixmap",
		"XPM image"
	},
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.1,
		0.1,
		"image/x-be-bitmap",
		"Be Bitmap Format"
	},
	{ 0, 0, 0, 0, 0, 0 }
};

translation_format outputFormats[] =
{
	{
		B_TRANSLATOR_BITMAP,
		B_TRANSLATOR_BITMAP,
		0.1,
		0.1,
		"image/x-be-bitmap",
		"Be Bitmap Format"
	},
	{
		XPM_TYPE_CODE,
		B_TRANSLATOR_BITMAP,
		0.1,
		0.1,
		"image/x-xpixmap",
		"XPM image"
	},
	{ 0, 0, 0, 0, 0, 0 }
};

uint32 get_stream_type(BPositionIO *);

//	Identify()
//	check the identity of the input stream, and see if a match with the
//	output capabilities of the translator exists.  The legitimate possibilities
//	are:
//	input = B_TRANSLATOR_BITMAP, output = XPM_TYPE_CODE or nothing -> output XPM data
//	input = XPM_TYPE_CODE, output = B_TRANSLATOR_BITMAP or nothing -> output B_TRANSLATOR_BITMAP

status_t Identify(BPositionIO *stream, \
	const translation_format *format, \
	BMessage *extension, \
	translator_info *info, \
	uint32 type)
{
	uint32 ourType;
	
	ourType = get_stream_type(stream);
	if (ourType == XPM_TYPE_CODE && (!type || type == B_TRANSLATOR_BITMAP))
	{
		info->type = inputFormats[0].type;
		info->group = inputFormats[0].group;
		info->quality = inputFormats[0].quality;
		info->capability = inputFormats[0].capability;
		strcpy(info->name,inputFormats[0].name);
		strcpy(info->MIME,inputFormats[0].MIME);
		return B_OK;
	}
	else if (ourType == B_TRANSLATOR_BITMAP && (!type || type == XPM_TYPE_CODE))
	{
		info->type = inputFormats[1].type;
		info->group = inputFormats[1].group;
		info->quality = inputFormats[1].quality;
		info->capability = inputFormats[1].capability;
		strcpy(info->name,inputFormats[1].name);
		strcpy(info->MIME,inputFormats[1].MIME);
		return B_OK;
	}
	else
	{
		return B_NO_TRANSLATOR;
	}
}

//	Translate()
//	do the same identification job as above, and execute the translation
//	with either toXPM() or fromXPM(), as necessary.
status_t Translate(BPositionIO *input, \
	const translator_info *info, \
	BMessage *extension, \
	uint32 type, \
	BPositionIO *output)
{
	uint32 ourType;
	
	ourType = get_stream_type(input);
	if (ourType == XPM_TYPE_CODE && (!type || type == B_TRANSLATOR_BITMAP))
		return fromXPM(input,output);
	else if (ourType == B_TRANSLATOR_BITMAP && (!type || type == XPM_TYPE_CODE))
		return toXPM(input,output);
	else
		return B_NO_TRANSLATOR;
}

//	get_stream_type()
//	accomplish the job of stream identification.  A B_TRANSLATOR_BITMAP is
//	indicated by the first byte, which must equal the "magic" value of
//	'bits'.  An XPM file stream must begin with the XPM header, "/* XPM */".
//	Otherwise return zero, to indicate unknown type.
uint32 get_stream_type(BPositionIO *stream)
{
	status_t err;
	char buffer[17];
	uint32 bitsType = B_HOST_TO_BENDIAN_INT32(B_TRANSLATOR_BITMAP);
	
	err = stream->Read(buffer,sizeof(buffer) - 1);
	if (err <= 0)
	{
      return 0;
    }
	stream->Seek(-err,SEEK_CUR);
	if (!strncmp(buffer,XPM_HEADER,strlen(XPM_HEADER)))
		return XPM_TYPE_CODE;
	else if (!memcmp(buffer,&bitsType,sizeof(bitsType)))
		return B_TRANSLATOR_BITMAP;
	else
	{
		buffer[16] = 0;
		return 0;
    }
}
