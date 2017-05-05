/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <math.h>
#include <LayoutBuilder.h>
#include <Slider.h>
#include <StatusBar.h>
#include <Window.h>

#define PI M_PI

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Twirl.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "SysInfoBeOS.h"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Twirl…";
	char menu_help_string[255] = "Twirls the active layer.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = DISTORT_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new TwirlManipulator(bm);
}


TwirlManipulator::TwirlManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	last_calculated_resolution = 8;
	lowest_available_quality = 8;
	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;
	config_view = NULL;

	sin_table = new float[720];
	cos_table = new float[720];
	for (int32 i=0;i<720;i++) {
		sin_table[i] = sin((float)i/720.0*2*PI);
		cos_table[i] = cos((float)i/720.0*2*PI);
	}

	SetPreviewBitmap(bm);
}


TwirlManipulator::~TwirlManipulator()
{
	delete[] sin_table;
	delete[] cos_table;

	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}
}


void TwirlManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	if (first_click)
		previous_settings = settings;

	settings.center = point;
	if (config_view != NULL)
		config_view->ChangeSettings(&settings);
}


BBitmap* TwirlManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	TwirlManipulatorSettings *new_settings = dynamic_cast<TwirlManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	BBitmap *source_bitmap;
	BBitmap *target_bitmap;
	BBitmap *new_bitmap;

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


	float start_y = 0;
	float end_y = target_bitmap->Bounds().bottom;
	float real_x;
	float real_y;
	float distance;
	float center_distance_from_edges = new_settings->twirl_radius;
	float cx = new_settings->center.x;
	float cy = new_settings->center.y;
	float k = new_settings->twirl_amount/(float)MAX_TWIRL_AMOUNT;
	float dx;
	float dy;
	int32 top = 0;
	int32 bottom = target_bitmap->Bounds().bottom;

	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	union {
		uint8 bytes[4];
		uint32 word;
	} background;

	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;


	if (selection->IsEmpty()) {
		uint32 p1,p2,p3,p4;
	 	for (float y=start_y;y<=end_y;y++) {
	 		for (float x=0;x<source_bpr;x++) {
				real_x = x-cx;
				real_y = y-cy;
				uint32 target_value = 0x00000000;
				distance = sqrt(real_x*real_x+real_y*real_y);

				if (distance <= center_distance_from_edges) {
					float omega = (center_distance_from_edges-distance)/center_distance_from_edges*k*2*PI;
					dx = (cos(omega)*real_x-sin(omega)*real_y)-real_x;
					dy = (sin(omega)*real_x+cos(omega)*real_y)-real_y;
				}
				else {
					dx = 0;
					dy = 0;
				}
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
	 		for (float x=0;x<source_bpr;x++) {
				if (selection->ContainsPoint(x,y)) {
					real_x = x-cx;
					real_y = y-cy;
					uint32 target_value = 0x00000000;
					distance = sqrt(real_x*real_x+real_y*real_y);

					if (distance <= center_distance_from_edges) {
						float omega = (center_distance_from_edges-distance)/center_distance_from_edges*k*2*PI;
						dx = (cos(omega)*real_x-sin(omega)*real_y)-real_x;
						dy = (sin(omega)*real_x+cos(omega)*real_y)-real_y;
					}
					else {
						dx = 0;
						dy = 0;
					}
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
				else
					*target_bits++ = *(source_bits + (int32)x + (int32)y*source_bpr);
			}
	 		// Send a progress-message if required
	 		if ((status_bar != NULL) && ((int32)y%10 == 0)) {
	 			progress_message.ReplaceFloat("delta",100.0/(float)(end_y-start_y)*10.0);
				status_bar->Window()->PostMessage(&progress_message,status_bar);
	 		}
	 	}
	}

	return original;
}


int32 TwirlManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
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

	int32 start_y = 0;
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
		int32 real_x;
		int32 real_y;
		int32 distance;
		int32 center_distance_from_edges = settings.twirl_radius;
		int32 cx = settings.center.x;
		int32 cy = settings.center.y;
		float k = settings.twirl_amount/(float)MAX_TWIRL_AMOUNT;
		int32 dx;
		int32 dy;
		float two_180_per_pi = 360.0/PI;
		int32 top = 0;
		int32 bottom = preview_bitmap->Bounds().bottom;
		float multiplier = 1.0 / (float)center_distance_from_edges*k*2.0*PI;

		if (selection->IsEmpty() == TRUE) {
		 	for (int32 y=start_y;y<=end_y;y += last_calculated_resolution) {
		 		for (int32 x=0;x<source_bpr;x += last_calculated_resolution) {
					real_x = x-cx;
					real_y = y-cy;
					#ifdef __POWERPC__
					distance = 1.0/reciprocal_of_square_root(real_x*real_x + real_y*real_y);
					#elif __INTEL__
//					distance = fsqrt(real_x*real_x + real_y*real_y);
					distance = sqrt(real_x*real_x + real_y*real_y);
					#endif
					if (distance <= center_distance_from_edges) {
						float omega = (center_distance_from_edges-distance)*multiplier;
						omega = (omega<0?2*PI+omega:omega);
						int32 cos_angle = (int32)(omega*two_180_per_pi)%720;
						int32 sin_angle = (int32)(omega*two_180_per_pi)%720;
						dx = (cos_table[cos_angle]*real_x-sin_table[sin_angle]*real_y)-real_x;
						dy = (sin_table[sin_angle]*real_x+cos_table[cos_angle]*real_y)-real_y;

					}
					else {
						dx = 0;
						dy = 0;
					}

					// This if is quite slow.
					if (((y+dy) <= bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr)) {
						*(target_bits + (int32)x + (int32)y*target_bpr) = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
					}
					else {
						*(target_bits + (int32)x + (int32)y*target_bpr) = background.word;
					}
				}
		 	}
		}
		else {
		 	for (int32 y=start_y;y<=end_y;y += last_calculated_resolution) {
		 		for (int32 x=0;x<source_bpr;x += last_calculated_resolution) {
					real_x = x-cx;
					real_y = y-cy;
					#ifdef __POWERPC__
					distance = 1.0/reciprocal_of_square_root(real_x*real_x + real_y*real_y);
					#elif __INTEL__
//					distance = fsqrt(real_x*real_x + real_y*real_y);
					distance = sqrt(real_x*real_x + real_y*real_y);
					#endif
					if ((distance <= center_distance_from_edges) && (selection->ContainsPoint(x,y))) {
						float omega = (center_distance_from_edges-distance)*multiplier;
						omega = (omega<0?2*PI+omega:omega);
						int32 cos_angle = (int32)(omega*two_180_per_pi)%720;
						int32 sin_angle = (int32)(omega*two_180_per_pi)%720;
						dx = (cos_table[cos_angle]*real_x-sin_table[sin_angle]*real_y)-real_x;
						dy = (sin_table[sin_angle]*real_x+cos_table[cos_angle]*real_y)-real_y;

					}
					else {
						dx = 0;
						dy = 0;
					}

					// This if is quite slow.
					if (((y+dy) <= bottom) && ((y+dy)>=top) && ((x+dx) >= 0) && ((x+dx)<target_bpr)) {
						*(target_bits + (int32)x + (int32)y*target_bpr) = *(source_bits + (int32)(x+dx) + (int32)(y+dy)*source_bpr);
					}
					else {
						*(target_bits + (int32)x + (int32)y*target_bpr) = background.word;
					}
				}
		 	}
		}
	}
	updated_region->Set(preview_bitmap->Bounds());
	return last_calculated_resolution;
}

void TwirlManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			if (preview_bitmap == NULL) {
				settings.center.x = bm->Bounds().right/2;
				settings.center.y = bm->Bounds().bottom/2;
			}
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
		if (config_view != NULL) {
			config_view->ChangeSettings(&settings);
		}
	}

	if (preview_bitmap != NULL) {
		BeOS_system_info info;
		get_BeOS_system_info(&info);
		double speed = info.cpu_count * info.cpu_clock_speed;
		speed = speed / 10000;

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


void TwirlManipulator::Reset(Selection*)
{
	if (preview_bitmap != NULL) {
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();

		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i=0;i<bits_length;i++)
			*target_bits++  = *source_bits++;
	}
}


BView* TwirlManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new TwirlManipulatorView(BRect(0,0,0,0),this,target);
	config_view->ChangeSettings(&settings);

	return config_view;
}

ManipulatorSettings* TwirlManipulator::ReturnSettings()
{
	return new TwirlManipulatorSettings(settings);
}

void TwirlManipulator::ChangeSettings(ManipulatorSettings *s)
{
	TwirlManipulatorSettings *new_settings = dynamic_cast<TwirlManipulatorSettings*>(s);

	if (new_settings != NULL) {
		previous_settings = settings;
		settings = *new_settings;
	}
}


TwirlManipulatorView::TwirlManipulatorView(BRect rect,TwirlManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	manipulator = manip;
	target = new BMessenger(t);
	preview_started = FALSE;

	twirl_radius_slider = new BSlider("twirl_radius_slider",
		"Twirl size:", new BMessage(TWIRL_RADIUS_CHANGED), 10, 1000,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	twirl_radius_slider->SetLimitLabels("Small","Big");
	twirl_radius_slider->SetModificationMessage(new BMessage(TWIRL_RADIUS_ADJUSTING_STARTED));
	twirl_radius_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	twirl_radius_slider->SetHashMarkCount(11);

	twirl_amount_slider = new BSlider("twirl_amount_slider",
		"Twirl direction:", new BMessage(TWIRL_AMOUNT_CHANGED), MIN_TWIRL_AMOUNT,
		MAX_TWIRL_AMOUNT, B_HORIZONTAL, B_TRIANGLE_THUMB);
	twirl_amount_slider->SetLimitLabels("Left","Right");
	twirl_amount_slider->SetModificationMessage(new BMessage(TWIRL_AMOUNT_ADJUSTING_STARTED));
	twirl_amount_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	twirl_amount_slider->SetHashMarkCount(11);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_ITEM_SPACING)
		.Add(twirl_radius_slider)
		.Add(twirl_amount_slider)
		.SetInsets(B_USE_SMALL_INSETS)
		.End();
}


TwirlManipulatorView::~TwirlManipulatorView()
{
	delete target;
}

void TwirlManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

	twirl_radius_slider->SetTarget(BMessenger(this));
	twirl_amount_slider->SetTarget(BMessenger(this));
}


void TwirlManipulatorView::AllAttached()
{
	twirl_radius_slider->SetValue(settings.twirl_radius);
	twirl_amount_slider->SetValue(settings.twirl_amount);
}


void TwirlManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case TWIRL_RADIUS_ADJUSTING_STARTED:
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			settings.twirl_radius = twirl_radius_slider->Value();
			manipulator->ChangeSettings(&settings);
			break;

		case TWIRL_RADIUS_CHANGED:
			preview_started = FALSE;
			settings.twirl_radius = twirl_radius_slider->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case TWIRL_AMOUNT_ADJUSTING_STARTED:
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			settings.twirl_amount = twirl_amount_slider->Value();
			manipulator->ChangeSettings(&settings);
			break;

		case TWIRL_AMOUNT_CHANGED:
			preview_started = FALSE;
			settings.twirl_amount = twirl_amount_slider->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void TwirlManipulatorView::ChangeSettings(TwirlManipulatorSettings *s)
{
	settings = *s;
	BWindow *window = Window();

	if (window != NULL) {
		window->Lock();

		twirl_radius_slider->SetValue(settings.twirl_radius);
		twirl_amount_slider->SetValue(settings.twirl_amount);

		window->Unlock();
	}
}
