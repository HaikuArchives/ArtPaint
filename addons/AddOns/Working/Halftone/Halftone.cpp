/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Window.h>

#include "AddOns.h"
#include "Halftone.h"
#include "RandomNumberGenerator.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Halftone";
	char menu_help_string[255] = "Makes a halftone-pattern of the image with foreground and background colors.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = GENERIC_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap*,ManipulatorInformer *i)
{
	return new Halftone(i);
}



Halftone::Halftone(ManipulatorInformer *i)
	: Manipulator(), round_dot_size(ROUND_DOT_SIZE), diagonal_line_size(DIAGONAL_LINE_SIZE)
	, ordered_matrix_size(ORDERED_MATRIX_SIZE)
{
	informer = i;
	round_dot_pattern[0][0] = 1;
	round_dot_pattern[0][1] = 8;
	round_dot_pattern[0][2] = 16;
	round_dot_pattern[0][3] = 29;
	round_dot_pattern[0][4] = 25;
	round_dot_pattern[0][5] = 22;
	round_dot_pattern[0][6] = 6;
	round_dot_pattern[0][7] = 2;

	round_dot_pattern[1][0] = 5;
	round_dot_pattern[1][1] = 12;
	round_dot_pattern[1][2] = 33;
	round_dot_pattern[1][3] = 42;
	round_dot_pattern[1][4] = 46;
	round_dot_pattern[1][5] = 38;
	round_dot_pattern[1][6] = 13;
	round_dot_pattern[1][7] = 9;

	round_dot_pattern[2][0] = 21;
	round_dot_pattern[2][1] = 37;
	round_dot_pattern[2][2] = 49;
	round_dot_pattern[2][3] = 58;
	round_dot_pattern[2][4] = 54;
	round_dot_pattern[2][5] = 50;
	round_dot_pattern[2][6] = 34;
	round_dot_pattern[2][7] = 17;

	round_dot_pattern[3][0] = 24;
	round_dot_pattern[3][1] = 45;
	round_dot_pattern[3][2] = 53;
	round_dot_pattern[3][3] = 62;
	round_dot_pattern[3][4] = 63;
	round_dot_pattern[3][5] = 59;
	round_dot_pattern[3][6] = 43;
	round_dot_pattern[3][7] = 30;

	round_dot_pattern[4][0] = 28;
	round_dot_pattern[4][1] = 41;
	round_dot_pattern[4][2] = 57;
	round_dot_pattern[4][3] = 61;
	round_dot_pattern[4][4] = 60;
	round_dot_pattern[4][5] = 55;
	round_dot_pattern[4][6] = 47;
	round_dot_pattern[4][7] = 26;

	round_dot_pattern[5][0] = 19;
	round_dot_pattern[5][1] = 32;
	round_dot_pattern[5][2] = 48;
	round_dot_pattern[5][3] = 52;
	round_dot_pattern[5][4] = 56;
	round_dot_pattern[5][5] = 51;
	round_dot_pattern[5][6] = 39;
	round_dot_pattern[5][7] = 23;

	round_dot_pattern[6][0] = 11;
	round_dot_pattern[6][1] = 15;
	round_dot_pattern[6][2] = 36;
	round_dot_pattern[6][3] = 44;
	round_dot_pattern[6][4] = 40;
	round_dot_pattern[6][5] = 35;
	round_dot_pattern[6][6] = 14;
	round_dot_pattern[6][7] = 7;

	round_dot_pattern[7][0] = 0;
	round_dot_pattern[7][1] = 4;
	round_dot_pattern[7][2] = 20;
	round_dot_pattern[7][3] = 27;
	round_dot_pattern[7][4] = 31;
	round_dot_pattern[7][5] = 18;
	round_dot_pattern[7][6] = 10;
	round_dot_pattern[7][7] = 3;


	diagonal_line_pattern[2][2] = 24;
	diagonal_line_pattern[1][3] = 23;
	diagonal_line_pattern[3][1] = 22;
	diagonal_line_pattern[0][4] = 21;
	diagonal_line_pattern[4][0] = 20;

	diagonal_line_pattern[1][0] = 19;
	diagonal_line_pattern[0][1] = 18;

	diagonal_line_pattern[4][3] = 17;
	diagonal_line_pattern[3][4] = 16;

	diagonal_line_pattern[1][1] = 15;
	diagonal_line_pattern[0][2] = 14;
	diagonal_line_pattern[2][0] = 13;

	diagonal_line_pattern[3][3] = 12;
	diagonal_line_pattern[2][4] = 11;
	diagonal_line_pattern[4][2] = 10;

	diagonal_line_pattern[2][1] = 9;
	diagonal_line_pattern[1][2] = 8;
	diagonal_line_pattern[3][0] = 7;
	diagonal_line_pattern[0][3] = 6;

	diagonal_line_pattern[3][2] = 5;
	diagonal_line_pattern[2][3] = 4;
	diagonal_line_pattern[4][1] = 3;
	diagonal_line_pattern[1][4] = 2;

	diagonal_line_pattern[0][0] = 1;
	diagonal_line_pattern[4][4] = 0;

	ordered_matrix[0][0] = 0;
	ordered_matrix[0][1] = 8;
	ordered_matrix[0][2] = 2;
	ordered_matrix[0][3] = 10;

	ordered_matrix[1][0] = 12;
	ordered_matrix[1][1] = 4;
	ordered_matrix[1][2] = 14;
	ordered_matrix[1][3] = 16;

	ordered_matrix[2][0] = 3;
	ordered_matrix[2][1] = 11;
	ordered_matrix[2][2] = 1;
	ordered_matrix[2][3] = 9;

	ordered_matrix[3][0] = 15;
	ordered_matrix[3][1] = 7;
	ordered_matrix[3][2] = 13;
	ordered_matrix[3][3] = 5;
}


Halftone::~Halftone()
{
	delete informer;
}


BBitmap* Halftone::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	return round_dot_halftone(original,selection,status_bar);
//	return diagonal_line_halftone(original,selection,status_bar);
//	return ordered_dither_halftone(original,selection,status_bar);
//	return fs_dither_halftone(original,selection,status_bar);
//	return ncandidate_dither_halftone(original,selection,status_bar);
}

const char* Halftone::ReturnName()
{
	return "Halftone";
}


BBitmap* Halftone::round_dot_halftone(BBitmap *original,Selection *selection, BStatusBar *status_bar)
{
	if (original == NULL)
		return NULL;

	uint32 *source_bits = (uint32*)original->Bits();
	uint32 *target_bits = (uint32*)original->Bits();
	int32 source_bpr = original->BytesPerRow()/4;
	int32 target_bpr = original->BytesPerRow()/4;
	int32 top,bottom,left,right;

	left = original->Bounds().left;
	right = original->Bounds().right + 1;
	top = original->Bounds().top;
	bottom = original->Bounds().bottom + 1;


	union {
		uint8 bytes[4];
		uint32 word;
	} color,c1,c2;
	rgb_color c = informer->GetBackgroundColor();
	c1.bytes[0] = c.blue;
	c1.bytes[1] = c.green;
	c1.bytes[2] = c.red;
	c1.bytes[3] = c.alpha;

	c = informer->GetForegroundColor();
	c2.bytes[0] = c.blue;
	c2.bytes[1] = c.green;
	c2.bytes[2] = c.red;
	c2.bytes[3] = c.alpha;

	if (selection->IsEmpty()) {
		// Here handle the whole image.
		float normalizer = 1.0/255.0*round_dot_size*round_dot_size;
		for (int32 y=top;y<bottom;y+=round_dot_size) {
			for (int32 x=left;x<right;x+=round_dot_size) {
				int32 r =min_c(x+round_dot_size,right);
				int32 b = min_c(y+round_dot_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						color.word = *s_delta_bits;
						float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
						int threshold = luminance * normalizer;
						*s_delta_bits++ = (round_dot_pattern[dy-y][dx-x]>threshold?c1.word:c2.word);
					}
				}
			}
		}
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		float normalizer = 1.0/255.0*round_dot_size*round_dot_size;
		for (int32 y=top;y<bottom;y+=round_dot_size) {
			for (int32 x=left;x<right;x+=round_dot_size) {
				int32 r =min_c(x+round_dot_size,right);
				int32 b = min_c(y+round_dot_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						if (selection->ContainsPoint(dx,dy)) {
							color.word = *s_delta_bits;
							float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
							int threshold = luminance * normalizer;
							*s_delta_bits = (round_dot_pattern[dy-y][dx-x]>threshold?c1.word:c2.word);
						}
						s_delta_bits++;
					}
				}
			}
		}
	}

	return original;
}


BBitmap* Halftone::diagonal_line_halftone(BBitmap *original,Selection *selection, BStatusBar *status_bar)
{
	if (original == NULL)
		return NULL;

	uint32 *source_bits = (uint32*)original->Bits();
	uint32 *target_bits = (uint32*)original->Bits();
	int32 source_bpr = original->BytesPerRow()/4;
	int32 target_bpr = original->BytesPerRow()/4;
	int32 top,bottom,left,right;

	left = original->Bounds().left;
	right = original->Bounds().right + 1;
	top = original->Bounds().top;
	bottom = original->Bounds().bottom + 1;


	union {
		uint8 bytes[4];
		uint32 word;
	} color,c1,c2;
	rgb_color c = informer->GetForegroundColor();
	c1.bytes[0] = c.blue;
	c1.bytes[1] = c.green;
	c1.bytes[2] = c.red;
	c1.bytes[3] = c.alpha;

	c = informer->GetBackgroundColor();
	c2.bytes[0] = c.blue;
	c2.bytes[1] = c.green;
	c2.bytes[2] = c.red;
	c2.bytes[3] = c.alpha;

	if (selection->IsEmpty()) {
		// Here handle the whole image.
		float normalizer = 1.0/255.0*diagonal_line_size*diagonal_line_size;
		for (int32 y=top;y<bottom;y+=diagonal_line_size) {
			for (int32 x=left;x<right;x+=diagonal_line_size) {
				int32 r =min_c(x+diagonal_line_size,right);
				int32 b = min_c(y+diagonal_line_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						color.word = *s_delta_bits;
						float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
						int threshold = luminance * normalizer;
						*s_delta_bits++ = (diagonal_line_pattern[dy-y][dx-x]>threshold?c1.word:c2.word);
					}
				}
			}
		}
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		float normalizer = 1.0/255.0*diagonal_line_size*diagonal_line_size;
		for (int32 y=top;y<bottom;y+=diagonal_line_size) {
			for (int32 x=left;x<right;x+=diagonal_line_size) {
				int32 r =min_c(x+diagonal_line_size,right);
				int32 b = min_c(y+diagonal_line_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						if (selection->ContainsPoint(dx,dy)) {
							color.word = *s_delta_bits;
							float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
							int threshold = luminance * normalizer;
							*s_delta_bits = (diagonal_line_pattern[dy-y][dx-x]>threshold?c1.word:c2.word);
						}
						s_delta_bits++;
					}
				}
			}
		}
	}

	return original;
}

BBitmap* Halftone::ordered_dither_halftone(BBitmap *original,Selection *selection, BStatusBar *status_bar)
{
	if (original == NULL)
		return NULL;

	uint32 *source_bits = (uint32*)original->Bits();
	uint32 *target_bits = (uint32*)original->Bits();
	int32 source_bpr = original->BytesPerRow()/4;
	int32 target_bpr = original->BytesPerRow()/4;
	int32 top,bottom,left,right;

	left = original->Bounds().left;
	right = original->Bounds().right + 1;
	top = original->Bounds().top;
	bottom = original->Bounds().bottom + 1;


	union {
		uint8 bytes[4];
		uint32 word;
	} color,c1,c2;
	rgb_color c = informer->GetForegroundColor();
	c1.bytes[0] = c.blue;
	c1.bytes[1] = c.green;
	c1.bytes[2] = c.red;
	c1.bytes[3] = c.alpha;

	c = informer->GetBackgroundColor();
	c2.bytes[0] = c.blue;
	c2.bytes[1] = c.green;
	c2.bytes[2] = c.red;
	c2.bytes[3] = c.alpha;

	if (selection->IsEmpty()) {
		// Here handle the whole image.
		float normalizer = 1.0/255.0*ordered_matrix_size*ordered_matrix_size;
		for (int32 y=top;y<bottom;y+=ordered_matrix_size) {
			for (int32 x=left;x<right;x+=ordered_matrix_size) {
				int32 r =min_c(x+ordered_matrix_size,right);
				int32 b = min_c(y+ordered_matrix_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						color.word = *s_delta_bits;
						float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
						int threshold = luminance * normalizer;
						*s_delta_bits++ = (ordered_matrix[dy-y][dx-x]>threshold?c1.word:c2.word);
					}
				}
			}
		}
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		float normalizer = 1.0/255.0*ordered_matrix_size*ordered_matrix_size;
		for (int32 y=top;y<bottom;y+=ordered_matrix_size) {
			for (int32 x=left;x<right;x+=ordered_matrix_size) {
				int32 r =min_c(x+ordered_matrix_size,right);
				int32 b = min_c(y+ordered_matrix_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						if (selection->ContainsPoint(dx,dy)) {
							color.word = *s_delta_bits;
							float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
							int threshold = luminance * normalizer;
							*s_delta_bits = (ordered_matrix[dy-y][dx-x]>threshold?c1.word:c2.word);
						}
						s_delta_bits++;
					}
				}
			}
		}
	}

	return original;
}


BBitmap* Halftone::fs_dither_halftone(BBitmap *original,Selection *selection, BStatusBar *status_bar)
{
	if (original == NULL)
		return NULL;

	uint32 *source_bits = (uint32*)original->Bits();
	uint32 *target_bits = (uint32*)original->Bits();
	int32 source_bpr = original->BytesPerRow()/4;
	int32 target_bpr = original->BytesPerRow()/4;
	int32 top,bottom,left,right;

	left = original->Bounds().left;
	right = original->Bounds().right + 1;
	top = original->Bounds().top;
	bottom = original->Bounds().bottom + 1;

	float *errors = new float[right-left+3];
	for (int32 i=0;i<right-left+3;i++)
		errors[i] = 0;

	float right_error = 0;

	union {
		uint8 bytes[4];
		uint32 word;
	} color,c1,c2;
	rgb_color c = informer->GetForegroundColor();
	c1.bytes[0] = c.blue;
	c1.bytes[1] = c.green;
	c1.bytes[2] = c.red;
	c1.bytes[3] = c.alpha;

	c = informer->GetBackgroundColor();
	c2.bytes[0] = c.blue;
	c2.bytes[1] = c.green;
	c2.bytes[2] = c.red;
	c2.bytes[3] = c.alpha;

	if (selection->IsEmpty()) {
		// Here handle the whole image.
		for (int32 y=top;y<bottom;y++) {
			for (int32 x=left;x<right;x++) {
				color.word = *source_bits;
				float threshold = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
				float value = min_c(255,max_c(threshold+right_error+errors[x+1],0));
				errors[x+1] = 0;
				right_error = 0;
				float error;
				if (value > 127) {
					error = -(255 - value);
					*source_bits++ = c2.word;
				}
				else {
					error = -(0 - value);
					*source_bits++ = c1.word;
				}
				right_error = .4375 * error;
				errors[x] += .1875 * error;
				errors[x+1] += .3125 * error;
				errors[x+2] += .0625 * error;
			}
		}
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		float normalizer = 1.0/255.0*ordered_matrix_size*ordered_matrix_size;
		for (int32 y=top;y<bottom;y+=ordered_matrix_size) {
			for (int32 x=left;x<right;x+=ordered_matrix_size) {
				int32 r =min_c(x+ordered_matrix_size,right);
				int32 b = min_c(y+ordered_matrix_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						if (selection->ContainsPoint(dx,dy)) {
							color.word = *s_delta_bits;
							float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
							int threshold = luminance * normalizer;
							*s_delta_bits = (ordered_matrix[dy-y][dx-x]>threshold?c1.word:c2.word);
						}
						s_delta_bits++;
					}
				}
			}
		}
	}

	return original;
}


BBitmap* Halftone::ncandidate_dither_halftone(BBitmap *original,Selection *selection, BStatusBar *status_bar)
{
	if (original == NULL)
		return NULL;

	uint32 *source_bits = (uint32*)original->Bits();
	uint32 *target_bits = (uint32*)original->Bits();
	int32 source_bpr = original->BytesPerRow()/4;
	int32 target_bpr = original->BytesPerRow()/4;
	int32 top,bottom,left,right;

	left = original->Bounds().left;
	right = original->Bounds().right + 1;
	top = original->Bounds().top;
	bottom = original->Bounds().bottom + 1;

	float *errors = new float[right-left+3];
	for (int32 i=0;i<right-left+3;i++)
		errors[i] = 0;

	float right_error = 0;

	union {
		uint8 bytes[4];
		uint32 word;
	} color,c1,c2;
	rgb_color c = informer->GetForegroundColor();
	c1.bytes[0] = c.blue;
	c1.bytes[1] = c.green;
	c1.bytes[2] = c.red;
	c1.bytes[3] = c.alpha;

	c = informer->GetBackgroundColor();
	c2.bytes[0] = c.blue;
	c2.bytes[1] = c.green;
	c2.bytes[2] = c.red;
	c2.bytes[3] = c.alpha;

	float probs[256];
	for (int32 i=0;i<256;i++) {
		probs[i] = (float)i/256.0;	// probability to get white
	}

	RandomNumberGenerator *generator = new RandomNumberGenerator(1027,1000000);
	if (selection->IsEmpty()) {
		// Here handle the whole image.
		for (int32 y=top;y<bottom;y++) {
			for (int32 x=left;x<right;x++) {
				color.word = *source_bits;
				int32 threshold = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
				float r = generator->UniformDistribution(0.0,1.0);
				if (probs[threshold] >= r)
					*source_bits++ = c2.word;
				else
					*source_bits++ = c1.word;
			}
		}
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		float normalizer = 1.0/255.0*ordered_matrix_size*ordered_matrix_size;
		for (int32 y=top;y<bottom;y+=ordered_matrix_size) {
			for (int32 x=left;x<right;x+=ordered_matrix_size) {
				int32 r =min_c(x+ordered_matrix_size,right);
				int32 b = min_c(y+ordered_matrix_size,bottom);
				uint32 *s_delta_bits;

				int32 number_of_pixels = 0;
				for (int32 dy=y;dy<b;dy++) {
					s_delta_bits = source_bits + dy*source_bpr + x;
					for (int32 dx=x;dx<r;dx++) {
						if (selection->ContainsPoint(dx,dy)) {
							color.word = *s_delta_bits;
							float luminance = color.bytes[0] * .114 + color.bytes[1]*.587 + color.bytes[2]*.299;
							int threshold = luminance * normalizer;
							*s_delta_bits = (ordered_matrix[dy-y][dx-x]>threshold?c1.word:c2.word);
						}
						s_delta_bits++;
					}
				}
			}
		}
	}
	delete generator;
	return original;
}

