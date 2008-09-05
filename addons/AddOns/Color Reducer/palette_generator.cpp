/*
	Filename:	palette_generator.cpp
	Contents:	Functions that generate palettes from images
	Author:		Heikki Suhonen (Heikki.Suhonen@Helsinki.FI)
*/

#include <Bitmap.h>
#include <stdio.h>

#include "palette_generator.h"
#include "color_mapper.h"
#include "ColorDistanceMetric.h"
#include "RandomNumberGenerator.h"

rgb_color* gla_palette(BBitmap *inBitmap,int paletteSize)
{
	ColorDistanceMetric *color_metric = new ColorDistanceMetric();

	
	 
	rgb_color *palette = new rgb_color[paletteSize];
	rgb_color *previous_palette = new rgb_color[paletteSize];
	
	struct color_chain {
		color_chain *next;
		rgb_color color;
	};

	color_chain **selected_colors = new color_chain*[paletteSize];


	
	color_chain *input_colors[32768];
	for (int32 i=0;i<32768;i++) {
		input_colors[i] = NULL;
	} 
	
	uint32 *bits = (uint32*)inBitmap->Bits();
	int32 bits_length = inBitmap->BitsLength()/4;
	
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	color.word = 0x00000000;	// Silence the warnings	
	
	int32 color_count = 0;
	for (int32 i=0;i<bits_length;i++) {
		color.word = *bits++;
		uint16 hash_value =  ((color.bytes[2] & 0x1f) << 10) | 
                  				((color.bytes[1] & 0x1f) << 5) | 
                  				((color.bytes[0] & 0x1f ) >> 0);

		color_chain *test_entry = input_colors[hash_value];
		bool found = false;
		rgb_color c;
		c.red = color.bytes[2];
		c.green = color.bytes[1];
		c.blue = color.bytes[0];
		c.alpha = 255;
		
		while ((test_entry != NULL) && !found) {
			rgb_color old_color = test_entry->color;
			if ((c.red == old_color.red) &&
				(c.green == old_color.green) &&
				(c.blue == old_color.blue)) {
				found = true;		
			}
			
			test_entry = test_entry->next;
		}	

		if (!found) {
			color_chain *entry = new color_chain();
			entry->next = input_colors[hash_value];
			input_colors[hash_value] = entry;
			entry->color = c;
			color_count++;
		}
	}
	int32 total_length = 0;
	int32 list_count = 0;
	for (int32 i=0;i<32768;i++) {
		int32 length = 0;
		color_chain *entry = input_colors[i];
		while (entry != NULL) {
			length++;
			entry = entry->next;
		}
		if (length > 0) {
			total_length += length;
			list_count++;
		}
	}
	printf("Number of lists is %d, average length %f\n",list_count,total_length/(float)list_count);

	// Initialize the palette.
	RandomNumberGenerator generator(123071,25000);
	for (int32 i=0;i<paletteSize;i++) {
		// Choose a random input color
		int32 index = generator.IntegerUniformDistribution(0,32767);
		while (input_colors[index] == NULL)
			index = (index+1)%32768;
					
		palette[i] = input_colors[index]->color;	
		
		palette[i].alpha = 255;		
		previous_palette[i] = palette[i];
		selected_colors[i] = NULL;
	}

	bool palette_still_improving = true;	
	int32 number_of_iterations = 0;
	while (palette_still_improving && (number_of_iterations < 100)) {
		number_of_iterations++;
		printf("Still improving\n");
		// Map all of the input colors	
		for (uint16 i=0;i<32768;i++) {
			color_chain *input_entry = input_colors[i];
			while (input_entry != NULL) {
				color.bytes[0] = input_entry->color.blue;
				color.bytes[1] = input_entry->color.green;
				color.bytes[2] = input_entry->color.red;
				color.bytes[3] = 255;
				
				int32 index = color_metric->find_palette_index(color.word,palette,paletteSize);		
				color_chain *entry = new color_chain();
				entry->next = selected_colors[index];
				selected_colors[index] = entry;
				entry->color.red = color.bytes[2];
				entry->color.green = color.bytes[1];
				entry->color.blue = color.bytes[0];
				entry->color.alpha = 255;
				
				input_entry = input_entry->next;
			}
		}
		
		// Store the current palette
		for (int32 i=0;i<paletteSize;i++) {
			previous_palette[i] = palette[i];
		}

		// Take the average of mappings as the new palette
		for (int32 i=0;i<paletteSize;i++) {
			color_chain *entry = selected_colors[i];
			float entry_count = 0;
			float red = 0;
			float green = 0;
			float blue = 0;
			while (entry != NULL) {
				red += entry->color.red;
				green += entry->color.green;
				blue += entry->color.blue;
				
				entry_count++;	
				entry = entry->next;
			}
			if (entry_count > 0) {
				red /= entry_count;
				green /= entry_count;
				blue /= entry_count;
				
				palette[i].red = (uint8)red;
				palette[i].green = (uint8)green;
				palette[i].blue = (uint8)blue;
			}
			else {
			}
		}
		
		// Here we destroy the linked list
		for (int32 i=0;i<paletteSize;i++) {
			color_chain *entry = selected_colors[i];
			while (entry != NULL) {
				color_chain *spare_entry = entry->next;
				delete entry;
				entry = spare_entry;
			}
			selected_colors[i] = NULL;
		}
		
		// Here compare if the palette actually improved
		palette_still_improving = false;
		float error_amount = 0;
		for (int32 i=0;i<paletteSize;i++) {
//			if ((palette[i].red != previous_palette[i].red) ||
//				(palette[i].green != previous_palette[i].green) ||
//				(palette[i].blue != previous_palette[i].blue)) {				
//				palette_still_improving = true;
//			}
			error_amount += color_metric->color_distance(palette[i],previous_palette[i]);
		}		
		if (error_amount > 0)
			palette_still_improving = true;
	}

	printf("Number of iterations %d\n",number_of_iterations);
		
	delete previous_palette;
	delete color_metric;
	
	// Here destroy the input color array/lists
	
	
	
//	for (int32 i=0;i<paletteSize;i++) {
//		printf("%d\t%d\t%d\t%d\n",palette[i].red,palette[i].green,palette[i].blue,palette[i].alpha);
//	}
	
	return palette;
}