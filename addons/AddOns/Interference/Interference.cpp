/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Box.h>
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <StatusBar.h>
#include <StopWatch.h>
#include <Window.h>


#include "AddOns.h"
#include "Interference.h"
#include "ManipulatorInformer.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "UtilityClasses.h"

#include <math.h>


#define PI M_PI

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Interference"


#define WAVE_LENGTH_A_CHANGED			'WvAc'
#define WAVE_LENGTH_B_CHANGED			'WvBc'
#define WAVE_LENGTH_ADJUSTING_STARTED 	'WLAs'
#define CENTER_A_X_CHANGED				'CaXc'
#define CENTER_A_Y_CHANGED				'CaYc'
#define CENTER_B_X_CHANGED				'CbXc'
#define CENTER_B_Y_CHANGED				'CbYc'
#define GRAYSCALE_CHANGED				'gScd'


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Interference" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Creates an interference-pattern.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = DISTORT_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator*
instantiate_add_on(BBitmap* bm, ManipulatorInformer* i)
{
	// Here create a view-manipulator. The class should inherit
	// from the WindowGuiManipulator base-class. It will be deleted
	// in the application program.
	return new InterferenceManipulator(bm, i);
}


InterferenceManipulator::InterferenceManipulator(BBitmap* bm, ManipulatorInformer* i)
		: WindowGUIManipulator()
{
	copy_of_the_preview_bitmap = NULL;
	preview_bitmap = NULL;
	config_view = NULL;
	informer = i;

	SetPreviewBitmap(bm);

	sin_table = new float[720];
	for (int32 i=0;i<720;i++)
		sin_table[i] = sin((float)i/720.0*2*PI);
}


InterferenceManipulator::~InterferenceManipulator()
{
	delete[] sin_table;
	delete copy_of_the_preview_bitmap;
	delete informer;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
		config_view = NULL;
	}
}


BBitmap*
InterferenceManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	InterferenceManipulatorSettings *new_settings = dynamic_cast<InterferenceManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	MakeInterference(original,new_settings,selection);

	return original;
}


int32
InterferenceManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	if ((settings == previous_settings) == FALSE) {
		previous_settings = settings;
		MakeInterference(preview_bitmap,&previous_settings,selection);
		updated_region->Set(selection->GetBoundingRect());
		return 1;
	}

	return DRAW_ONLY_GUI;
}


void
InterferenceManipulator::MouseDown(BPoint point,uint32 buttons,BView*,bool first_click)
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


void
InterferenceManipulator::MakeInterference(BBitmap *target, InterferenceManipulatorSettings *set, Selection *sel)
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
	} c, fg, bg;

	c.bytes[3] = 255;

	float max_dist = 500;

	rgb_color fgColor;
	rgb_color bgColor;

	if (set->grayscale == B_CONTROL_ON) {
		fgColor.set_to(255, 255, 255, 255);
		bgColor.set_to(0, 0, 0, 255);
	} else {
		fgColor = informer->GetForegroundColor();
		bgColor = informer->GetBackgroundColor();
	}

	fg.word = RGBColorToBGRA(fgColor);
	bg.word = RGBColorToBGRA(bgColor);

	for (int32 y=b.top;y<=b.bottom;y++) {
		for (int32 x=b.left;x<=b.right;x++) {
			if (sel->ContainsPoint(x,y)) {
				float dist_A = sqrt(pow(fabs(x-c_A.x),2) + pow(fabs(y-c_A.y),2));
				float dist_B = sqrt(pow(fabs(x-c_B.x),2) + pow(fabs(y-c_B.y),2));

				float contrib_A = sin(((dist_A*wl_A)-floor(dist_A*wl_A))*2*PI) * max_c(0,(max_dist-dist_A))/max_dist;
				float contrib_B = sin(((dist_B*wl_B)-floor(dist_B*wl_B))*2*PI) * max_c(0,(max_dist-dist_B))/max_dist;
//				float contrib_A = sin_table[(int32)(((dist_A*wl_A)-floor(dist_A*wl_A))*720)];
//				float contrib_B = sin_table[(int32)(((dist_B*wl_B)-floor(dist_B*wl_B))*720)];

				float contrib = (contrib_A + contrib_B + 2) / 4;
				float contrib_opposite = 1.0 - contrib;

				c.bytes[0] = bg.bytes[0] * contrib_opposite + fg.bytes[0] * contrib;
				c.bytes[1] = bg.bytes[1] * contrib_opposite + fg.bytes[1] * contrib;
				c.bytes[2] = bg.bytes[2] * contrib_opposite + fg.bytes[2] * contrib;
				c.bytes[3] = bg.bytes[3] * contrib_opposite + fg.bytes[3] * contrib;


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


void
InterferenceManipulator::SetPreviewBitmap(BBitmap *bm)
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
		BRect bounds = preview_bitmap->Bounds();

		if ((settings.centerA == BPoint(0,0)) && (settings.centerB == BPoint(0,0))) {
			settings.centerA.x = bounds.Width()/2;
			settings.centerB.x = bounds.Width()/2;
			settings.centerA.y = bounds.Height()/2-50;
			settings.centerB.y = bounds.Height()/2+50;
		}
	}
}


const char*
InterferenceManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Left-click to move Center A, right-click to move Center B.");
}


const char*
InterferenceManipulator::ReturnName()
{
	return B_TRANSLATE("Interference");
}


void
InterferenceManipulator::Reset(Selection*)
{
	if (preview_bitmap != NULL) {
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();

		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i=0;i<bits_length;i++)
			*target_bits++  = *source_bits++;
	}
}


BView*
InterferenceManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new InterferenceManipulatorView(BRect(0,0,0,0),this,target);
	config_view->ChangeSettings(&settings);
	return config_view;
}


ManipulatorSettings*
InterferenceManipulator::ReturnSettings()
{
	return new InterferenceManipulatorSettings(settings);
}


void
InterferenceManipulator::ChangeSettings(ManipulatorSettings *s)
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
	manipulator = manip;
	target = new BMessenger(t);
	preview_started = FALSE;

	centerAX = new BSpinner("centerAx", "X:", new BMessage(CENTER_A_X_CHANGED));
	centerAX->SetMaxValue(9999);
	centerAX->SetMinValue(-9999);
	centerAY = new BSpinner("centerAy", "Y:", new BMessage(CENTER_A_Y_CHANGED));
	centerAY->SetMaxValue(9999);
	centerAY->SetMinValue(-9999);
	centerBX = new BSpinner("centerBx", "X:", new BMessage(CENTER_B_X_CHANGED));
	centerBX->SetMaxValue(9999);
	centerBX->SetMinValue(-9999);
	centerBY = new BSpinner("centerBy", "Y:", new BMessage(CENTER_B_Y_CHANGED));
	centerBY->SetMaxValue(9999);
	centerBY->SetMinValue(-9999);

	waveLengthSliderA = new BSlider("waveLengthSliderA",
		B_TRANSLATE("Wavelength:"),
		new BMessage(WAVE_LENGTH_A_CHANGED),
		(int32)MIN_WAVE_LENGTH, (int32)MAX_WAVE_LENGTH,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	waveLengthSliderA->SetLimitLabels(B_TRANSLATE("Short"),
		B_TRANSLATE("Long"));
	waveLengthSliderA->SetModificationMessage(
		new BMessage(WAVE_LENGTH_ADJUSTING_STARTED));

	waveLengthSliderB = new BSlider("waveLengthSliderB",
		B_TRANSLATE("Wavelength:"),
		new BMessage(WAVE_LENGTH_B_CHANGED),
		(int32)MIN_WAVE_LENGTH, (int32)MAX_WAVE_LENGTH,
		B_HORIZONTAL, B_TRIANGLE_THUMB);
	waveLengthSliderB->SetLimitLabels(B_TRANSLATE("Short"),
		B_TRANSLATE("Long"));
	waveLengthSliderB->SetModificationMessage(
		new BMessage(WAVE_LENGTH_ADJUSTING_STARTED));

	BBox *frameA = new BBox(B_FANCY_BORDER, BGroupLayoutBuilder(B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add(centerAX)
			.Add(centerAY)
			.End()
		.Add(waveLengthSliderA)
		.SetInsets(5.0, 5.0, 5.0, 5.0)
		.TopView());

	frameA->SetLabel(B_TRANSLATE("Center A"));

	BBox *frameB = new BBox(B_FANCY_BORDER, BGroupLayoutBuilder(B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add(centerBX)
			.Add(centerBY)
			.End()
		.Add(waveLengthSliderB)
		.SetInsets(5.0, 5.0, 5.0, 5.0)
		.TopView());

	frameB->SetLabel(B_TRANSLATE("Center B"));

	grayScale = new BCheckBox(B_TRANSLATE("Grayscale"), new BMessage(GRAYSCALE_CHANGED));

	SetLayout(BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(frameA)
		.Add(frameB)
		.Add(grayScale)
	);

}


void
InterferenceManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

	waveLengthSliderA->SetTarget(BMessenger(this));
	waveLengthSliderB->SetTarget(BMessenger(this));
	centerAX->SetTarget(BMessenger(this));
	centerAY->SetTarget(BMessenger(this));
	centerBX->SetTarget(BMessenger(this));
	centerBY->SetTarget(BMessenger(this));
	grayScale->SetTarget(BMessenger(this));
}


void
InterferenceManipulatorView::AllAttached()
{
	waveLengthSliderA->SetValue(settings.waveLengthA);
	waveLengthSliderB->SetValue(settings.waveLengthB);

	centerAX->SetValue(settings.centerA.x);
	centerAY->SetValue(settings.centerA.y);
	centerBX->SetValue(settings.centerB.x);
	centerBY->SetValue(settings.centerB.y);
	grayScale->SetValue(settings.grayscale);
}


void
InterferenceManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case WAVE_LENGTH_ADJUSTING_STARTED:
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			settings.waveLengthA = waveLengthSliderA->Value();
			settings.waveLengthB = waveLengthSliderB->Value();
			manipulator->ChangeSettings(&settings);
			break;

		case WAVE_LENGTH_A_CHANGED:
			preview_started = FALSE;
			settings.waveLengthA = waveLengthSliderA->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case WAVE_LENGTH_B_CHANGED:
			preview_started = FALSE;
			settings.waveLengthB = waveLengthSliderB->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case CENTER_A_X_CHANGED:
			preview_started = FALSE;
			settings.centerA.x = centerAX->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case CENTER_A_Y_CHANGED:
			preview_started = FALSE;
			settings.centerA.y = centerAY->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case CENTER_B_X_CHANGED:
			preview_started = FALSE;
			settings.centerB.x = centerBX->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case CENTER_B_Y_CHANGED:
			preview_started = FALSE;
			settings.centerB.y = centerBY->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case GRAYSCALE_CHANGED:
			preview_started = FALSE;
			settings.grayscale = grayScale->Value();
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void
InterferenceManipulatorView::ChangeSettings(InterferenceManipulatorSettings *s)
{
	settings = *s;
	BWindow *window = Window();

	if (window != NULL) {
		window->Lock();

		waveLengthSliderA->SetValue(settings.waveLengthA);
		waveLengthSliderB->SetValue(settings.waveLengthB);
		centerAX->SetValue(settings.centerA.x);
		centerAY->SetValue(settings.centerA.y);
		centerBX->SetValue(settings.centerB.x);
		centerBY->SetValue(settings.centerB.y);
		grayScale->SetValue(settings.grayscale);

		window->Unlock();
	}
}
