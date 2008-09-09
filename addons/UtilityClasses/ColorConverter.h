/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	COLOR_CONVERTER_H
#define	COLOR_CONVERTER_H


#include <GraphicsDefs.h>

#include "RandomNumberGenerator.h"

union color_union {
	uint8 bytes[4];
	uint32 word;
};

struct hsv_color {
	float	hue;
	float	saturation;
	float	value;
	float	alpha;
};

struct cmyk_color {
	uint8	cyan;
	uint8	magenta;
	uint8	yellow;
	uint8	black;
	uint8	alpha;
};



class ColorConverter {
		rgb_color	color_as_rgb;
		cmyk_color	color_as_cmyk;
		uint32		color_as_bgra;

		RandomNumberGenerator	*random_numbers;


inline	cmyk_color	rgb_to_cmyk(rgb_color&);
inline	rgb_color	cmyk_to_rgb(cmyk_color&);
public:
		ColorConverter();
		~ColorConverter();
void		SetColor(rgb_color&);
void		SetColor(uint32 bgra_color);
void		SetColor(hsv_color&);
void		SetColor(cmyk_color&);

rgb_color	ReturnColorAsRGB() { return color_as_rgb; }
uint32		ReturnColorAsBGRA() { return color_as_bgra; }
hsv_color	ReturnColorAsHSV();
cmyk_color	ReturnColorAsCMYK() { return color_as_cmyk; }


// These are for adding randomness to color.
// parameter of 0 does not add any randomness and
// parameter of 1.0 adds full randomness.
void		JitterRed(float);
void		JitterGreen(float);
void		JitterBlue(float);
void		JitterHue(float);
void		JitterSaturation(float);
void		JitterValue(float);
};



cmyk_color ColorConverter::rgb_to_cmyk(rgb_color &c)
{
	cmyk_color cm;
	cm.cyan = 255-c.red;
	cm.magenta = 255-c.green;
	cm.yellow = 255-c.blue;
	cm.black = min_c(cm.cyan,min_c(cm.yellow,cm.magenta));

	cm.cyan -= cm.black;
	cm.magenta -= cm.black;
	cm.yellow -= cm.black;

	cm.alpha = c.alpha;

	return cm;
}


rgb_color ColorConverter::cmyk_to_rgb(cmyk_color &cm)
{
	cm.cyan += cm.black;
	cm.magenta += cm.black;
	cm.yellow += cm.black;

	rgb_color c;
	c.red = 255-cm.cyan;
	c.green = 255-cm.magenta;
	c.blue = 255-cm.yellow;
	c.alpha = cm.alpha;

	return c;
}



#endif
