/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *	Array overflow fix 2017/7/18  Pete G.
 */
#include <OS.h>
#include <stdio.h>

#include "ImageProcessingLibrary.h"


status_t ImageProcessingLibrary::gaussian_blur(BBitmap *bitmap,float radius)
{
	int32 kernel_radius = int32(ceil(radius));
	float *kernel_array = new float[2*kernel_radius+1];
	float *kernel = &kernel_array[kernel_radius];
	float sum = 0;
	int32 *fixed_kernel_array = new int32[2*kernel_radius+1];
	int32 *fixed_kernel = &fixed_kernel_array[kernel_radius];

	// was before -log(0.002)
	float p = -log(0.004)/(pow(radius,2)*log(2));
	for (int32 i=-kernel_radius;i<=kernel_radius;i++) {
		kernel[i] = pow(2,-p*i*i);
		sum += kernel[i];
	}
	for (int32 i = -kernel_radius; i <= kernel_radius; i++) {
		kernel[i] /= sum;
		fixed_kernel[i] = kernel[i] * 32768;
	}

	BBitmap *intermediate;
	BRect bitmap_bounds = bitmap->Bounds();
	BRect intermediate_bounds;
	intermediate_bounds.left = bitmap_bounds.top;
	intermediate_bounds.top = bitmap_bounds.left;
	intermediate_bounds.right = bitmap_bounds.bottom;
	intermediate_bounds.bottom = bitmap_bounds.right;

	intermediate = new BBitmap(intermediate_bounds,B_RGB32,false);

	uint32 *b_bits = (uint32*)bitmap->Bits();
	uint32 *i_bits = (uint32*)intermediate->Bits();
	int32 b_bpr = bitmap->BytesPerRow()/4;
	int32 i_bpr = intermediate->BytesPerRow()/4;

	// Blur from bitmap to intermediate and rotate
	int32 width = bitmap_bounds.IntegerWidth() + 1;
	int32 height = bitmap_bounds.IntegerHeight() + 1;
	uint32 *source_array = new uint32[width + kernel_radius*2];
	uint32 *target_array = new uint32[width];


	for (int32 y=0;y<height;y++) {
		uint32 *source_array_position = source_array;
		for (int32 dx = 0;dx<kernel_radius;dx++) {
			*source_array_position++ = *b_bits;
		}
		for (int32 dx=0;dx<width;dx++) {
			*source_array_position++ = *b_bits++;
		}
		b_bits--;
		for (int32 dx = 0;dx<kernel_radius;dx++) {
			*source_array_position++ = *b_bits;
		}
		b_bits++;
		convolve_1d_fixed(source_array+kernel_radius,target_array,width,fixed_kernel,kernel_radius);

		uint32 *target_array_position = target_array;
		for (int32 x = 0;x<width;x++) {
			*(i_bits + (height-y-1) + x*i_bpr) = *target_array_position++;
		}

	}


	b_bits = (uint32*)bitmap->Bits();
	i_bits = (uint32*)intermediate->Bits();


	delete[] source_array;
	delete[] target_array;


	// Blur from intermediate to bitmap and rotate
	width = intermediate_bounds.IntegerWidth() + 1;
	height = intermediate_bounds.IntegerHeight() + 1;
	source_array = new uint32[width + kernel_radius*2];
	target_array = new uint32[width];

	for (int32 y=0;y<height;y++) {
		uint32 *source_array_position = source_array;
		for (int32 dx = 0;dx<kernel_radius;dx++) {
			*source_array_position++ = *i_bits;
		}
		for (int32 dx=0;dx<width;dx++) {
			*source_array_position++ = *i_bits++;
		}
		i_bits--;
		for (int32 dx = 0;dx<kernel_radius;dx++) {
			*source_array_position++ = *i_bits;
		}
		i_bits++;
		convolve_1d_fixed(source_array+kernel_radius,target_array,width,fixed_kernel,kernel_radius);

		uint32 *target_array_position = target_array;
		for (int32 dx = 0;dx<width;dx++) {
			*(b_bits + y + (width-dx-1)*b_bpr) = *target_array_position++;
		}

	}

	delete[] source_array;
	delete[] target_array;
	delete[] kernel_array;
	delete[] fixed_kernel_array;
	delete intermediate;

	return B_OK;
}


status_t ImageProcessingLibrary::gaussian_blur(BBitmap *bitmap,float radius, int32 threadCount)
{
	int32 kernel_radius = ceil(radius);
	float *kernel_array = new float[2*kernel_radius+1];
	float *kernel = &kernel_array[kernel_radius];
	float sum = 0;
	int32 *fixed_kernel_array = new int32[2*kernel_radius+1];
	int32 *fixed_kernel = &fixed_kernel_array[kernel_radius];

	float p = -log(0.004)/(pow(radius,2)*log(2));
	for (int32 i=-kernel_radius;i<=kernel_radius;i++) {
		kernel[i] = pow(2,-p*i*i);
		sum += kernel[i];
	}
	for (int32 i=-kernel_radius;i<=kernel_radius;i++) {
		kernel[i] /= sum;
		fixed_kernel[i] = kernel[i] * 32768;
	}


	// Use thread-count threads.
	// First blur horizontally and rotate by 90 clockwise.
	/*
		Rotation changes things like this
		*....		..*
		.....		...
		.....		...
					...
					...
	*/


	BRect bitmap_bounds = bitmap->Bounds();
	BRect intermediate_bounds(0,0,bitmap_bounds.Height(),bitmap_bounds.Width());
	BBitmap *intermediate = new BBitmap(intermediate_bounds,B_RGB32,false);

	int32 *s_bits = (int32*)bitmap->Bits();
	int32 s_bpr = bitmap->BytesPerRow()/4;

	int32 *d_bits = (int32*)intermediate->Bits();
	int32 d_bpr = intermediate->BytesPerRow()/4;

	// Start the threads.
	thread_id blur_thread_array[8]; //TODO - Was B_MAX_CPU_COUNT, find way to decide amount of threads to use based on cpu count
	for (int32 i=0;i<threadCount;i++) {
		int32 height = bitmap_bounds.Height() / threadCount + 1;
		filter_thread_data *data = new filter_thread_data();
		data->left = 0;
		data->right = bitmap_bounds.right;
		int32 top;
		top = data->top = min_c(bitmap_bounds.bottom,bitmap_bounds.top + i*height);
		data->bottom = min_c(bitmap_bounds.bottom,top+height-1);

		data->s_bpr = s_bpr;
		data->s_bits = s_bits + top*s_bpr;
		data->d_bpr = d_bpr;
		data->d_bits = d_bits;

		data->kernel = fixed_kernel;
		data->kernel_radius = kernel_radius;

		blur_thread_array[i] = spawn_thread(start_filter_1d_thread_clockwise,"filter_1d_thread",B_NORMAL_PRIORITY,data);
		resume_thread(blur_thread_array[i]);
	}

	for (int32 i=0;i<threadCount;i++) {
		int32 return_value;
		wait_for_thread(blur_thread_array[i],&return_value);
	}

	s_bits = (int32*)intermediate->Bits();
	s_bpr = intermediate->BytesPerRow() / 4;

	d_bits = (int32*)bitmap->Bits();
	d_bpr = bitmap->BytesPerRow()/4;

	// Here blur from intermediate to bitmap horizontally and rotate
	// by 90 degrees counterclockwise
	// Start the threads.
	for (int32 i=0;i<threadCount;i++) {
		int32 height = intermediate_bounds.Height() / threadCount + 1;
		filter_thread_data *data = new filter_thread_data();
		data->left = 0;
		data->right = intermediate_bounds.right;
		int32 top;
		top = data->top = min_c(intermediate_bounds.bottom,intermediate_bounds.top + i*height);
		data->bottom = min_c(intermediate_bounds.bottom,top+height-1);

		data->s_bpr = s_bpr;
		data->s_bits = s_bits + top*s_bpr;
		data->d_bpr = d_bpr;
		data->d_bits = d_bits;

		data->kernel = fixed_kernel;
		data->kernel_radius = kernel_radius;

		blur_thread_array[i] = spawn_thread(start_filter_1d_thread_counterclockwise,"filter_1d_thread",B_NORMAL_PRIORITY,data);
		resume_thread(blur_thread_array[i]);
	}

	for (int32 i=0;i<threadCount;i++) {
		int32 return_value;
		wait_for_thread(blur_thread_array[i],&return_value);
	}

	return B_OK;
}

int32 ImageProcessingLibrary::start_filter_1d_thread_clockwise(void *d)
{
	filter_thread_data *data = (filter_thread_data*)d;

	filter_1d_and_rotate_clockwise(data->s_bits,data->s_bpr,data->d_bits,data->d_bpr,
						 data->left,data->right,data->top,data->bottom,data->kernel,data->kernel_radius);

	delete data;

	return B_OK;
}


int32 ImageProcessingLibrary::start_filter_1d_thread_counterclockwise(void *d)
{
	filter_thread_data *data = (filter_thread_data*)d;

	filter_1d_and_rotate_counterclockwise(data->s_bits,data->s_bpr,data->d_bits,data->d_bpr,
						 data->left,data->right,data->top,data->bottom,data->kernel,data->kernel_radius);

	delete data;

	return B_OK;
}


void ImageProcessingLibrary::filter_1d_and_rotate_clockwise(int32 *s_bits,int32 s_bpr,int32 *d_bits,int32 d_bpr,int32 left,int32 right,int32 top,int32 bottom,int32 *kernel,int32 kernel_radius)
{
	int32 width = right - left + 1;
	uint32 *source_array = new uint32[width + kernel_radius*2];
	uint32 *target_array = new uint32[width];

	for (int32 y=top;y<=bottom;++y) {
		uint32 *source_array_position = source_array;
		for (int32 dx = 0;dx<kernel_radius;++dx) {
			*source_array_position++ = *s_bits;
		}
		for (int32 dx=left;dx<=right;++dx) {
			*source_array_position++ = *s_bits++;
		}
		s_bits--;
		for (int32 dx = 0;dx<kernel_radius;++dx) {
			*source_array_position++ = *s_bits;
		}
		s_bits++;

		convolve_1d_fixed(source_array+kernel_radius,target_array,width,kernel,kernel_radius);

		uint32 *target_array_position = target_array;
		for (int32 x = left;x<=right;x++) {
			*(d_bits + (d_bpr-1-y) + x*d_bpr) = *target_array_position++;
		}
	}

	delete[] source_array;
	delete[] target_array;
}


void ImageProcessingLibrary::filter_1d_and_rotate_counterclockwise(int32 *s_bits,int32 s_bpr,int32 *d_bits,int32 d_bpr,int32 left,int32 right,int32 top,int32 bottom,int32 *kernel,int32 kernel_radius)
{
	int32 width = right - left + 1;
	uint32 *source_array = new uint32[width + kernel_radius*2];
	uint32 *target_array = new uint32[width];


	for (int32 y=top;y<=bottom;++y) {
		uint32 *source_array_position = source_array;
		for (int32 dx = 0;dx<kernel_radius;++dx) {
			*source_array_position++ = *s_bits;
		}
		for (int32 dx=left;dx<=right;++dx) {
			*source_array_position++ = *s_bits++;
		}
		s_bits--;
		for (int32 dx = 0;dx<kernel_radius;++dx) {
			*source_array_position++ = *s_bits;
		}
		s_bits++;

		convolve_1d_fixed(source_array+kernel_radius,target_array,width,kernel,kernel_radius);

		uint32 *target_array_position = target_array;
		for (int32 x = left;x<=right;x++) {
			*(d_bits + y + (right-x)*d_bpr) = *target_array_position++;
		}
	}

	delete[] source_array;
	delete[] target_array;
}


void ImageProcessingLibrary::convolve_1d(uint32 *s, uint32 *t,int32 length,float *kernel,int32 kernel_radius)
{
	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	for (int32 x=0;x<length;x++) {
		float red=0;
		float green=0;
		float blue=0;
		float alpha=0;
		for (int32 index = -kernel_radius;index<=kernel_radius;index++) {
			c.word = *(s + index);
			red += c.bytes[2] * kernel[index];
			green += c.bytes[1] * kernel[index];
			blue += c.bytes[0] * kernel[index];
			alpha += c.bytes[3] * kernel[index];
		}

		c.bytes[0] = blue;
		c.bytes[1] = green;
		c.bytes[2] = red;
		c.bytes[3] = alpha;

		*t++ = c.word;
		s++;
	}
}


void ImageProcessingLibrary::convolve_1d_fixed(uint32 *s, uint32 *t,int32 length,int32 *kernel,int32 kernel_radius)
{
	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	for (register int32 x=0;x<length;++x) {
		int32 red=0;
		int32 green=0;
		int32 blue=0;
		int32 alpha=0;
		uint32 *tmp = s-kernel_radius;
		int32 *kernel_tmp = kernel-kernel_radius;
		for (int32 index = -kernel_radius;index<=kernel_radius;++index) {
			c.word = *tmp++;
			if (c.bytes[3] != 0) {
				blue += (c.bytes[0] * *kernel_tmp);
				green += (c.bytes[1] * *kernel_tmp);
				red += (c.bytes[2] * *kernel_tmp);
			}
			alpha += (c.bytes[3] * *kernel_tmp++);
		}

		c.bytes[0] = blue >> 15;
		c.bytes[1] = green >> 15;
		c.bytes[2] = red >> 15;
		c.bytes[3] = alpha >> 15;

		*t++ = c.word;
		s++;
	}
}








/*

Grayscale AHE

*/

status_t ImageProcessingLibrary::grayscale_ahe(BBitmap *bitmap, int32 regionSize)
{
	int32 right,bottom;
	right = bitmap->Bounds().right;
	bottom = bitmap->Bounds().bottom;

	int32 region_half = regionSize / 2;

	uint8 **prev_histograms;
	uint8 **next_histograms;

	int32 histogram_count_per_row = right / regionSize + 1 + 2; // extras at the edges

	prev_histograms = new uint8*[histogram_count_per_row];
	next_histograms = new uint8*[histogram_count_per_row];

	for (int32 i=0;i<histogram_count_per_row;i++) {
		prev_histograms[i] = new uint8[256];
		next_histograms[i] = new uint8[256];
	}

	int32 corner_y = 0;
	int32 corner_x = 0;
	// calculate the initial row of histograms
	for (int32 i=0;i<histogram_count_per_row;i++) {
		if (i == 1) {
			for (int32 j=0;j<256;j++) {
				next_histograms[1][j] = next_histograms[0][j];
			}
		}
		else if(i == histogram_count_per_row-1) {
			for (int32 j=0;j<256;j++) {
				next_histograms[i][j] = next_histograms[i][j];
			}
		}
		else {
			calculate_local_mapping_function(bitmap,corner_x,corner_y,regionSize,next_histograms[i]);
			corner_x = corner_x + regionSize;
		}
	}

	// make a copy of it
	for (int32 i=0;i<histogram_count_per_row;i++) {
		for (int32 j=0;j<256;j++) {
			prev_histograms[i][j] = next_histograms[i][j];
		}
	}


	// loop through the picture
	uint32 *bits = (uint32*)bitmap->Bits();
	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	for (int32 y=0;y<=bottom;y++) {
		if (y % regionSize == region_half) {
			corner_y += regionSize;
			if (corner_y > bottom) {
				for (int32 i=0;i<histogram_count_per_row;i++) {
					for (int32 j=0;j<256;j++) {
						prev_histograms[i][j] = next_histograms[i][j];
					}
				}
			}
			else {
				uint8 **spare = prev_histograms;
				prev_histograms = next_histograms;

				next_histograms = spare;

				corner_x = 0;

				// calculate the next row of histograms
				for (int32 i=0;i<histogram_count_per_row;i++) {
					if (i == 1) {
						for (int32 j=0;j<256;j++) {
							next_histograms[1][j] = next_histograms[0][j];
						}
					}
					else if(i == histogram_count_per_row-1) {
						for (int32 j=0;j<256;j++) {
							next_histograms[i][j] = next_histograms[i][j];
						}
					}
					else {
						calculate_local_mapping_function(bitmap,corner_x,corner_y,regionSize,next_histograms[i]);
						corner_x = corner_x + regionSize;
					}
				}
			}
		}

		int32 current_bin = 1;
		int32 x_dist = region_half;
		int32 y_dist = region_half;	// distance to the previous center
		uint8 hist_index;
		uint8 map_value;

		float left_coeff,right_coeff,top_coeff,bottom_coeff;

		for (int32 x=0;x<=right;x++) {
			if (x % regionSize == region_half)
				current_bin++;

			if (x % regionSize < region_half) {
				x_dist = x % regionSize + region_half;
			}
			else {
				x_dist = x % regionSize - region_half;
			}

			if (y % regionSize < region_half) {
				y_dist = y % regionSize + region_half;
			}
			else {
				y_dist = y % regionSize - region_half;
			}

			// the bins are prev...[current_bin], prev...[current_bin-1] and the same for next

			c.word = *bits;
			hist_index = c.bytes[0];

			right_coeff = (float)x_dist / (float)regionSize;
			bottom_coeff = (float)y_dist / (float)regionSize;
			left_coeff = 1.0 - right_coeff;
			top_coeff = 1.0 - bottom_coeff;

			map_value = left_coeff*(top_coeff*prev_histograms[current_bin-1][hist_index] +
									bottom_coeff*next_histograms[current_bin-1][hist_index]) +
						right_coeff*(top_coeff*prev_histograms[current_bin][hist_index] +
									 bottom_coeff*next_histograms[current_bin][hist_index]);

			c.bytes[0] = c.bytes[1] = c.bytes[2] = map_value;
			*bits++ = c.word;
		}
	}

	// clean up...
	for (int32 i=0;i<histogram_count_per_row;i++) {
		delete[] prev_histograms[i];
		delete[] next_histograms[i];
		prev_histograms[i] = NULL;
		next_histograms[i] = NULL;
	}

	delete[] prev_histograms;
	delete[] next_histograms;

	return B_OK;
}




status_t ImageProcessingLibrary::grayscale_clahe(BBitmap *bitmap, int32 regionSize,int32 clipLimit)
{
	int32 right,bottom;
	right = bitmap->Bounds().right;
	bottom = bitmap->Bounds().bottom;

	int32 region_half = regionSize / 2;

	uint8 **prev_histograms;
	uint8 **next_histograms;

	int32 histogram_count_per_row = right / regionSize + 1 + 2; // extras at the edges

	prev_histograms = new uint8*[histogram_count_per_row];
	next_histograms = new uint8*[histogram_count_per_row];

	for (int32 i=0;i<histogram_count_per_row;i++) {
		prev_histograms[i] = new uint8[256];
		next_histograms[i] = new uint8[256];
	}

	int32 corner_y = 0;
	int32 corner_x = 0;
	// calculate the initial row of histograms
	for (int32 i=0;i<histogram_count_per_row;i++) {
		if (i == 1) {
			for (int32 j=0;j<256;j++) {
				next_histograms[1][j] = next_histograms[0][j];
			}
		}
		else if(i == histogram_count_per_row-1) {
			for (int32 j=0;j<256;j++) {
				next_histograms[i][j] = next_histograms[i][j];
			}
		}
		else {
			calculate_local_mapping_function_clip(bitmap,corner_x,corner_y,regionSize,clipLimit,next_histograms[i]);
			corner_x = corner_x + regionSize;
		}
	}

	// make a copy of it
	for (int32 i=0;i<histogram_count_per_row;i++) {
		for (int32 j=0;j<256;j++) {
			prev_histograms[i][j] = next_histograms[i][j];
		}
	}


	// loop through the picture
	uint32 *bits = (uint32*)bitmap->Bits();
	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	for (int32 y=0;y<=bottom;y++) {
		if (y % regionSize == region_half) {
			corner_y += regionSize;
			if (corner_y > bottom) {
				for (int32 i=0;i<histogram_count_per_row;i++) {
					for (int32 j=0;j<256;j++) {
						prev_histograms[i][j] = next_histograms[i][j];
					}
				}
			}
			else {
				uint8 **spare = prev_histograms;
				prev_histograms = next_histograms;

				next_histograms = spare;

				corner_x = 0;

				// calculate the next row of histograms
				for (int32 i=0;i<histogram_count_per_row;i++) {
					if (i == 1) {
						for (int32 j=0;j<256;j++) {
							next_histograms[1][j] = next_histograms[0][j];
						}
					}
					else if(i == histogram_count_per_row-1) {
						for (int32 j=0;j<256;j++) {
							next_histograms[i][j] = next_histograms[i][j];
						}
					}
					else {
						calculate_local_mapping_function_clip(bitmap,corner_x,corner_y,regionSize,clipLimit,next_histograms[i]);
						corner_x = corner_x + regionSize;
					}
				}
			}
		}

		int32 current_bin = 1;
		int32 x_dist = region_half;
		int32 y_dist = region_half;	// distance to the previous center
		uint8 hist_index;
		uint8 map_value;

		float left_coeff,right_coeff,top_coeff,bottom_coeff;

		for (int32 x=0;x<=right;x++) {
			if (x % regionSize == region_half)
				current_bin++;

			if (x % regionSize < region_half) {
				x_dist = x % regionSize + region_half;
			}
			else {
				x_dist = x % regionSize - region_half;
			}

			if (y % regionSize < region_half) {
				y_dist = y % regionSize + region_half;
			}
			else {
				y_dist = y % regionSize - region_half;
			}

			// the bins are prev...[current_bin], prev...[current_bin-1] and the same for next

			c.word = *bits;
			hist_index = c.bytes[0];

			right_coeff = (float)x_dist / (float)regionSize;
			bottom_coeff = (float)y_dist / (float)regionSize;
			left_coeff = 1.0 - right_coeff;
			top_coeff = 1.0 - bottom_coeff;

			map_value = left_coeff*(top_coeff*prev_histograms[current_bin-1][hist_index] +
									bottom_coeff*next_histograms[current_bin-1][hist_index]) +
						right_coeff*(top_coeff*prev_histograms[current_bin][hist_index] +
									 bottom_coeff*next_histograms[current_bin][hist_index]);

			c.bytes[0] = c.bytes[1] = c.bytes[2] = map_value;
			*bits++ = c.word;
		}
	}

	// clean up...
	for (int32 i=0;i<histogram_count_per_row;i++) {
		delete[] prev_histograms[i];
		delete[] next_histograms[i];
		prev_histograms[i] = NULL;
		next_histograms[i] = NULL;
	}

	delete[] prev_histograms;
	delete[] next_histograms;

	return B_OK;
}




void ImageProcessingLibrary::calculate_local_mapping_function(BBitmap *bitmap,int32 cx, int32 cy,int32 regionSize,uint8 *mapFunction)
{
	int32 left,right,top,bottom;
	left = cx;
	top = cy;
	right = min_c(left+regionSize-1,bitmap->Bounds().right);
	bottom = min_c(top+regionSize-1,bitmap->Bounds().bottom);


	uint32 *bits = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow()/4;

	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	// calculate the histogram
	int32 histogram[256];
	for (int32 i=0;i<256;i++) {
		histogram[i] = 0;
	}

	for (int32 y=top;y<=bottom;y++) {
		for (int32 x=left;x<=right;x++) {
			c.word = *(bits + x + y*bpr);
			histogram[c.bytes[0]]++;
		}
	}

	// make the histogram cumulative
	for (int32 i=1;i<256;i++) {
		histogram[i] = histogram[i] + histogram[i-1];
	}


	// normalize cumulative histogram to create mapping function
	float multiplier = 255.0 / (float)histogram[255];

	for (int32 i=0;i<256;i++) {
		mapFunction[i] = floor(histogram[i]*multiplier);
	}
}



void ImageProcessingLibrary::calculate_local_mapping_function_clip(BBitmap *bitmap,int32 cx, int32 cy,int32 regionSize,int32 clipLimit,uint8 *mapFunction)
{
	int32 left,right,top,bottom;
	left = cx;
	top = cy;
	right = min_c(left+regionSize-1,bitmap->Bounds().right);
	bottom = min_c(top+regionSize-1,bitmap->Bounds().bottom);

	uint32 *bits = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow()/4;

	union {
		uint8 bytes[4];
		uint32 word;
	} c;

	// calculate the histogram
	int32 histogram[256];
	for (int32 i=0;i<256;i++) {
		histogram[i] = 0;
	}

	for (int32 y=top;y<=bottom;y++) {
		for (int32 x=left;x<=right;x++) {
			c.word = *(bits + x + y*bpr);
			histogram[c.bytes[0]]++;
		}
	}


	// clip the histogram
	int32 actual_clip_limit;

	top = clipLimit;
	bottom = 0;
	int32 middle;
	int32 S;
	while ((top - bottom) > 1) {
		middle = (top + bottom) / 2;
		S = 0;
		for (int32 i=0;i<256;i++) {
			if (histogram[i] > middle)
				S += histogram[i] - middle;
		}
		if (S > (clipLimit-middle)*256) {
			top = middle;
		}
		else {
			bottom = middle;
		}
	}

	actual_clip_limit = bottom + S/256;
	int32 L = clipLimit - actual_clip_limit;

	for (int32 i=0;i<256;i++) {
		if (histogram[i] >= actual_clip_limit)
			histogram[i] = clipLimit;
		else
			histogram[i] += L;
	}



	// make the histogram cumulative
	for (int32 i=1;i<256;i++) {
		histogram[i] = histogram[i] + histogram[i-1];
	}


	// normalize cumulative histogram to create mapping function
	float multiplier = 255.0 / (float)histogram[255];

	for (int32 i=0;i<256;i++) {
		mapFunction[i] = floor(histogram[i]*multiplier);
	}

}
