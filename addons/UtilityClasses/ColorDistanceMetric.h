/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef COLOR_DISTANCE_METRIC_H
#define	COLOR_DISTANCE_METRIC_H

#include <Locker.h>
#include <GraphicsDefs.h>

class ColorDistanceMetric {
static	float	*sqrt_table;
static	int		*sqr_table;
static	int		instance_counter;
static	BLocker	ctr_dtr_locker;

public:
		ColorDistanceMetric();
		~ColorDistanceMetric();

inline int32 find_palette_index(uint32 bgra_word,const rgb_color * inPalette, int inPaletteSize)
{
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;

	bgra32.word = bgra_word;

	float min_distance = 1000000.0;
	int32 red = bgra32.bytes[2];
	int32 green = bgra32.bytes[1];
	int32 blue = bgra32.bytes[0];

	int32 selected_index = -1;

	for (int i=0;i<inPaletteSize;i++) {
		float distance = sqrt_table[sqr_table[inPalette[i].red-red] +
									sqr_table[inPalette[i].green-green] +
									sqr_table[inPalette[i].blue-blue]];
		if (distance < min_distance) {
			selected_index = i;
			min_distance = distance;
		}
	}

	return selected_index;
}


inline	float	color_distance(rgb_color c1, rgb_color c2) {
	return sqrt_table[sqr_table[c1.red-c2.red]
						+ sqr_table[c1.green-c2.green]
						+ sqr_table[c1.blue-c2.blue]];
}

};

#endif
