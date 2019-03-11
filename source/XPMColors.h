//	XPMColors.h

#ifndef XPM_COLORS_H
#define XPM_COLORS_H

typedef struct
{
	char *name;
	rgb_color color;
}
xpm_named_color;

extern const xpm_named_color named_color[];

#endif 