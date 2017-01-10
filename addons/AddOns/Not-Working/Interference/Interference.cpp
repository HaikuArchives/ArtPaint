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
#include <StatusBar.h>
#include <StopWatch.h>
#include <Window.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Interference.h"
#include "PixelOperations.h"
#include "Selection.h"

#define PI M_PI


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "Interference…";
	char menu_help_string[255] = "Makes an interference-pattern on the active layer.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = DISTORT_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	// Here create a view-manipulator. The class should inherit
	// from the WindowGuiManipulator base-class. It will be deleted
	// in the application program.
	delete i;
	return new InterferenceManipulator(bm);
}



InterferenceManipulator::InterferenceManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	livePreview = false;
	copy_of_the_preview_bitmap = NULL;
	preview_bitmap = NULL;

	SetPreviewBitmap(bm);

	sin_table = new float[720];
	for (int32 i=0;i<720;i++)
		sin_table[i] = sin((float)i/720.0*2*PI);
}


InterferenceManipulator::~InterferenceManipulator()
{
	delete[] sin_table;
	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
		config_view = NULL;
	}
}


BBitmap* InterferenceManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	InterferenceManipulatorSettings *new_settings = dynamic_cast<InterferenceManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	MakeInterference(original,new_settings,selection);

	return original;
}

int32 InterferenceManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	previous_settings = settings;
	if (full_quality || livePreview) {
		MakeInterference(preview_bitmap,&previous_settings,selection);
		updated_region->Set(selection->GetBoundingRect());
		return 1;
	}

	return DRAW_ONLY_GUI;
}


void InterferenceManipulator::MouseDown(BPoint point,uint32 buttons,BView*,bool first_click)
{
	if (first_click == TRUE)
		previous_settings = settings;

	if (buttons & B_PRIMARY_MOUSE_BUTTON)
		settings.centerA = point;
	else
		settings.centerB = point;


	if (config_view != NULL)
		config_view->ChangeSettings(&settings);
}



void InterferenceManipulator::MakeInterference(BBitmap *target, InterferenceManipulatorSettings *set, Selection *sel)
{
	BStopWatch watch("Making an interference");
	uint32 *target_bits = (uint32*)target->Bits();
	uint32 target_bpr = target->BytesPerRow()/4;

	BRect b = sel->GetBoundingRect();

	// the wave lengths are taken as inverse numbers
	float wl_A = 1.0/set->waveLengthA;
	float wl_B = 1.0/set->waveLengthB;

	// the centers
	BPoint c_A = set->centerA;
	BPoint c_B = set->centerB;

	union {
		uint8 bytes[4];
		uint32 word;
	} c;
	c.bytes[3] = 255;

	float max_dist = 500;

	for (int32 y=b.top;y<=b.bottom;y++) {
		for (int32 x=b.left;x<=b.right;x++) {
			if (sel->ContainsPoint(x,y)) {
				float dist_A = sqrt(pow(fabs(x-c_A.x),2) + pow(fabs(y-c_A.y),2));
				float dist_B = sqrt(pow(fabs(x-c_B.x),2) + pow(fabs(y-c_B.y),2));

				float contrib_A = sin(((dist_A*wl_A)-floor(dist_A*wl_A))*2*PI) * max_c(0,(max_dist-dist_A))/max_dist;
				float contrib_B = sin(((dist_B*wl_B)-floor(dist_B*wl_B))*2*PI) * max_c(0,(max_dist-dist_B))/max_dist;
//				float contrib_A = sin_table[(int32)(((dist_A*wl_A)-floor(dist_A*wl_A))*720)];
//				float contrib_B = sin_table[(int32)(((dist_B*wl_B)-floor(dist_B*wl_B))*720)];

				float contrib = contrib_A + contrib_B;
				uint8 value = 127 + contrib*.5*127;
				c.bytes[0] = c.bytes[1] = c.bytes[2] = value;
//				if (contrib_A*contrib_B > 0) {
//					// partially constructive
//					c.bytes[0] = c.bytes[1] = c.bytes[2] = 255;
//				}
//				else {
//					// destructive
//					c.bytes[0] = c.bytes[1] = c.bytes[2] = 0;
//				}

				*(target_bits + x + y*target_bpr) = c.word;
			}
		}
	}
}


void InterferenceManipulator::SetPreviewBitmap(BBitmap *bm)
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
		double speed = info.cpu_count * 2000; // TODO: used to be info.cpu_clock_speed but was removed
		speed = speed / 15000;

		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width()+1) * (bounds.Height() + 1);

		if (num_pixels < 200000)
			livePreview = true;

		if ((settings.centerA == BPoint(0,0)) && (settings.centerB == BPoint(0,0))) {
			settings.centerA.x = bounds.Width()/2;
			settings.centerB.x = bounds.Width()/2;
			settings.centerA.y = bounds.Height()/2-50;
			settings.centerB.y = bounds.Height()/2+50;
		}

	}
}


void InterferenceManipulator::Reset(Selection*)
{
	if (preview_bitmap != NULL) {
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();

		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i=0;i<bits_length;i++)
			*target_bits++  = *source_bits++;
	}
}

BView* InterferenceManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new InterferenceManipulatorView(BRect(0,0,0,0),this,target);
	config_view->ChangeSettings(&settings);
	return config_view;
}


ManipulatorSettings* InterferenceManipulator::ReturnSettings()
{
	return new InterferenceManipulatorSettings(settings);
}

void InterferenceManipulator::ChangeSettings(ManipulatorSettings *s)
{
	InterferenceManipulatorSettings *new_settings = dynamic_cast<InterferenceManipulatorSettings*>(s);
	if (new_settings != NULL) {
		previous_settings = settings;
		settings = *new_settings;
	}
}

//-------------


InterferenceManipulatorView::InterferenceManipulatorView(BRect rect,
	InterferenceManipulator *manip, const BMessenger& t)
	: WindowGUIManipulatorView()
	, target(NULL)
{
//	manipulator = manip;
//	target = new BMessenger(t);
//	preview_started = FALSE;
//
//	wave_length_slider = new ControlSlider(BRect(0,0,150,0),"wave_length_slider","Wave Length",new BMessage(WAVE_LENGTH_CHANGED),MIN_WAVE_LENGTH,MAX_WAVE_LENGTH,B_TRIANGLE_THUMB);
//	wave_length_slider->SetLimitLabels("Short","Long");
//	wave_length_slider->SetModificationMessage(new BMessage(WAVE_LENGTH_ADJUSTING_STARTED));
//	wave_length_slider->ResizeToPreferred();
//	wave_length_slider->MoveTo(4,4);
//
//	BRect frame = wave_length_slider->Frame();
//	frame.OffsetBy(0,frame.Height()+4);
//
//	wave_amount_slider = new ControlSlider(frame,"wave_amount_slider","Wave Strength",new BMessage(WAVE_AMOUNT_CHANGED),MIN_WAVE_AMOUNT,MAX_WAVE_AMOUNT,B_TRIANGLE_THUMB);
//	wave_amount_slider->SetLimitLabels("Mild","Strong");
//	wave_amount_slider->SetModificationMessage(new BMessage(WAVE_AMOUNT_ADJUSTING_STARTED));
//	wave_amount_slider->ResizeToPreferred();
//
//	AddChild(wave_length_slider);
//	AddChild(wave_amount_slider);
//
//	ResizeTo(wave_amount_slider->Frame().Width()+8,wave_amount_slider->Frame().bottom+4);
}



void InterferenceManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

//	wave_length_slider->SetTarget(BMessenger(this));
//	wave_amount_slider->SetTarget(BMessenger(this));
}


void InterferenceManipulatorView::AllAttached()
{
//	wave_length_slider->SetValue(settings.wave_length);
//	wave_amount_slider->SetValue(settings.wave_amount);

}


void InterferenceManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
//		case WAVE_LENGTH_ADJUSTING_STARTED:
//			if (preview_started == FALSE) {
//				preview_started = TRUE;
//				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
//			}
//			settings.wave_length = wave_length_slider->Value();
//			manipulator->ChangeSettings(&settings);
//			break;
//
//		case WAVE_LENGTH_CHANGED:
//			preview_started = FALSE;
//			settings.wave_length = wave_length_slider->Value();
//			manipulator->ChangeSettings(&settings);
//			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
//			break;
//
//		case WAVE_AMOUNT_ADJUSTING_STARTED:
//			if (preview_started == FALSE) {
//				preview_started = TRUE;
//				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
//			}
//			settings.wave_amount = wave_amount_slider->Value();
//			manipulator->ChangeSettings(&settings);
//			break;
//
//		case WAVE_AMOUNT_CHANGED:
//			preview_started = FALSE;
//			settings.wave_amount = wave_amount_slider->Value();
//			manipulator->ChangeSettings(&settings);
//			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
//			break;
//
		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void InterferenceManipulatorView::ChangeSettings(InterferenceManipulatorSettings *s)
{
//	settings = *s;
//	BWindow *window = Window();
//
//	if (window != NULL) {
//		window->Lock();
//
//		wave_length_slider->SetValue(settings.wave_length);
//		wave_amount_slider->SetValue(settings.wave_amount);
//
//		window->Unlock();
//	}
}
