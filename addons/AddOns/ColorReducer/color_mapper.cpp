/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */


/*
	Contents:	Functions that map an image to a palette while reducing
				artifacts caused by reduction of colors.

	Notes:		Currently all returned bitmaps are in B_CMAP8 mode. The
				indices in the image are to the palette that is provided as
				a parameter.
*/

#include <Bitmap.h>
#include <stdio.h>

#include "BitmapAnalyzer.h"
#include "ImageProcessingLibrary.h"
#include "RandomNumberGenerator.h"
#include "Selection.h"
#include "color_mapper.h"


BBitmap*
nearest_color_mapper(BBitmap* inSource, const rgb_color* inPalette, int inPaletteSize)
{
	int32* map_function = new int32[32768]; // using 15 bit colors to do the mapping
	for (int i = 0; i < 32768; i++)
		map_function[i] = -1; // the nearest color is not yet found

	BBitmap* outDestination = new BBitmap(inSource->Bounds(), B_CMAP8, false);

	uint32* source_bits = (uint32*)inSource->Bits();
	uint32 source_bpr = inSource->BytesPerRow() / 4;

	uint8* destination_bits = (uint8*)outDestination->Bits();
	uint32 destination_bpr = outDestination->BytesPerRow();

	int32 width = inSource->Bounds().IntegerWidth();
	int32 height = inSource->Bounds().IntegerHeight();

	int32 source_padding = source_bpr - width - 1;
	int32 destination_padding = destination_bpr - width - 1;

	// Use this union to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;

	for (int32 y = 0; y <= height; y++) {
		for (int32 x = 0; x <= width; x++) {
			bgra32.word = *source_bits++;
			// squeeze the 32-bit color to 15 bit index. See BeBook
			// BScreen chapter for the reference on this.
			uint16 rgb15 = ((bgra32.bytes[2] & 0xf8) << 7) | ((bgra32.bytes[1] & 0xf8) << 2)
				| ((bgra32.bytes[0] & 0xf8) >> 3);
			if (map_function[rgb15] < 0) {
				// The mapping for the color has not yet been found.
				map_function[rgb15] = find_palette_index(bgra32.word, inPalette, inPaletteSize);
			}
			*destination_bits++ = (uint8)map_function[rgb15];
		}
		destination_bits += destination_padding;
		source_bits += source_padding;
	}

	delete[] map_function;

	return outDestination;
}


/*
	This function maps the colors by using Floyd-Steinberg error diffusion dithering.
	The main idea is to distribute the quantization error to the neighbouring
	(unprocessed) pixels. The following weights are used in this version:

				X		7/16

		3/16	5/16	1/16

	Every other scanline is processed from left to right and every other from right to left.
	This reduces the dithering artifacts and helps with the cache on big images. Currently
	this is not the case and every scanline is processed from left to right.

	Note that Floyd-Steinberg (or any other EDD) is almost impossible to parellelize properly.
	This is because error propagates through image from pixel to pixel. Any attemp to divide the
	image in blocks and dither them separately leads to unwanted visible lines between the
	different parts.
*/
BBitmap*
floyd_steinberg_edd_color_mapper(BBitmap* inSource, const rgb_color* inPalette, int inPaletteSize)
{
	int32* map_function = new int32[32768]; // using 15 bit colors to do the mapping
	for (int i = 0; i < 32768; i++)
		map_function[i] = -1; // the nearest color is not yet found

	BBitmap* outDestination = new BBitmap(inSource->Bounds(), B_CMAP8, false);

	uint32* source_bits = (uint32*)inSource->Bits();
	uint32 source_bpr = inSource->BytesPerRow() / 4;

	uint8* destination_bits = (uint8*)outDestination->Bits();
	uint32 destination_bpr = outDestination->BytesPerRow();

	int32 width = inSource->Bounds().IntegerWidth();
	int32 height = inSource->Bounds().IntegerHeight();

	int32 source_padding = source_bpr - width - 1;
	int32 destination_padding = destination_bpr - width - 1;

	// We use three error arrays and three side error variables.
	// The error arrays are a little bit wider than the image to
	// avoid cheking for border conditions
	int32* red_error = new int32[width + 1 + 2];
	red_error = &red_error[1]; // Allow index of -1 too.
	int16 red_side_error = 0;

	int32* green_error = new int32[width + 1 + 2];
	green_error = &green_error[1]; // Allow index of -1 too.
	int16 green_side_error = 0;

	int32* blue_error = new int32[width + 1 + 2];
	blue_error = &blue_error[1]; // Allow index of -1 too.
	int16 blue_side_error = 0;

	// Initialize the errors to 0.
	for (int32 i = -1; i <= width + 1; i++)
		red_error[i] = green_error[i] = blue_error[i] = 0;

	// We use fixed point arithmetic to avoid type conversions.
	int32 one_sixteenth = (1.0 / 16.0) * 32768;
	int32 three_sixteenth = (3.0 / 16.0) * 32768;
	int32 five_sixteenth = (5.0 / 16.0) * 32768;
	int32 seven_sixteenth = (7.0 / 16.0) * 32768;

	// Use this union to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;

	for (int32 y = 0; y <= height; y++) {
		red_side_error = green_side_error = blue_side_error = 0;
		// Go from left to right.
		for (int32 x = 0; x <= width; x++) {
			bgra32.word = *source_bits++;
			// Add the error.
			bgra32.bytes[0] = min_c(255, max_c(0, bgra32.bytes[0] - blue_side_error));
			bgra32.bytes[1] = min_c(255, max_c(0, bgra32.bytes[1] - green_side_error));
			bgra32.bytes[2] = min_c(255, max_c(0, bgra32.bytes[2] - red_side_error));

			// squeeze the 32-bit color to 15 bit index. See BeBook
			// BScreen chapter for the reference on this.
			uint16 rgb15 = ((bgra32.bytes[2] & 0xf8) << 7) | ((bgra32.bytes[1] & 0xf8) << 2)
				| ((bgra32.bytes[0] & 0xf8) >> 3);

			if (map_function[rgb15] < 0) {
				// The mapping for the color has not yet been found.
				map_function[rgb15] = find_palette_index(bgra32.word, inPalette, inPaletteSize);
			}

			uint8 color_index = *destination_bits++ = (uint8)map_function[rgb15];

			int32 red_total_error = inPalette[color_index].red - bgra32.bytes[2];
			int32 green_total_error = inPalette[color_index].green - bgra32.bytes[1];
			int32 blue_total_error = inPalette[color_index].blue - bgra32.bytes[0];

			red_side_error = (red_error[x + 1] + (red_total_error * seven_sixteenth)) >> 15;
			blue_side_error = (blue_error[x + 1] + (blue_total_error * seven_sixteenth)) >> 15;
			green_side_error
				= (green_error[x + 1] + (green_total_error * seven_sixteenth)) >> 15;

			red_error[x + 1] = (red_total_error * one_sixteenth);
			green_error[x + 1] = (green_total_error * one_sixteenth);
			blue_error[x + 1] = (blue_total_error * one_sixteenth);

			red_error[x] += (red_total_error * five_sixteenth);
			green_error[x] += (green_total_error * five_sixteenth);
			blue_error[x] += (blue_total_error * five_sixteenth);

			red_error[x - 1] += (red_total_error * three_sixteenth);
			green_error[x - 1] += (green_total_error * three_sixteenth);
			blue_error[x - 1] += (blue_total_error * three_sixteenth);
		}
		destination_bits += destination_padding;
		source_bits += source_padding;
	}

	delete[] map_function;

	return outDestination;
}


BBitmap*
preserve_solids_fs_color_mapper(BBitmap* inSource, const rgb_color* inPalette, int inPaletteSize)
{
	int32* map_function = new int32[32768]; // using 15 bit colors to do the mapping
	for (int i = 0; i < 32768; i++)
		map_function[i] = -1; // the nearest color is not yet found

	BBitmap* outDestination = new BBitmap(inSource->Bounds(), B_CMAP8, false);

	uint32* source_bits = (uint32*)inSource->Bits();
	uint32 source_bpr = inSource->BytesPerRow() / 4;

	uint8* destination_bits = (uint8*)outDestination->Bits();
	uint32 destination_bpr = outDestination->BytesPerRow();

	int32 width = inSource->Bounds().IntegerWidth();
	int32 height = inSource->Bounds().IntegerHeight();

	// We use three error arrays and three side error variables.
	// The error arrays are a little bit wider than the image to
	// avoid cheking for border conditions
	int32* red_error = new int32[width + 1 + 2];
	red_error = &red_error[1]; // Allow index of -1 too.
	int16 red_side_error = 0;

	int32* green_error = new int32[width + 1 + 2];
	green_error = &green_error[1]; // Allow index of -1 too.
	int16 green_side_error = 0;

	int32* blue_error = new int32[width + 1 + 2];
	blue_error = &blue_error[1]; // Allow index of -1 too.
	int16 blue_side_error = 0;

	// Initialize the errors to 0.
	for (int32 i = -1; i <= width + 1; i++)
		red_error[i] = green_error[i] = blue_error[i] = 0;

	// We use fixed point arithmetic to avoid type conversions.
	int32 one_sixteenth = (1.0 / 16.0) * 32768;
	int32 three_sixteenth = (3.0 / 16.0) * 32768;
	int32 five_sixteenth = (5.0 / 16.0) * 32768;
	int32 seven_sixteenth = (7.0 / 16.0) * 32768;

	// Use this union to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;

	// Make the blurred bitmap
	BBitmap* blurred = new BBitmap(inSource);
	ImageProcessingLibrary iplib;
	iplib.gaussian_blur(blurred, 10);
	BitmapAnalyzer* analyzer = new BitmapAnalyzer(inSource);

	for (int32 y = 0; y <= height; y++) {
		red_side_error = green_side_error = blue_side_error = 0;
		// Go from left to right.
		for (int32 x = 0; x <= width; x++) {
			if (analyzer->GradientMagnitude(x, y) > 0) {
				bgra32.word = *(source_bits + x + y * source_bpr);
				// Add the error.
				bgra32.bytes[0] = min_c(255, max_c(0, bgra32.bytes[0] - blue_side_error));
				bgra32.bytes[1] = min_c(255, max_c(0, bgra32.bytes[1] - green_side_error));
				bgra32.bytes[2] = min_c(255, max_c(0, bgra32.bytes[2] - red_side_error));

				// squeeze the 32-bit color to 15 bit index. See BeBook
				// BScreen chapter for the reference on this.
				uint16 rgb15
					= ((bgra32.bytes[2] & 0xf8) << 7)
					| ((bgra32.bytes[1] & 0xf8) << 2)
					| ((bgra32.bytes[0] & 0xf8) >> 3);

				if (map_function[rgb15] < 0) {
					// The mapping for the color has not yet been found.
					map_function[rgb15]
						= find_palette_index(bgra32.word, inPalette, inPaletteSize);
				}

				uint8 color_index = *(destination_bits + x + y * destination_bpr) 
					= (uint8)map_function[rgb15];

				int32 red_total_error = inPalette[color_index].red - bgra32.bytes[2];
				int32 green_total_error = inPalette[color_index].green - bgra32.bytes[1];
				int32 blue_total_error = inPalette[color_index].blue - bgra32.bytes[0];

				red_side_error
					= (red_error[x + 1] + (red_total_error * seven_sixteenth)) >> 15;
				blue_side_error
					= (blue_error[x + 1] + (blue_total_error * seven_sixteenth)) >> 15;
				green_side_error
					= (green_error[x + 1] + (green_total_error * seven_sixteenth)) >> 15;

				red_error[x + 1] = (red_total_error * one_sixteenth);
				green_error[x + 1] = (green_total_error * one_sixteenth);
				blue_error[x + 1] = (blue_total_error * one_sixteenth);

				red_error[x] += (red_total_error * five_sixteenth);
				green_error[x] += (green_total_error * five_sixteenth);
				blue_error[x] += (blue_total_error * five_sixteenth);

				red_error[x - 1] += (red_total_error * three_sixteenth);
				green_error[x - 1] += (green_total_error * three_sixteenth);
				blue_error[x - 1] += (blue_total_error * three_sixteenth);
			} else {
				bgra32.word = *(source_bits + x + y * source_bpr);

				// squeeze the 32-bit color to 15 bit index. See BeBook
				// BScreen chapter for the reference on this.
				uint16 rgb15
					= ((bgra32.bytes[2] & 0xf8) << 7)
					| ((bgra32.bytes[1] & 0xf8) << 2)
					| ((bgra32.bytes[0] & 0xf8) >> 3);

				if (map_function[rgb15] < 0) {
					// The mapping for the color has not yet been found.
					map_function[rgb15]
						= find_palette_index(bgra32.word, inPalette, inPaletteSize);
				}

				red_side_error = red_error[x + 1] >> 15;
				blue_side_error = blue_error[x + 1] >> 15;
				green_side_error = green_error[x + 1] >> 15;

				red_error[x + 1] = 0;
				green_error[x + 1] = 0;
				blue_error[x + 1] = 0;
			}
		}
	}

	delete[] map_function;
	delete analyzer;
	delete blurred;

	return outDestination;
}


BBitmap*
n_candidate_color_mapper(
	BBitmap* inSource, const rgb_color* inPalette, int inPaletteSize, int maxCandidates)
{
	RandomNumberGenerator* generator = new RandomNumberGenerator(10111071, 10000);

	struct Candidate {
		int32 index;
		float prob;
	};

	struct CandidateList {
		int32* candidate_count;
		Candidate** candidate_table;
	};

	CandidateList candidates;
	candidates.candidate_table = new Candidate*[32768];
	candidates.candidate_count = new int32[32768];

	for (int i = 0; i < 32768; i++) {
		candidates.candidate_table[i] = new Candidate[maxCandidates];
		candidates.candidate_count[i] = 0; // the candidates have not yet been found
	}

	BBitmap* outDestination = new BBitmap(inSource->Bounds(), B_CMAP8, false);

	uint32* source_bits = (uint32*)inSource->Bits();
	uint32 source_bpr = inSource->BytesPerRow() / 4;

	uint8* destination_bits = (uint8*)outDestination->Bits();
	uint32 destination_bpr = outDestination->BytesPerRow();

	int32 width = inSource->Bounds().IntegerWidth();
	int32 height = inSource->Bounds().IntegerHeight();

	int32 source_padding = source_bpr - width - 1;
	int32 destination_padding = destination_bpr - width - 1;

	// Use this union to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;

	for (int32 y = 0; y <= height; y++) {
		for (int32 x = 0; x <= width; x++) {
			bgra32.word = *source_bits++;
			// squeeze the 32-bit color to 15 bit index. See BeBook
			// BScreen chapter for the reference on this.
			uint16 rgb15
				= ((bgra32.bytes[2] & 0xf8) << 7)
				| ((bgra32.bytes[1] & 0xf8) << 2)
				| ((bgra32.bytes[0] & 0xf8) >> 3);
			if (candidates.candidate_count[rgb15] <= 0) {
				// The candidates for the color have not yet been found.
//				map_function[rgb15] = find_palette_index(bgra32.word,inPalette,inPaletteSize);
				float max_error = -1;
				float inverse_error_sum = 0;
				int32 number_of_candidates = 0;
				for (int32 i = 0; i < maxCandidates; i++) {
					float distance;
					candidates.candidate_table[rgb15][i].index
						= find_candidate(bgra32.word, inPalette, inPaletteSize, i + 1, &distance);
					candidates.candidate_table[rgb15][i].prob = distance;
					if (max_error < 0) {
						max_error = 5.0 * distance;
						number_of_candidates++;
						if (distance > 0)
							inverse_error_sum += 1.0 / distance;
						printf("color:\t%d\t%d\t%d\n", bgra32.bytes[2], bgra32.bytes[1],
							bgra32.bytes[0]);
						rgb_color pc = inPalette[candidates.candidate_table[rgb15][i].index];
						printf("\tcandidate:\t%d\t%d\t%d\n", pc.red, pc.green, pc.blue);
					} else if (distance <= max_error) {
						number_of_candidates++;
						rgb_color pc = inPalette[candidates.candidate_table[rgb15][i].index];
						printf("\tcandidate:\t%d\t%d\t%d\n", pc.red, pc.green, pc.blue);
						if (distance > 0)
							inverse_error_sum += 1.0 / distance;
					}
				}

				// calculate the probabilities
				candidates.candidate_count[rgb15] = number_of_candidates;
				for (int32 i = 0; i < number_of_candidates; i++) {
					float dist = candidates.candidate_table[rgb15][i].prob;
					if (dist > 0) {
						candidates.candidate_table[rgb15][i].prob
							= (1.0 / dist) / inverse_error_sum;
						if (i > 0) {
							candidates.candidate_table[rgb15][i].prob
								+= candidates.candidate_table[rgb15][i - 1].prob;
						}
					} else
						candidates.candidate_table[rgb15][i].prob = 1.0;
				}
			}

			// Here take a random number and select the proper candidate.
			float random_number = generator->UniformDistribution(0, 1.0);
			int i = 0;
			while ((candidates.candidate_table[rgb15][i].prob < random_number)
				&& (i < (candidates.candidate_count[rgb15] - 1)))
				i++;

			*destination_bits++ = candidates.candidate_table[rgb15][i].index;
		}
		destination_bits += destination_padding;
		source_bits += source_padding;
	}

	return outDestination;
}
