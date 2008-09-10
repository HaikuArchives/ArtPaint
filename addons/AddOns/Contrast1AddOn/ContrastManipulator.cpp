/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "AddOns.h"
#include "ContrastManipulator.h"


extern "C" __declspec(dllexport) char name[255] = "Stretch Histogram";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Improves the contrast of the active layer by stretching its histogram.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = COLOR_ADD_ON;


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new ContrastManipulator(bm);
}


ContrastManipulator::ContrastManipulator(BBitmap*)
		: Manipulator()
{
}


ContrastManipulator::~ContrastManipulator()
{

}


BBitmap* ContrastManipulator::ManipulateBitmap(BBitmap *original, Selection *selection, BStatusBar*)
{
	uint32 *bits = (uint32*)original->Bits();
	int32 bits_length = original->BitsLength()/4;

	// This manipulator stretches the histogram of the image, so that the lowest
	// values will be 0 and largest will be 255. This will not touch the alpha-channel.
	int32 r_hist[256];
	int32 g_hist[256];
	int32 b_hist[256];
	for (int32 i=0;i<256;i++) {
		r_hist[i] = 0;
		g_hist[i] = 0;
		b_hist[i] = 0;
	}

	// These maps contain the values that particular values of component map to.
	// e.g red value of 23 might map to 1 <=> r_map[23] == 0x00000100.
//	uint32 r_map[256];
//	uint32 g_map[256];
//	uint32 b_map[256];

	uchar r_min=255,r_max=0;
	uchar g_min=255,g_max=0;
	uchar b_min=255,b_max=0;

	union {
		uint8 bytes[4];
		uint32 word;
	} color,map[256];

	// First generate histograms for all of the color-components.
	for (int32 i=0;i<bits_length;i++) {
		color.word = *bits++;
		++(b_hist[color.bytes[0]]);
		++(g_hist[color.bytes[1]]);
		++(r_hist[color.bytes[2]]);
	}

	// Take the maximum and minimum values of histograms.
	for (int32 i=0;i<256;i++) {
		if (r_hist[i] != 0) {
			r_min = min_c(r_min,i);
			r_max = max_c(r_max,i);
		}
		if (g_hist[i] != 0) {
			g_min = min_c(g_min,i);
			g_max = max_c(g_max,i);
		}
		if (b_hist[i] != 0) {
			b_min = min_c(b_min,i);
			b_max = max_c(b_max,i);
		}
	}


	// Here generate the mappings of colors.
	// The value i of red should map to (i-min_r)/(max_r-min_r)*255.
	for (float i=0;i<256;i++) {
		map[(int32)i].bytes[0] = (i - b_min)/(b_max-b_min)*255;
		map[(int32)i].bytes[1] = (i - g_min)/(g_max-g_min)*255;
		map[(int32)i].bytes[2] = (i - r_min)/(r_max-r_min)*255;

//		r_map[(int32)i] = (((uint32)(((float)(i-r_min))/((float)(r_max-r_min))*255.0)) << 8) & 0x0000FF00;
//		g_map[(int32)i] = (((uint32)(((float)(i-g_min))/((float)(g_max-g_min))*255.0)) << 16) & 0x00FF0000;
//		b_map[(int32)i] = (((uint32)(((float)(i-b_min))/((float)(b_max-b_min))*255.0)) << 24) & 0xFF000000;
	}

	// Here change the pixel values according to mapping.
	bits = (uint32*)original->Bits();

	if (selection->IsEmpty() == TRUE) {
		for (int32 i=0;i<bits_length;i++) {
	//		*bits = r_map[(*bits >> 8) & 0xFF] | g_map[(*bits >> 16) & 0xFF] | b_map[(*bits >> 24) & 0xFF] | (*bits & 0xFF);
			color.word = *bits;
			color.bytes[0] = map[color.bytes[0]].bytes[0];
			color.bytes[1] = map[color.bytes[1]].bytes[1];
			color.bytes[2] = map[color.bytes[2]].bytes[2];
			*bits++ = color.word;
		}
	}
	else {
		int32 width = original->Bounds().Width();
		int32 height = original->Bounds().Height();
		for (int32 y=0;y<=height;y++) {
			for (int32 x=0;x<=width;x++) {
				if (selection->ContainsPoint(x,y)) {
					color.word = *bits;
					color.bytes[0] = map[color.bytes[0]].bytes[0];
					color.bytes[1] = map[color.bytes[1]].bytes[1];
					color.bytes[2] = map[color.bytes[2]].bytes[2];
					*bits = color.word;
				}
				++bits;
			}
		}
	}
	return original;
}
