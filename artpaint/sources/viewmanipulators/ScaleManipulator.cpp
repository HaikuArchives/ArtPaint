/*

	Filename:	ScaleManipulator.cpp
	Contents:	ScaleManipulator-class definition.
	Author:		Heikki Suhonen

*/

#include <CheckBox.h>
#include <ClassInfo.h>
#include <Button.h>
#include <new.h>
#include <StatusBar.h>
#include <stdio.h>
#include <string.h>
#include <Window.h>

#include "ScaleManipulator.h"
#include "PixelOperations.h"
#include "MessageConstants.h"
#include "StringServer.h"

ScaleManipulator::ScaleManipulator(BBitmap *bm)
	:	WindowGUIManipulator()
{
	configuration_view = NULL;
	settings = new ScaleManipulatorSettings();

	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bm);
}



ScaleManipulator::~ScaleManipulator()
{
	if (configuration_view != NULL) {
		configuration_view->RemoveSelf();
		delete configuration_view;
	}

	delete copy_of_the_preview_bitmap;
}



BBitmap* ScaleManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection*,BStatusBar *status_bar)
{
	ScaleManipulatorSettings *new_settings = cast_as(set,ScaleManipulatorSettings);
	if (new_settings == NULL)
		return NULL;


	float starting_width = original->Bounds().Width()+1;
	float starting_height = original->Bounds().Height()+1;

	float new_width = round(starting_width * new_settings->width_coefficient);
	float new_height = round(starting_height * new_settings->height_coefficient);

	// Create a new bitmap here and copy it applying scaling.
	// But first create an intermediate bitmap for scaling in one direction only.
	// Remember that the returned bitmap must accept views
	// First scale the width.
	// If the new size is the same as old return the original
	if ((new_width == starting_width) && (new_height == starting_height))
		return NULL;


	BMessage progress_message = BMessage(B_UPDATE_STATUS_BAR);
	progress_message.AddFloat("delta",0.0);

	BBitmap *intermediate_bitmap;
	if (new_width != starting_width) {
		intermediate_bitmap = new BBitmap(BRect(0,0,new_width-1,starting_height-1),B_RGB_32_BIT);
		if (intermediate_bitmap->IsValid() == FALSE)
			throw bad_alloc();
		uint32 *target_bits = (uint32*)intermediate_bitmap->Bits();
		int32 target_bpr = intermediate_bitmap->BytesPerRow()/4;
		uint32 *source_bits = (uint32*)original->Bits();
		int32 source_bpr = original->BytesPerRow()/4;
		int32 bottom = (int32)original->Bounds().bottom;
		float diff = (starting_width)/(new_width);
		float accumulation = 0;


		if (diff<1) {
			// Enlarge in x direction.
			for (int32 y=0;y<=bottom;y++) {
				accumulation = 0;
				for (int32 x=0;x<target_bpr;x++) {
					// This does not calculate correct values because mix_2_pixels does not
					// take rounding errors into account.
	//				*target_bits++ = mix_2_pixels(*(source_bits + (int32)floor(accumulation)),*(source_bits + (int32)ceil(accumulation)),ceil(accumulation)-accumulation);
					*target_bits++ = mix_2_pixels_fixed(*(source_bits + (int32)floor(accumulation)),*(source_bits + (int32)ceil(accumulation)),(uint32)(32768*(ceil(accumulation)-accumulation)));

					accumulation += diff;
				}
				source_bits += source_bpr;
				if ((y%10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}
		else if (diff>1) {
			// Make smaller in x direction.
			diff = (starting_width-1)/new_width;	// Why this line???
			for (int32 y=0;y<=bottom;y++) {
				accumulation = 0;
				for (int32 x=0;x<target_bpr;x++) {
					// Here we average the original pixels between accumulation and accumulation+diff.
					// The pixels at end get a little lower coefficients than the other pixels.
					// But for now we just settle for averaging the pixels between floor(accumulation)
					// and floor(accumulation+diff):
					float coeff = 1.0;
					float coeff_diff = 1.0/(floor(accumulation+diff)-floor(accumulation));
					uint32 target_value = 0x00000000;
					for (int32 i=(int32)floor(accumulation);i<floor(accumulation+diff);i++) {
						// This does not calculate correct values because mix_2_pixels does not
						// take rounding errors into account.
	//					target_value = mix_2_pixels(*(source_bits + i),target_value,coeff);
						target_value = mix_2_pixels_fixed(*(source_bits + i),target_value,(uint32)(32768*coeff));
						coeff -= coeff_diff;
					}
					*target_bits++ = target_value;
					accumulation += diff;
				}
				source_bits += source_bpr;
				if ((y%10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}
		else {
			for (int32 y=0;y<=bottom;y++) {
				for (int32 x=0;x<target_bpr;x++) {
					// Just copy it straight
					*target_bits++ = *source_bits++;
				}
				if ((y%10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}
	}
	else {
		intermediate_bitmap = original;
	}

	if (new_height != starting_height) {
		BBitmap *new_bitmap = new BBitmap(BRect(0,0,new_width-1,new_height-1),B_RGB_32_BIT);
		if (new_bitmap->IsValid() == FALSE)
			throw bad_alloc();

		uint32 *target_bits = (uint32*)new_bitmap->Bits();
		int32 target_bpr = new_bitmap->BytesPerRow()/4;
		uint32 *source_bits = (uint32*)intermediate_bitmap->Bits();
		int32 source_bpr = intermediate_bitmap->BytesPerRow()/4;
		int32 bottom = (int32)new_bitmap->Bounds().bottom;
		float diff = (starting_height-1)/(new_height);
		float accumulation = 0;
		if (diff<1) {
			// Make larger in y direction.
			for (int32 y=0;y<=bottom;y++) {
				for (int32 x=0;x<target_bpr;x++) {
//					*target_bits++ = mix_2_pixels(*(source_bits + (int32)floor(accumulation)*source_bpr),*(source_bits + (int32)ceil(accumulation)*source_bpr),ceil(accumulation)-accumulation);
					*target_bits++ = mix_2_pixels_fixed(*(source_bits + (int32)floor(accumulation)*source_bpr),*(source_bits + (int32)ceil(accumulation)*source_bpr),(uint32)(32768*(ceil(accumulation)-accumulation)));
					source_bits++;
				}
				source_bits -= source_bpr;
				accumulation += diff;
				if ((y%10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}
		else if (diff>1) {
			// Make smaller in y direction.
			diff = (starting_height-1)/new_height;
			accumulation = 0;
			for (int32 y=0;y<=bottom;y++) {
				for (int32 x=0;x<target_bpr;x++) {
					// Here we average the original pixels between accumulation and accumulation+diff.
					// The pixels at end get a little lower coefficients than the other pixels.
					// But for now we just settle for averaging the pixels between floor(accumulation)
					// and floor(accumulation+diff):
					uint32 target_value = 0x00000000;
					float coeff = 1.0;
					float coeff_diff = 1.0/(floor(accumulation+diff)-floor(accumulation));
					for (int32 i=(int32)floor(accumulation);i<floor(accumulation+diff);i++) {
//						target_value = mix_2_pixels(*(source_bits + i*source_bpr),target_value,coeff);
						target_value = mix_2_pixels_fixed(*(source_bits + i*source_bpr),target_value,(uint32)(32768*coeff));
						coeff -= coeff_diff;
					}
					*target_bits++ = target_value;
					source_bits++;
				}
				source_bits -= source_bpr;
				accumulation += diff;
				if ((y%10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}
		else {
			for (int32 y=0;y<=bottom;y++) {
				for (int32 x=0;x<target_bpr;x++) {
					// Just copy it straight
					*target_bits++ = *source_bits++;
				}
				if ((y%10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}

		if (intermediate_bitmap != original)
			delete intermediate_bitmap;

		return new_bitmap;
	}
	else
		return intermediate_bitmap;
}


int32 ScaleManipulator::PreviewBitmap(Selection*,bool,BRegion *region)
{
	if (preview_bitmap == NULL)
		return 0;

	union {
		uint8 bytes[4];
		uint32 word;
	} white;

	white.bytes[0] = 0xFF;
	white.bytes[1] = 0xFF;
	white.bytes[2] = 0xFF;
	white.bytes[3] = 0x00;

	// Here do a DDA-scaling from copy_of_the_preview_bitmap to preview_bitmap.
	int32 width = preview_bitmap->Bounds().IntegerWidth();
	int32 height = preview_bitmap->Bounds().IntegerHeight();

	uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	uint32 *target_bits = (uint32*)preview_bitmap->Bits();
	int32 bpr = preview_bitmap->BytesPerRow()/4;

	float width_coeff = 1.0 / settings->width_coefficient;
	float height_coeff = 1.0 / settings->height_coefficient;

	int32 *source_x_table = new int32[width+1];
	for (int32 i=0;i<=width;i++)
		source_x_table[i] = (int32)floor(i*width_coeff);


	for (int32 y=0;y<=height;y++) {
		int32 source_y = (int32)floor(y*height_coeff);
		int32 y_times_bpr = y*bpr;
		int32 source_y_times_bpr = source_y*bpr;
		for (register int32 x=0;x<=width;x++) {
			int32 source_x = source_x_table[x];
			if ((source_x > width) ||(source_y > height)) {
				*(target_bits + x + y_times_bpr) = white.word;
			}
			else {
				*(target_bits +x + y_times_bpr) = *(source_bits + source_x + source_y_times_bpr);
			}
		}
	}

	region->Set(preview_bitmap->Bounds());

	delete[] source_x_table;
	return 1;
}

void ScaleManipulator::MouseDown(BPoint point,uint32 buttons,BView*,bool first_click)
{
	if ((point.x > 0) && (point.y > 0)) {
		if (configuration_view != NULL) {
			settings->width_coefficient = point.x/original_width;
			settings->height_coefficient = point.y/original_height;
			if (configuration_view->MaintainProportions() == TRUE) {
				settings->width_coefficient = max_c(settings->width_coefficient,settings->height_coefficient);
				settings->height_coefficient = settings->width_coefficient;
			}

			configuration_view->SetValues(settings->width_coefficient*original_width,settings->height_coefficient*original_height);
		}
		else {
			settings->width_coefficient = point.x/original_width;
			settings->height_coefficient = point.y/original_height;
		}
	}
}



void ScaleManipulator::SetValues(float width, float height)
{
	settings->width_coefficient = width / original_width;
	settings->height_coefficient = height / original_height;
}


void ScaleManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}

void ScaleManipulator::SetPreviewBitmap(BBitmap *bitmap)
{
	if (bitmap != NULL) {
		original_width = bitmap->Bounds().Width()+1;
		original_height = bitmap->Bounds().Height()+1;

		if (configuration_view != NULL) {
			configuration_view->SetValues(original_width,original_height);
		}
	}

	if ((bitmap == NULL) || (preview_bitmap == NULL) || (bitmap->Bounds() != preview_bitmap->Bounds())) {
		try {
			if (preview_bitmap != NULL) {
				delete copy_of_the_preview_bitmap;
			}
			if (bitmap != NULL) {
				preview_bitmap = bitmap;
				copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
			}
			else {
				preview_bitmap = NULL;
				copy_of_the_preview_bitmap = NULL;
			}
		}
		catch (bad_alloc e) {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap=NULL;
			throw e;
		}
	}
	else {
		// Just update the copy_of_the_preview_bitmap
		preview_bitmap = bitmap;
		uint32 *source = (uint32*)preview_bitmap->Bits();
		uint32 *target = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bitslength = min_c(preview_bitmap->BitsLength(),copy_of_the_preview_bitmap->BitsLength());
		memcpy(target,source,bitslength);
	}
}


ManipulatorSettings* ScaleManipulator::ReturnSettings()
{
	return new ScaleManipulatorSettings(settings);
}

BView* ScaleManipulator::MakeConfigurationView(BMessenger *messenger)
{
	configuration_view = new ScaleManipulatorView(BRect(0,0,0,0),this,messenger);
	if (configuration_view != NULL) {
		configuration_view->SetValues(original_width,original_height);
	}
	return configuration_view;
}


const char*	ScaleManipulator::ReturnHelpString()
{
	return StringServer::ReturnString(DO_SCALE_HELP_STRING);
}

const char*	ScaleManipulator::ReturnName()
{
	return StringServer::ReturnString(SCALE_STRING);
}




ScaleManipulatorView::ScaleManipulatorView(BRect rect,ScaleManipulator *manip,BMessenger *t)
	: WindowGUIManipulatorView(rect)
{
	target = new BMessenger(*t);
	BMessage *message;
	manipulator = manip;

	BRect control_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE);
	width_control = new NumberControl(control_frame,"width_control",StringServer::ReturnString(WIDTH_STRING),"",new BMessage(WIDTH_CHANGED),5);
	width_control->ResizeToPreferred();
	BRect button_frame;
	button_frame = control_frame = width_control->Frame();
	control_frame.OffsetBy(0,control_frame.Height()+EXTRA_EDGE);
	button_frame.OffsetBy(button_frame.Width()+EXTRA_EDGE,0);
	message = new BMessage(MULTIPLY_WIDTH);
	message->AddFloat("coefficient",2);
	BButton *a_button = new BButton(button_frame,"multiply width","x2",message);
	a_button->ResizeTo(button_frame.Height(),button_frame.Height());
	AddChild(a_button);

	button_frame = a_button->Frame();
	button_frame.OffsetBy(button_frame.Width()+EXTRA_EDGE,0);
	message = new BMessage(MULTIPLY_WIDTH);
	message->AddFloat("coefficient",0.5);
	a_button = new BButton(button_frame,"multiply width","/2",message);
	a_button->ResizeTo(button_frame.Height(),button_frame.Height());
	AddChild(a_button);

	button_frame = a_button->Frame();
	button_frame.OffsetBy(button_frame.Width()+EXTRA_EDGE,0);
	message = new BMessage(RESTORE_WIDTH);
	a_button = new BButton(button_frame,"restore width","R",message);
	a_button->ResizeTo(button_frame.Height(),button_frame.Height());
	AddChild(a_button);

	BCheckBox *proportion_box;
	proportion_box = new BCheckBox(control_frame,"proportion_box",StringServer::ReturnString(LOCK_PROPORTIONS_STRING),new BMessage(PROPORTION_CHANGED));
	proportion_box->ResizeToPreferred();
	proportion_box->SetValue(B_CONTROL_ON);
	AddChild(proportion_box);
	maintain_proportions = TRUE;
//	control_frame = proportion_box->Frame();
	control_frame.OffsetBy(0,control_frame.Height()+EXTRA_EDGE);

	height_control = new NumberControl(control_frame,"height_control",StringServer::ReturnString(HEIGHT_STRING),"",new BMessage(HEIGHT_CHANGED),5);
	AddChild(width_control);
	AddChild(height_control);

	button_frame = height_control->Frame();
	button_frame.OffsetBy(button_frame.Width()+EXTRA_EDGE,0);
	message = new BMessage(MULTIPLY_HEIGHT);
	message->AddFloat("coefficient",2);
	a_button = new BButton(button_frame,"multiply height","x2",message);
	a_button->ResizeTo(button_frame.Height(),button_frame.Height());
	AddChild(a_button);

	proportion_box->MoveTo(a_button->Frame().left,proportion_box->Frame().top);

	button_frame = a_button->Frame();
	button_frame.OffsetBy(button_frame.Width()+EXTRA_EDGE,0);
	message = new BMessage(MULTIPLY_HEIGHT);
	message->AddFloat("coefficient",0.5);
	a_button = new BButton(button_frame,"multiply height","/2",message);
	a_button->ResizeTo(button_frame.Height(),button_frame.Height());
	AddChild(a_button);

	button_frame = a_button->Frame();
	button_frame.OffsetBy(button_frame.Width()+EXTRA_EDGE,0);
	message = new BMessage(RESTORE_HEIGHT);
	message->AddFloat("coefficient",0.5);
	a_button = new BButton(button_frame,"restore height","R",message);
	a_button->ResizeTo(button_frame.Height(),button_frame.Height());
	AddChild(a_button);

	ResizeTo(proportion_box->Frame().right+EXTRA_EDGE,a_button->Frame().bottom+EXTRA_EDGE);

	// Make the NumberControls the same size
	float divider = width_control->Divider();
	divider = max_c(divider,height_control->Divider());
	width_control->SetDivider(divider);
	height_control->SetDivider(divider);


	original_width = -1;
	original_height = -1;
}

ScaleManipulatorView::~ScaleManipulatorView()
{
	delete target;
}


void ScaleManipulatorView::AttachedToWindow()
{
	BControl *control;
	for (int32 i=0;i<CountChildren();i++) {
		control = cast_as(ChildAt(i),BControl);
		if (control != NULL)
			control->SetTarget(this);
	}

	width_control->MakeFocus(true);
	WindowGUIManipulatorView::AttachedToWindow();
}


void ScaleManipulatorView::MessageReceived(BMessage *message)
{
	float coefficient;

	switch (message->what) {
		case WIDTH_CHANGED:
		case HEIGHT_CHANGED:
			if (message->what == WIDTH_CHANGED) {
				current_width = width_control->Value();
				current_width = max_c(1,ceil(current_width));
				// Need to round the height correctly to the nearest pixel
				if (maintain_proportions) {
					current_height = max_c(1, floor(original_height * (current_width / original_width) + 0.5));
					height_control->SetValue(current_height);
				}

				width_control->SetValue(current_width);
			}
			else {
				current_height = height_control->Value();
				current_height = max_c(1,ceil(current_height));
				if (maintain_proportions) {
					current_width = max_c(1, floor(original_width * (current_height / original_height) + 0.5));
					width_control->SetValue(current_width);
				}
				height_control->SetValue(current_height);
			}
			if (manipulator != NULL)
				manipulator->SetValues(current_width,current_height);

			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case MULTIPLY_WIDTH:
		case MULTIPLY_HEIGHT:
			if (message->FindFloat("coefficient",&coefficient) == B_NO_ERROR) {
				if (message->what == MULTIPLY_WIDTH) {
					current_width = max_c(1,ceil(coefficient*current_width));
					if (maintain_proportions) {
						current_height = max_c(1, floor(original_height * (current_width / original_width) + 0.5));
						height_control->SetValue(current_height);
					}
					width_control->SetValue(current_width);
					Window()->PostMessage(WIDTH_CHANGED,this);
				}
				else {
					current_height = max_c(1,ceil(coefficient*current_height));
					if (maintain_proportions) {
						current_width = max_c(1, floor(original_width * (current_height / original_height) + 0.5));
						width_control->SetValue(current_width);
					}
					height_control->SetValue(current_height);
					Window()->PostMessage(HEIGHT_CHANGED,this);
				}
//				if (manipulator != NULL)
//					manipulator->SetValues(current_width,current_height);
//
//				target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			}
			break;

		case RESTORE_HEIGHT:
		case RESTORE_WIDTH:
			if (message->what == RESTORE_WIDTH) {
				width_control->SetValue(original_width);
				Window()->PostMessage(WIDTH_CHANGED,this);
			}
			else if (message->what == RESTORE_HEIGHT) {
				height_control->SetValue(original_height);
				Window()->PostMessage(HEIGHT_CHANGED,this);
			}
//			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case PROPORTION_CHANGED:
			maintain_proportions = !maintain_proportions;
			if (maintain_proportions == TRUE) {
				current_height = max_c(1, floor(original_height * (current_width/original_width) + 0.5));
				if (manipulator != NULL) {
					manipulator->SetValues(current_width,current_height);
					width_control->SetValue(current_width);
					height_control->SetValue(current_height);
				}
			}
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void ScaleManipulatorView::SetValues(float width, float height)
{
	if (original_width <= 0) {
		original_width = round(width);
		original_height = round(height);
	}
	current_width = round(width);
	current_height = round(height);

//	char text[256];

//	sprintf(text,"%.0f",original_width);
//	BTextControl *text_control = cast_as(FindView("width_control"),BTextControl);
//	if (text_control != NULL) {
//		text_control->SetText(text);
//	}
//	sprintf(text,"%.0f",original_height);
//
//	text_control = cast_as(FindView("height_control"),BTextControl);
//	if (text_control != NULL) {
//		text_control->SetText(text);
//	}

	if (LockLooper()) {
		BControl *text_control = dynamic_cast<BControl*>(FindView("width_control"));
		if (text_control != NULL) {
			text_control->SetValue(current_width);
		}

		text_control = dynamic_cast<BControl*>(FindView("height_control"));
		if (text_control != NULL) {
			text_control->SetValue(current_height);
		}
		UnlockLooper();
	}
	else {
		BControl *text_control = dynamic_cast<BControl*>(FindView("width_control"));
		if (text_control != NULL) {
			text_control->SetValue(current_width);
		}

		text_control = dynamic_cast<BControl*>(FindView("height_control"));
		if (text_control != NULL) {
			text_control->SetValue(current_height);
		}
	}
}
