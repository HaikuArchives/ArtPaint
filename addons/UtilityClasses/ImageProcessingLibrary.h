/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Rect.h>

#ifndef	IMAGE_PROCESSING_LIBRARY_H
#define	IMAGE_PROCESSING_LIBRARY_H


class ImageProcessingLibrary {
public:
status_t	gaussian_blur(BBitmap *bitmap,float radius);
status_t	gaussian_blur(BBitmap *bitmap,float radius,int32 thread_count);

status_t	box_blur(BBitmap* bitmap, float radius, int32 thread_count);

status_t	fast_gaussian_blur(BBitmap* bitmap, float radius);
status_t	fast_gaussian_blur(BBitmap* bitmap, float radius, int32 thread_count);

status_t	grayscale_ahe(BBitmap *bitmap,int32 regionSize);
status_t	grayscale_clahe(BBitmap *bitmap,int32 regionSize,int32 clipLimit);

private:
// gaussian blur stuff
static	void		convolve_1d(uint32 *s,uint32 *t,int32 length,float *kernel,int32 kernel_radius);
static	void		convolve_1d_fixed(uint32 *s,uint32 *t,int32 length,int32 *kernel,int32 kernel_radius);

static	int32		start_filter_1d_thread_clockwise(void*);
static	int32		start_filter_1d_thread_counterclockwise(void*);

static	int32		start_filter_box_blur_thread_h(void*);
static	int32		start_filter_box_blur_thread_t(void*);

static	void		filter_1d_and_rotate_clockwise(int32 *s_bits,int32 s_bpr,int32 *d_bits,int32 d_bpr,int32 left,int32 right,int32 top,int32 bottom,int32 *kernel,int32 kernel_radius);
static	void		filter_1d_and_rotate_counterclockwise(int32 *s_bits,int32 s_bpr,int32 *d_bits,int32 d_bpr,int32 left,int32 right,int32 top,int32 bottom,int32 *kernel,int32 kernel_radius);

static void			box_blur_h(int32* s_bits, int32* d_bits, int32 width, int32 height,
						int32 kernel_radius);
static void			box_blur_t(int32* s_bits, int32* d_bits, int32 width, int32 height,
						int32 kernel_radius);


// grayscale ahe stuff
static	void		calculate_local_mapping_function(BBitmap *bitmap,
						int32 cx, int32 cy,int32 regionSize,uint8 *mapFunction);

static	void		calculate_local_mapping_function_clip(BBitmap *bitmap,
						int32 cx, int32 cy,int32 regionSize,int32 clipLimit,
						uint8 *mapFunction);

};


struct filter_thread_data {
	int32	*s_bits;
	int32	s_bpr;
	int32	*d_bits;
	int32	d_bpr;
	int32	left;
	int32	right;
	int32	top;
	int32	bottom;
	int32	*kernel;
	int32	kernel_radius;
};

#endif
