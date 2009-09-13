/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <math.h>
#include <StatusBar.h>
#include <StopWatch.h>
#include <Slider.h>
#include <Window.h>

#include "AddOns.h"
#include "Wave.h"
#include "PixelOperations.h"

#define PI M_PI


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Wave…";
	char menu_help_string[255] = "Starts waving the active layer.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = DISTORT_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	// Here create a view-manipulator. The class should inherit
	// from the ViewManipulator base-class. It will be deleted
	// in the application program.
	delete i;
	return new WaveManipulator(bm);
}



WaveManipulator::WaveManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	last_calculated_resolution = 8;
	lowest_available_quality = 8;

	copy_of_the_preview_bitmap = NULL;
	preview_bitmap = NULL;

	SetPreviewBitmap(bm);

	sin_table = new float[720];
	for (int32 i=0;i<720;i++)
		sin_table[i] = sin((float)i/720.0*2*PI);
}


WaveManipulator::~WaveManipulator()
{
	delete[] sin_table;
	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
		config_view = NULL;
	}
}


BBitmap* WaveManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	WaveManipulatorSettings *new_settings = dynamic_cast<WaveManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	BBitmap *source_bitmap;
	BBitmap *target_bitmap;
	BBitmap *new_bitmap=NULL;

	if (original == preview_bitmap) {
		target_bitmap = original;
		source_bitmap = copy_of_the_preview_bitmap;
	}
	else {
		target_bitmap = original;
		new_bitmap =  DuplicateBitmap(original,0);
		source_bitmap = new_bitmap;
	}


	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;
	int32 target_bpr = target_bitmap->BytesPerRow()/4;

	int32 start_y = target_bitmap->Bounds().top;
	int32 end_y = target_bitmap->Bounds().bottom;

	target_bits += start_y * target_bpr;

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	float s = new_settings->wave_length;
	float A = new_settings->wave_amount;
	float k = 0;
	float dx,dy;
	register float sqrt_x_plus_y;
	register float one_per_sqrt_x_plus_y;
	float cx,cy;
	int32 top,bottom;
	top = start_y;
	bottom = end_y;

	cx = floor(new_settings->center.x);
	cy = floor(new_settings->center.y);
	float R = sqrt(pow(max_c(fabs(cx),target_bpr-fabs(cx)),2)+pow(max_c(fabs(cy),end_y-fabs(cy)),2));
	float one_per_R = 1.0/R;

	float real_x,real_y;
	register float two_pi_per_s = 2*PI/s;
	register float two_360_per_s = 720.0/s;

	union {
		uint8 bytes[4];
		uint32 word;
	} background;

	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	if ((selection == NULL) || (selection->IsEmpty())) {
		uint32 p1,p2,p3,p4;
		float float_end_y = end_y;
		float float_target_bpr = target_bpr;
	 	for (float y=start_y;y<=float_end_y;y++) {
	 		for (float x=0;x<float_target_bpr;x++) {
				real_x = x-cx;
				real_y = y-cy;
				uint32 target_value = 0x00000000;
				if ((real_x != 0) && (real_y != 0)) {
					sqrt_x_plus_y = sqrt(real_x*real_x+real_y*real_y);
					dx = real_x/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);	// The sin and div are slow.
					dx = dx*(1-k*sqrt_x_plus_y*one_per_R);
					dy = dx/real_x*real_y;
					int32 ceil_y = ceil(y+dy);
					int32 floor_y = ceil_y -1;
					int32 floor_x = floor(x+dx);
					int32 ceil_x = floor_x+1;

					float v = ceil_y-(y+dy);
					float u = (x+dx)-floor_x;
					if ((ceil_y <= bottom) && (ceil_y>=top)) {
						if ((floor_x >= 0) && (floor_x <source_bpr)) {
							p1 = *(source_bits + ceil_y*source_bpr + floor_x);
						}
						else
							p1 = background.word;

						if ((ceil_x >= 0) && (ceil_x <source_bpr)) {
							p2 = *(source_bits + ceil_y*source_bpr + ceil_x);
						}
						else
							p2 = background.word;
					}
					else {
						p1 = background.word;
						p2 = background.word;
					}
					if ((floor_y <= bottom) && (floor_y>=top)) {
						if ((floor_x >= 0) && (floor_x <source_bpr)) {
							p3 = *(source_bits + floor_y*source_bpr + floor_x);
						}
						else
							p3 = background.word;

						if ((ceil_x >= 0) && (ceil_x <source_bpr)) {
							p4 = *(source_bits + floor_y*source_bpr + ceil_x);
						}
						else
							p4 = background.word;
					}
					else {
						p3 = background.word;
						p4 = background.word;
					}
					*target_bits++ = bilinear_interpolation(p1,p2,p3,p4,u,v);
				}
				else {
					if (real_x != 0) {
						sqrt_x_plus_y = sqrt(real_x*real_x);
						dx =  real_x/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);
						dx = dx*(1-k*sqrt_x_plus_y/R);
						int32 ceil_y = ceil(y+dy);
						int32 floor_y = ceil_y -1;
						int32 floor_x = floor(x+dx);
						int32 ceil_x = floor_x+1;
						float x_mix_right = (x+dx)-floor_x;
						if ((ceil_x >= 0) && (ceil_x<target_bpr))
							p1 = *(source_bits + (int32)y*target_bpr + ceil_x);
						else
							p1 = background.word;

						if ((floor_x >= 0) && (floor_x<target_bpr))
							p2 = *(source_bits + (int32)y*target_bpr + floor_x);
						else
							p2 = background.word;
						*target_bits++ = mix_2_pixels_fixed(p1,p2,32768*x_mix_right);
					}
					else if (real_y != 0) {
						sqrt_x_plus_y = sqrt(real_y*real_y);
						dy = real_y/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);
						dy = dy*(1-k*sqrt_x_plus_y/R);
						int32 ceil_y = ceil(y+dy);
						int32 floor_y = ceil_y -1;
						int32 floor_x = floor(x+dx);
						int32 ceil_x = floor_x+1;
						float y_mix_upper = ceil(y+dy)-(y+dy);
						if ((ceil_y<=bottom) && (ceil_y >= top))
							p1 = *(source_bits + (int32)x + ceil_y*target_bpr);
						else
							p1 = background.word;

						if ((floor_y<=bottom) && (floor_y >= top))
							p2 = *(source_bits + (int32)x + floor_y*target_bpr);
						else
							p2 = background.word;
							*target_bits++ = mix_2_pixels_fixed(p2,p1,32768*y_mix_upper);
					}
					else {
						*target_bits++ = *(source_bits + (int32)x + (int32)y*source_bpr);
					}
				}

			}
	 		// Send a progress-message if required
	 		if ((status_bar != NULL) && ((int32)y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",100.0/(float)(end_y-start_y)*10.0);
				status_bar->Window()->PostMessage(&progress_message,status_bar);
	 		}
	 	}
	}
	else {
		uint32 p1,p2,p3,p4;
	 	for (float y=start_y;y<=end_y;y++) {
	 		for (float x=0;x<target_bpr;x++) {
				if (selection->ContainsPoint(x,y) == TRUE) {
					real_x = x-cx;
					real_y = y-cy;
					uint32 target_value = 0x00000000;
					if ((real_x != 0) && (real_y != 0)) {
						sqrt_x_plus_y = sqrt(real_x*real_x+real_y*real_y);
						dx = real_x/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);	// The sin and div are slow.
						dx = dx*(1-k*sqrt_x_plus_y/R);
						dy = dx/real_x*real_y;
						float y_mix_upper = ceil(y+dy)-(y+dy);
						float x_mix_right = (x+dx)- floor(x+dx);
						if ((ceil(y+dy) <= bottom) && (ceil(y+dy)>=top)) {
							if ((floor(x+dx) >= 0) && (floor(x+dx) <source_bpr)) {
								p1 = *(source_bits + (int32)ceil(y+dy)*source_bpr + (int32)floor(x+dx));
							}
							else
								p1 = background.word;

							if ((ceil(x+dx) >= 0) && (ceil(x+dx) <source_bpr)) {
								p2 = *(source_bits + (int32)ceil(y+dy)*source_bpr + (int32)ceil(x+dx));
							}
							else
								p2 = background.word;
						}
						else {
							p1 = background.word;
							p2 = background.word;
						}
						if ((floor(y+dy) <= bottom) && (floor(y+dy)>=top)) {
							if ((floor(x+dx) >= 0) && (floor(x+dx) <source_bpr)) {
								p3 = *(source_bits + (int32)floor(y+dy)*source_bpr + (int32)floor(x+dx));
							}
							else
								p3 = background.word;

							if ((ceil(x+dx) >= 0) && (ceil(x+dx) <source_bpr)) {
								p4 = *(source_bits + (int32)floor(y+dy)*source_bpr + (int32)ceil(x+dx));
							}
							else
								p4 = background.word;
						}
						else {
							p3 = background.word;
							p4 = background.word;
						}
						*target_bits++ = combine_4_pixels(p1,p2,p3,p4,(1-y_mix_upper)*(1-x_mix_right),(1-y_mix_upper)*(x_mix_right),(y_mix_upper)*(1-x_mix_right),(y_mix_upper)*(x_mix_right));
					}
					else {
						if (real_x != 0) {
							sqrt_x_plus_y = sqrt(real_x*real_x);
							dx =  real_x/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);
							dx = dx*(1-k*sqrt_x_plus_y/R);
							float x_mix_right = (x+dx)-floor(x+dx);
							if ((ceil(x+dx) >= 0) && (ceil(x+dx)<target_bpr))
								p1 = *(source_bits + (int32)y*target_bpr + (int32)ceil((x +dx)));
							else
								p1 = background.word;

							if ((floor(x+dx) >= 0) && (floor(x+dx)<target_bpr))
								p2 = *(source_bits + (int32)y*target_bpr + (int32)floor((x +dx)));
							else
								p2 = background.word;
							*target_bits++ = mix_2_pixels_fixed(p1,p2,32768*x_mix_right);
						}
						else if (real_y != 0) {
							sqrt_x_plus_y = sqrt(real_y*real_y);
							dy = real_y/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);
							dy = dy*(1-k*sqrt_x_plus_y/R);
							float y_mix_upper = ceil(y+dy)-(y+dy);
							if ((ceil(y+dy)<=bottom) && (ceil(y+dy) >= top))
								p1 = *(source_bits + (int32)x + (int32)ceil((y + dy))*target_bpr);
							else
								p1 = background.word;

							if ((floor(y+dy)<=bottom) && (floor(y+dy) >= top))
								p2 = *(source_bits + (int32)x + (int32)floor((y + dy))*target_bpr);
							else
								p2 = background.word;
								*target_bits++ = mix_2_pixels_fixed(p2,p1,32768*y_mix_upper);
						}
						else {
							*target_bits++ = *(source_bits + (int32)x + (int32)y*source_bpr);
						}
					}
				}
				else {
					*target_bits++ = *(source_bits + (int32)x + (int32)y*source_bpr);
				}
			}
	 		// Send a progress-message if required
	 		if ((status_bar != NULL) && ((int32)y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",100.0/(float)(end_y-start_y)*10.0);
				status_bar->Window()->PostMessage(&progress_message,status_bar);
	 		}
	 	}
	}

	if (new_bitmap != NULL) {
		delete new_bitmap;
	}

	return original;
}

int32 WaveManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	if ((settings == previous_settings) == FALSE) {
		previous_settings = settings;
		last_calculated_resolution = lowest_available_quality;
	}
	else {
		last_calculated_resolution = max_c(highest_available_quality,floor(last_calculated_resolution/2.0));
	}
	if (full_quality == TRUE)
		last_calculated_resolution = min_c(last_calculated_resolution,1);

	uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	uint32 *target_bits = (uint32*)preview_bitmap->Bits();
	int32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow()/4;
	int32 target_bpr = preview_bitmap->BytesPerRow()/4;

	int32 start_y = preview_bitmap->Bounds().top;
	int32 end_y = preview_bitmap->Bounds().bottom;

	union {
		uint8 bytes[4];
		uint32 word;
	} background;

	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;

	if (last_calculated_resolution > 0) {
		float real_x;
		float real_y;
		float one_per_sqrt_x_plus_y;
		float sqrt_x_plus_y;
		float cx = settings.center.x;
		float cy = settings.center.y;
		float dx;
		float dy;
		float A = settings.wave_amount;
		float s = settings.wave_length;
		float two_360_per_s = 720.0/s;
		float k = 0;
		float R = sqrt(pow(max_c(fabs(cx),target_bpr-fabs(cx)),2)+pow(max_c(fabs(cy),end_y-fabs(cy)),2));
		if (selection->IsEmpty()) {
		 	for (float y=start_y;y<=end_y;y += last_calculated_resolution) {
				int32 y_target_bpr = y*target_bpr;
		 		for (float x=0;x<source_bpr;x += last_calculated_resolution) {
					real_x = x-cx;
					real_y = y-cy;
					if ((real_x != 0) && (real_y != 0)) {
						// Let's try the inline assemler function for square root estimation.
						// On intel this does not yet work and the corresponding function is very slow.
						#if __POWERPC__
						one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x+real_y*real_y);
						sqrt_x_plus_y = 1.0/one_per_sqrt_x_plus_y;
						#elif __INTEL__
//						sqrt_x_plus_y = fsqrt(real_x*real_x+real_y*real_y);
						sqrt_x_plus_y = sqrt(real_x*real_x+real_y*real_y);
						one_per_sqrt_x_plus_y = 1.0 / sqrt_x_plus_y;
						#endif

						dx = one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720];
						dx = dx*(1-k*sqrt_x_plus_y/R);
						dy = dx*real_y;
						dx = dx*real_x;
						// This if is quite slow.
						if (((y+dy) <= end_y) && ((y+dy)>=0) && ((x+dx) >= 0) && ((x+dx)<target_bpr)) {
							*(target_bits + (int32)x + y_target_bpr) = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
						}
						else {
							*(target_bits +(int32)x + y_target_bpr) = background.word;
						}
					}
					else {
						if (real_x != 0) {
							// We also have to estimate the sqrt here.
							one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x);
							sqrt_x_plus_y = 1/one_per_sqrt_x_plus_y;
							dx =  one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720]*real_x;
							dx = dx*(1-k*sqrt_x_plus_y/R);
							if (((x+dx) >= 0) && ((x+dx)<target_bpr))
								*(target_bits +(int32)x +y_target_bpr) = *(source_bits + (int32)y*target_bpr + (int32)(x +dx));
							else
								*(target_bits +(int32)x + y_target_bpr)= background.word;
						}
						else if (real_y != 0) {
							one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_y*real_y);
							sqrt_x_plus_y = 1/one_per_sqrt_x_plus_y;
							dy =  one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720]*real_y;
							dy = dy*(1-k*sqrt_x_plus_y/R);
							if (((y+dy)<=end_y) && ((y+dy) >= 0))
								*(target_bits +(int32)x + y_target_bpr) = *(source_bits + (int32)x + (int32)(y + dy)*source_bpr);
							else
								*(target_bits +(int32)x +y_target_bpr) = background.word;
						}
						else {
							*(target_bits +(int32)x +y_target_bpr) = *(source_bits + (int32)x + (int32)y*source_bpr);
						}
					}
				}
		 	}
		}
		else {
		 	for (float y=start_y;y<=end_y;y += last_calculated_resolution) {
				int32 y_target_bpr = y*target_bpr;
		 		for (float x=0;x<source_bpr;x += last_calculated_resolution) {
					real_x = x-cx;
					real_y = y-cy;
					if (selection->ContainsPoint(x,y) == TRUE) {
						if ((real_x != 0) && (real_y != 0)) {
							// Let's try the inline assemler function for square root estimation
							#ifdef __POWERPC__
							one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x+real_y*real_y);
							sqrt_x_plus_y = 1.0/one_per_sqrt_x_plus_y;
							#elif __INTEL__
//							sqrt_x_plus_y = fsqrt(real_x*real_x+real_y*real_y);
							sqrt_x_plus_y = sqrt(real_x*real_x+real_y*real_y);
							one_per_sqrt_x_plus_y = 1.0 / sqrt_x_plus_y;
							#endif
							dx = one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720];
							dx = dx*(1-k*sqrt_x_plus_y/R);
							dy = dx*real_y;
							dx = dx*real_x;
							// This if is quite slow.
							if (((y+dy) <= end_y) && ((y+dy)>=0) && ((x+dx) >= 0) && ((x+dx)<target_bpr)) {
								*(target_bits + (int32)x + y_target_bpr) = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
							}
							else {
								*(target_bits +(int32)x + y_target_bpr) = background.word;
							}
						}
						else {
							if (real_x != 0) {
								// We also have to estimate the sqrt here.
								one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x);
								sqrt_x_plus_y = 1/one_per_sqrt_x_plus_y;
								dx =  one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720]*real_x;
								dx = dx*(1-k*sqrt_x_plus_y/R);
								if (((x+dx) >= 0) && ((x+dx)<target_bpr))
									*(target_bits +(int32)x +y_target_bpr) = *(source_bits + (int32)y*target_bpr + (int32)(x +dx));
								else
									*(target_bits +(int32)x + y_target_bpr)= background.word;
							}
							else if (real_y != 0) {
								one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_y*real_y);
								sqrt_x_plus_y = 1/one_per_sqrt_x_plus_y;
								dy =  one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720]*real_y;
								dy = dy*(1-k*sqrt_x_plus_y/R);
								if (((y+dy)<=end_y) && ((y+dy) >= 0))
									*(target_bits +(int32)x + y_target_bpr) = *(source_bits + (int32)x + (int32)(y + dy)*source_bpr);
								else
									*(target_bits +(int32)x +y_target_bpr) = background.word;
							}
							else {
								*(target_bits +(int32)x +y_target_bpr) = *(source_bits + (int32)x + (int32)y*source_bpr);
							}
						}
					}
					else {
						*(target_bits +(int32)x +y_target_bpr) = *(source_bits + (int32)x + (int32)y*source_bpr);
					}
				}
		 	}
		}
	}
	updated_region->Set(preview_bitmap->Bounds());
	return last_calculated_resolution;
}


void WaveManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	if (first_click == TRUE)
		previous_settings = settings;

	settings.center = point;
	if (config_view != NULL)
		config_view->ChangeSettings(&settings);
}




//int32 WaveManipulator::wave_func(uint32 *target_bits,uint32 *source_bits,int32 target_bpr,int32 source_bpr,int32 start_y,int32 end_y,int32 amount_of_wave,int32 length_of_wave,int32 dampening_of_wave,float center_x, float center_y,float top_of_image,float bottom_of_image)
//{
//	// First move the bits to right starting positions.
//	// In this manipulator do not move the source bits, because it will be
//	// 'indexed' directly.
////	source_bits += start_y * source_bpr;
//	target_bits += start_y * target_bpr;
//
//	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
//	progress_message.AddFloat("delta",0.0);
//
//	BStopWatch *watch = new BStopWatch("Wave Time");
//
//	float s = length_of_wave;
//	float A = amount_of_wave;
//	float k = (float)(dampening_of_wave)/(float)MAX_WAVE_DAMPENING;
//	float dx,dy;
//	register float sqrt_x_plus_y;
//	register float one_per_sqrt_x_plus_y;
//	float cx,cy;
//	float top,bottom;
//	top = top_of_image;
//	bottom = bottom_of_image;
//
//	cx = floor(center_x);
//	cy = floor(center_y);
//	float R = sqrt(pow(max_c(abs(cx),target_bpr-abs(cx)),2)+pow(max_c(abs(cy),bottom_of_image-abs(cy)),2));
//
//
//	float real_x,real_y;
//	register float two_pi_per_s = 2*PI/s;
//	register float two_360_per_s = 720.0/s;
//
//	if (preview_level == FULL_CALCULATION) {
//		uint32 p1,p2,p3,p4;
//	 	for (float y=start_y;y<=end_y;y++) {
//	 		for (float x=0;x<source_bpr;x++) {
//				real_x = x-cx;
//				real_y = y-cy;
//				uint32 target_value = 0x00000000;
//				if ((real_x != 0) && (real_y != 0)) {
//					sqrt_x_plus_y = sqrt(real_x*real_x+real_y*real_y);
//					dx = real_x/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);	// The sin and div are slow.
//					dx = dx*(1-k*sqrt_x_plus_y/R);
//					dy = dx/real_x*real_y;
//					float y_mix_upper = ceil(y+dy)-(y+dy);
//					float x_mix_right = (x+dx)- floor(x+dx);
//					if ((ceil(y+dy) <= bottom) && (ceil(y+dy)>=top)) {
//						if ((floor(x+dx) >= 0) && (floor(x+dx) <source_bpr)) {
//							p1 = *(source_bits + (int32)ceil(y+dy)*source_bpr + (int32)floor(x+dx));
//						}
//						else
//							p1 = 0xFFFFFF00;
//
//						if ((ceil(x+dx) >= 0) && (ceil(x+dx) <source_bpr)) {
//							p2 = *(source_bits + (int32)ceil(y+dy)*source_bpr + (int32)ceil(x+dx));
//						}
//						else
//							p2 = 0xFFFFFF00;
//					}
//					else {
//						p1 = 0xFFFFFF00;
//						p2 = 0xFFFFFF00;
//					}
//					if ((floor(y+dy) <= bottom) && (floor(y+dy)>=top)) {
//						if ((floor(x+dx) >= 0) && (floor(x+dx) <source_bpr)) {
//							p3 = *(source_bits + (int32)floor(y+dy)*source_bpr + (int32)floor(x+dx));
//						}
//						else
//							p3 = 0xFFFFFF00;
//
//						if ((ceil(x+dx) >= 0) && (ceil(x+dx) <source_bpr)) {
//							p4 = *(source_bits + (int32)floor(y+dy)*source_bpr + (int32)ceil(x+dx));
//						}
//						else
//							p4 = 0xFFFFFF00;
//					}
//					else {
//						p3 = 0xFFFFFF00;
//						p4 = 0xFFFFFF00;
//					}
//					*target_bits++ = combine_4_pixels(p1,p2,p3,p4,(1-y_mix_upper)*(1-x_mix_right),(1-y_mix_upper)*(x_mix_right),(y_mix_upper)*(1-x_mix_right),(y_mix_upper)*(x_mix_right));
//				}
//				else {
//					if (real_x != 0) {
//						sqrt_x_plus_y = sqrt(real_x*real_x);
//						dx =  real_x/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);
//						dx = dx*(1-k*sqrt_x_plus_y/R);
//						float x_mix_right = (x+dx)-floor(x+dx);
//						if ((ceil(x+dx) >= 0) && (ceil(x+dx)<target_bpr))
//							p1 = *(source_bits + (int32)y*target_bpr + (int32)ceil((x +dx)));
//						else
//							p1 = 0xFFFFFF00;
//
//						if ((floor(x+dx) >= 0) && (floor(x+dx)<target_bpr))
//							p2 = *(source_bits + (int32)y*target_bpr + (int32)floor((x +dx)));
//						else
//							p2 = 0xFFFFFF00;
//						*target_bits++ = mix_2_pixels_fixed(p1,p2,32768*x_mix_right);
//					}
//					else if (real_y != 0) {
//						sqrt_x_plus_y = sqrt(real_y*real_y);
//						dy = real_y/sqrt_x_plus_y*A*sin(sqrt_x_plus_y*two_pi_per_s);
//						dy = dy*(1-k*sqrt_x_plus_y/R);
//						float y_mix_upper = ceil(y+dy)-(y+dy);
//						if ((ceil(y+dy)<=bottom) && (ceil(y+dy) >= top))
//							p1 = *(source_bits + (int32)x + (int32)ceil((y + dy))*target_bpr);
//						else
//							p1 = 0xFFFFFF00;
//
//						if ((floor(y+dy)<=bottom) && (floor(y+dy) >= top))
//							p2 = *(source_bits + (int32)x + (int32)floor((y + dy))*target_bpr);
//						else
//							p2 = 0xFFFFFF00;
//							*target_bits++ = mix_2_pixels_fixed(p2,p1,32768*y_mix_upper);
//					}
//					else {
//						*target_bits++ = *(source_bits + (int32)x + (int32)y*source_bpr);
//					}
//				}
//
//			}
//	 		// Send a progress-message if required
//	 		if ((status_view != NULL) && ((int32)y%10 == 0)) {
//	 			progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
//				status_view->Window()->PostMessage(&progress_message,status_view);
//	 		}
//	 	}
//	}
//	else if (preview_level == GOOD_PREVIEW) {
//	 	for (float y=start_y;y<=end_y;y++) {
//	 		for (float x=0;x<source_bpr;x++) {
//				real_x = x-cx;
//				real_y = y-cy;
//				if ((real_x != 0) && (real_y != 0)) {
//					// Let's try the inline assemler function for square root estimation
//					one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x+real_y*real_y);
//					sqrt_x_plus_y = 1.0/one_per_sqrt_x_plus_y;
//
//					dx = one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720];
//					dx = dx*(1-k*sqrt_x_plus_y/R);
//					dy = dx*real_y;
//					dx = dx*real_x;
//					// This if is quite slow.
//					if (((y+dy) <= bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr)) {
//						*(target_bits) = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
//					}
//					else {
//						*target_bits = 0xFFFFFF00;
//					}
//				}
//				else {
//					if (real_x != 0) {
//						// We also have to estimate the sqrt here.
//						one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x);
//						sqrt_x_plus_y = 1/one_per_sqrt_x_plus_y;
//						dx =  one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720]*real_x;
//						dx = dx*(1-k*sqrt_x_plus_y/R);
//						if (((x+dx) >= 0) && ((x+dx)<target_bpr))
//							*target_bits = *(source_bits + (int32)y*target_bpr + (int32)(x +dx));
//						else
//							*target_bits = 0xFFFFFF00;
//					}
//					else if (real_y != 0) {
//						one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_y*real_y);
//						sqrt_x_plus_y = 1/one_per_sqrt_x_plus_y;
//						dy =  one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720]*real_y;
//						dy = dy*(1-k*sqrt_x_plus_y/R);
//						if (((y+dy)<=bottom) && ((y+dy) >= top))
//							*target_bits = *(source_bits + (int32)x + (int32)(y + dy)*target_bpr);
//						else
//							*target_bits = 0xFFFFFF00;
//					}
//					else {
//						*target_bits = *(source_bits + (int32)x + (int32)y*target_bpr);
//					}
//				}
//				target_bits++;
//			}
//	 		// Send a progress-message if required
//	 		if ((status_view != NULL) && ((int32)y%10 == 0)) {
//	 			progress_message.ReplaceFloat("delta",(100.0*progress_step)/(float)(end_y-start_y)*10.0);
//				status_view->Window()->PostMessage(&progress_message,status_view);
//	 		}
//	 	}
//	}
//	else {
//		int32 width = source_bpr/2*2;
//	 	for (float y=start_y;y<end_y;y+=2) {
//	 		for (float x=0;x<width;x+=2) {
//				real_x = x-cx;
//				real_y = y-cy;
//				if ((real_x != 0) && (real_y != 0)) {
//					one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x+real_y*real_y);
//					sqrt_x_plus_y = 1.0/one_per_sqrt_x_plus_y;
//
//					dx = one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720];
//					dx = dx*(1-k*sqrt_x_plus_y/R);
//					dy = dx*real_y;
//					dx = dx*real_x;
//					// This if is quite slow.
//					if (((y+dy) < bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr-1)) {
//						uint32 new_value = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
//						*(target_bits) = new_value;
//						*(target_bits+1) = new_value;
//						*(target_bits+target_bpr) = new_value;
//						*(target_bits+target_bpr+1) = new_value;
//
//					}
//					else {
//						*target_bits = 0xFFFFFF00;
//						*(target_bits+1) = 0xFFFFFF00;
//						*(target_bits+target_bpr) = 0xFFFFFF00;
//						*(target_bits+target_bpr+1) = 0xFFFFFF00;
//					}
//				}
//				else {
//					if (real_x != 0) {
//						dx =  A*sin(real_x*two_pi_per_s);
//						dx = dx*(1-k*real_x/R);
//						if (((x+dx) >= 0) && ((x+dx)<target_bpr)) {
//							uint32 new_value = *(source_bits + (int32)y*target_bpr + (int32)(x +dx));
//							*target_bits = new_value;
//							*(target_bits+1) = new_value;
//							*(target_bits+target_bpr) = new_value;
//							*(target_bits+target_bpr+1) = new_value;
//						}
//						else {
//							*target_bits = 0xFFFFFF00;
//							*(target_bits+1) = 0xFFFFFF00;
//							*(target_bits+target_bpr) = 0xFFFFFF00;
//							*(target_bits+target_bpr+1) = 0xFFFFFF00;
//						}
//					}
//					else if (real_y != 0) {
//						dy = A*sin(real_y*two_pi_per_s);
//						dy = dy*(1-k*real_y/R);
//						if (((y+dy)<=bottom) && ((y+dy) >= top)) {
//							int32 new_value = *(source_bits + (int32)x + (int32)(y + dy)*target_bpr);
//							*target_bits = new_value;
//							*(target_bits+1) = new_value;
//							*(target_bits+target_bpr) = new_value;
//							*(target_bits+target_bpr+1) = new_value;
//						}
//						else {
//							*target_bits = 0xFFFFFF00;
//							*(target_bits+1) = 0xFFFFFF00;
//							*(target_bits+target_bpr) = 0xFFFFFF00;
//							*(target_bits+target_bpr+1) = 0xFFFFFF00;
//						}
//					}
//					else {
//						int32 new_value = *(source_bits + (int32)x + (int32)y*target_bpr);
//						*target_bits = new_value;
//						*(target_bits+1) = new_value;
//						*(target_bits+target_bpr) = new_value;
//						*(target_bits+target_bpr+1) = new_value;
//					}
//				}
//				target_bits+=2;
//			}
//			if ((source_bpr % 2) == 0) {
//				target_bits += target_bpr;
//			}
//			else
//				target_bits += target_bpr + 1;
//	 	}
//		// Here we handle the last row separately if needed.
//		if ((end_y-start_y)%2 == 0) {
//			float y = end_y;
//	 		for (float x=0;x<width;x+=2) {
//				real_x = x-cx;
//				real_y = y-cy;
//				if ((real_x != 0) && (real_y != 0)) {
//					one_per_sqrt_x_plus_y = reciprocal_of_square_root(real_x*real_x+real_y*real_y);
//					sqrt_x_plus_y = 1.0/one_per_sqrt_x_plus_y;
//
//					dx = one_per_sqrt_x_plus_y*A*sin_table[(int32)(sqrt_x_plus_y*two_360_per_s)%720];
//					dx = dx*(1-k*sqrt_x_plus_y/R);
//					dy = dx*real_y;
//					dx = dx*real_x;
//					// This if is quite slow.
//					if (((y+dy) < bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr-1)) {
//						uint32 new_value = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
//						*(target_bits) = new_value;
//						*(target_bits+1) = new_value;
//					}
//					else {
//						*target_bits = 0xFFFFFF00;
//						*(target_bits + 1) = 0xFFFFFF00;
//					}
//				}
//				else {
//					if (real_x != 0) {
//						dx =  A*sin(real_x*two_pi_per_s);
//						dx = dx*(1-k*real_x/R);
//						if (((x+dx) >= 0) && ((x+dx)<target_bpr)) {
//							uint32 new_value = *(source_bits + (int32)y*target_bpr + (int32)(x +dx));
//							*target_bits = new_value;
//							*(target_bits+1) = new_value;
//						}
//						else {
//							*target_bits = 0xFFFFFF00;
//							*(target_bits+1) = 0xFFFFFF00;
//						}
//					}
//					else if (real_y != 0) {
//						dy = A*sin(real_y*two_pi_per_s);
//						dy = dy*(1-k*real_y/R);
//						if (((y+dy)<=bottom) && ((y+dy) >= top)) {
//							int32 new_value = *(source_bits + (int32)x + (int32)(y + dy)*target_bpr);
//							*target_bits = new_value;
//							*(target_bits+1) = new_value;
//						}
//						else {
//							*target_bits = 0xFFFFFF00;
//							*(target_bits+1) = 0xFFFFFF00;
//						}
//					}
//					else {
//						int32 new_value = *(source_bits + (int32)x + (int32)y*target_bpr);
//						*target_bits = new_value;
//						*(target_bits+1) = new_value;
//					}
//				}
//				target_bits+=2;
//			}
//
//		}
//	}
//	delete watch;
//	return B_OK;
//}


void WaveManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}
	if (preview_bitmap != NULL) {
		system_info info;
		get_system_info(&info);
		double speed = info.cpu_count * info.cpu_clock_speed;
		speed = speed / 15000;

		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width()+1) * (bounds.Height() + 1);
		lowest_available_quality = 1;
		while ((2*num_pixels/lowest_available_quality/lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		lowest_available_quality = min_c(lowest_available_quality,16);
		highest_available_quality = max_c(lowest_available_quality/2,1);
	}
	else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}


void WaveManipulator::Reset(Selection*)
{
	if (preview_bitmap != NULL) {
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();

		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i=0;i<bits_length;i++)
			*target_bits++  = *source_bits++;
	}
}

BView* WaveManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new WaveManipulatorView(BRect(0,0,0,0),this, target);
	config_view->ChangeSettings(&settings);
	return config_view;
}


ManipulatorSettings* WaveManipulator::ReturnSettings()
{
	return new WaveManipulatorSettings(settings);
}

void WaveManipulator::ChangeSettings(ManipulatorSettings *s)
{
	WaveManipulatorSettings *new_settings = dynamic_cast<WaveManipulatorSettings*>(s);
	if (new_settings != NULL) {
		previous_settings = settings;
		settings = *new_settings;
	}
}

//-------------


WaveManipulatorView::WaveManipulatorView(BRect rect,WaveManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	manipulator = manip;
	target = new BMessenger(t);
	preview_started = FALSE;

	wave_length_slider = new BSlider(BRect(0,0,150,0), "wave_length_slider",
		"Wave Length", new BMessage(WAVE_LENGTH_CHANGED), MIN_WAVE_LENGTH,
		MAX_WAVE_LENGTH, B_HORIZONTAL, B_TRIANGLE_THUMB);
	wave_length_slider->SetLimitLabels("Short","Long");
	wave_length_slider->SetModificationMessage(new BMessage(WAVE_LENGTH_ADJUSTING_STARTED));
	wave_length_slider->ResizeToPreferred();
	wave_length_slider->MoveTo(4,4);

	BRect frame = wave_length_slider->Frame();
	frame.OffsetBy(0,frame.Height()+4);

	wave_amount_slider = new BSlider(frame, "wave_amount_slider", "Wave Strength",
		new BMessage(WAVE_AMOUNT_CHANGED), MIN_WAVE_AMOUNT, MAX_WAVE_AMOUNT,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	wave_amount_slider->SetLimitLabels("Mild","Strong");
	wave_amount_slider->SetModificationMessage(new BMessage(WAVE_AMOUNT_ADJUSTING_STARTED));
	wave_amount_slider->ResizeToPreferred();

	AddChild(wave_length_slider);
	AddChild(wave_amount_slider);

	ResizeTo(wave_amount_slider->Frame().Width()+8,wave_amount_slider->Frame().bottom+4);
}


WaveManipulatorView::~WaveManipulatorView()
{
	delete target;
}


void WaveManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

	wave_length_slider->SetTarget(BMessenger(this));
	wave_amount_slider->SetTarget(BMessenger(this));
}


void WaveManipulatorView::AllAttached()
{
	wave_length_slider->SetValue(settings.wave_length);
	wave_amount_slider->SetValue(settings.wave_amount);

}


void WaveManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case WAVE_LENGTH_ADJUSTING_STARTED:
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			settings.wave_length = wave_length_slider->Value();
			manipulator->ChangeSettings(&settings);
			break;

		case WAVE_LENGTH_CHANGED:
			preview_started = FALSE;
			settings.wave_length = wave_length_slider->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case WAVE_AMOUNT_ADJUSTING_STARTED:
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			settings.wave_amount = wave_amount_slider->Value();
			manipulator->ChangeSettings(&settings);
			break;

		case WAVE_AMOUNT_CHANGED:
			preview_started = FALSE;
			settings.wave_amount = wave_amount_slider->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void WaveManipulatorView::ChangeSettings(WaveManipulatorSettings *s)
{
	settings = *s;
	BWindow *window = Window();

	if (window != NULL) {
		window->Lock();

		wave_length_slider->SetValue(settings.wave_length);
		wave_amount_slider->SetValue(settings.wave_amount);

		window->Unlock();
	}
}
