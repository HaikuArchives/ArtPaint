/*
 * Copyright 2026, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "Gradient.h"

#include "PixelOperations.h"

#include <Autolock.h>

#include <iostream>


int stopCompare(const ColorStop* a, const ColorStop* b)
{
	const ColorStop* colorA = a;
	const ColorStop* colorB = b;

	return colorA->pos > colorB->pos;
}


Gradient::Gradient()
{
	for (int i=0; i < 1024; ++i)
		lut[i] = 0;

	resolution = 1024;

	fStopLocker = new BLocker();
}


Gradient::~Gradient()
{
	clearStops();
}



void
Gradient::addStop(float pos, uint32 color)
{
	BAutolock stopLock(fStopLocker);

	ColorStop* stop = new ColorStop{pos, color};

	stops.AddItem(stop);

	stops.SortItems(stopCompare);

	recalculateLUT();
}


void
Gradient::insertStop(float pos, uint32 color, int index)
{
	BAutolock stopLock(fStopLocker);

	ColorStop* stop = new ColorStop{pos, color};

	stops.AddItem(stop, index);

	recalculateLUT();
}


void
Gradient::deleteStop(int index)
{
	BAutolock stopLock(fStopLocker);

	stops.RemoveItemAt(index);
}


void
Gradient::clearStops()
{
	BAutolock stopLock(fStopLocker);

	while (stops.CountItems() > 0) {
		ColorStop* stop = stops.ItemAt(0);
		stops.RemoveItemAt(0);
		delete stop;
	}

	stops.MakeEmpty();
}


void
Gradient::recalculateLUT()
{
	BAutolock stopLock(fStopLocker);

	uint32 numColors = stops.CountItems(); //stops.size();

	union color_conversion start, end, curr;
	int index = 0;

	for (uint32 i = 0; i < numColors - 1; ++i) {
		ColorStop* color1 = (ColorStop*)stops.ItemAt(i);
		ColorStop* color2 = (ColorStop*)stops.ItemAt(i + 1);
		float stop_delta = color2->pos - color1->pos;
		uint32 step = ceil(resolution * stop_delta);

		start.word = color1->color;
		end.word = color2->color;

		curr.word = start.word;

		int r_diff = end.bytes[2] - start.bytes[2];
		int g_diff = end.bytes[1] - start.bytes[1];
		int b_diff = end.bytes[0] - start.bytes[0];
		int a_diff = end.bytes[3] - start.bytes[3];

		int r_delta = (r_diff != 0) ? (int)step / r_diff : 0;
		int g_delta = (g_diff != 0) ? (int)step / g_diff : 0;
		int b_delta = (b_diff != 0) ? (int)step / b_diff : 0;
		int a_delta = (a_diff != 0) ? (int)step / a_diff : 0;

		for (uint32 j = 0; j < step && index < resolution; ++j) {
			lut[index] = curr.word;
			++index;

			if (r_delta != 0 && j % abs(r_delta) == 0) {
				if (r_delta > 0)
					curr.bytes[2] = min_c(end.bytes[2], curr.bytes[2] + 1);
				else
					curr.bytes[2] = max_c(end.bytes[2], curr.bytes[2] - 1);
			}
			if (g_delta != 0 && j % abs(g_delta) == 0) {
				if (g_delta > 0)
					curr.bytes[1] = min_c(end.bytes[1], curr.bytes[1] + 1);
				else
					curr.bytes[1] = max_c(end.bytes[1], curr.bytes[1] - 1);
			}
			if (b_delta != 0 && j % abs(b_delta) == 0) {
				if (b_delta > 0)
					curr.bytes[0] = min_c(end.bytes[0], curr.bytes[0] + 1);
				else
					curr.bytes[0] = max_c(end.bytes[0], curr.bytes[0] - 1);
			}
			if (a_delta != 0 && j % abs(a_delta) == 0) {
				if (a_delta > 0)
					curr.bytes[3] = min_c(end.bytes[3], curr.bytes[3] + 1);
				else
					curr.bytes[3] = max_c(end.bytes[3], curr.bytes[3] - 1);
			}
		}
	}
}


uint32
Gradient::getColor(float pos)
{
	if (pos > 1.0)
		pos = 1.0;

	if (pos < 0)
		pos = 0;

	int lutIndex = (resolution - 1) * pos;

	return lut[lutIndex];
}


uint32
Gradient::getAverageColor(float pos, float pos1, float pos2, float pos3, float pos4)
{
	union color_conversion color0, color1, color2, color3, color4;

	color0.word = getColor(pos);
	color1.word = getColor(pos1);
	color2.word = getColor(pos2);
	color3.word = getColor(pos3);
	color4.word = getColor(pos4);

	float luma = getLuminance(color0.word);

	float luma1 = getLuminance(color1.word);
	float luma2 = getLuminance(color2.word);
	float luma3 = getLuminance(color3.word);
	float luma4 = getLuminance(color4.word);

	float rangeMin = min_c(luma, min_c(min_c(luma3, luma1), min_c(luma4, luma2)));
	float rangeMax = max_c(luma, max_c(max_c(luma3, luma1), min_c(luma4, luma2)));
	float range = rangeMax - rangeMin;

	if  (range >= max_c(0.0625, rangeMax * 0.125)) {
		for (int idx = 0; idx < 4; ++idx) {
			color0.bytes[idx] = min_c(255, ((int)(color1.bytes[idx] +
				color2.bytes[idx] +
				color3.bytes[idx] +
				color4.bytes[idx] +
				color0.bytes[idx]) / 5));
		}
	}

	return color0.word;
}


float
Gradient::getLuminance(uint32 color)
{
	union color_conversion inColor;

	inColor.word = color;

	return inColor.bytes[2] * (0.587 / 0.299) + inColor.bytes[1];
}