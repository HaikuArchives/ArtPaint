/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "ColorConverter.h"


ColorConverter::ColorConverter()
{
	random_numbers = new RandomNumberGenerator(1235179, 1025);
}


ColorConverter::~ColorConverter()
{
	delete random_numbers;
}


void
ColorConverter::SetColor(rgb_color& c)
{
	color_as_rgb = c;
	color_as_cmyk = rgb_to_cmyk(c);

	color_union u;
	u.bytes[0] = c.blue;
	u.bytes[1] = c.green;
	u.bytes[2] = c.red;
	u.bytes[3] = c.alpha;

	color_as_bgra = u.word;
}


void
ColorConverter::SetColor(cmyk_color& c)
{
	color_as_cmyk = c;
	color_as_rgb = cmyk_to_rgb(c);

	color_union u;
	u.bytes[0] = color_as_rgb.blue;
	u.bytes[1] = color_as_rgb.green;
	u.bytes[2] = color_as_rgb.red;
	u.bytes[3] = color_as_rgb.alpha;

	color_as_bgra = u.word;
}


void
ColorConverter::SetColor(uint32 bgra_color)
{
	color_as_bgra = bgra_color;

	color_union u;
	u.word = bgra_color;

	color_as_rgb.red = u.bytes[2];
	color_as_rgb.green = u.bytes[1];
	color_as_rgb.blue = u.bytes[0];
	color_as_rgb.alpha = u.bytes[3];

	color_as_cmyk = rgb_to_cmyk(color_as_rgb);
}
