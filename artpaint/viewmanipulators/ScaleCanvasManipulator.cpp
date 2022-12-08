/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ScaleCanvasManipulator.h"

#include "MessageConstants.h"
#include "NumberControl.h"
#include "PixelOperations.h"
#include "Selection.h"


#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayout.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <SpaceLayoutItem.h>
#include <StatusBar.h>
#include <Window.h>


#include <new>
#include <string.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Manipulators"


using ArtPaint::Interface::NumberControl;


ScaleCanvasManipulator::ScaleCanvasManipulator(BBitmap *bm)
	:	WindowGUIManipulator(),
	selection(NULL)
{
	configuration_view = NULL;
	settings = new ScaleCanvasManipulatorSettings();

	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bm);
}



ScaleCanvasManipulator::~ScaleCanvasManipulator()
{
	if (configuration_view != NULL) {
		configuration_view->RemoveSelf();
		delete configuration_view;
	}

	delete copy_of_the_preview_bitmap;
}



BBitmap* ScaleCanvasManipulator::ManipulateBitmap(ManipulatorSettings *set,
	BBitmap *original, BStatusBar *status_bar)
{
	ScaleCanvasManipulatorSettings *new_settings =
			dynamic_cast<ScaleCanvasManipulatorSettings*> (set);
	if (new_settings == NULL)
		return NULL;

	BBitmap *final_bitmap = NULL;	
	
	BRect bounds = original->Bounds();
	
	if (bounds.IsValid() == FALSE)
		return NULL;
	
	union color_conversion background;
	// Transparent background.
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;
	
	if (original == preview_bitmap) {
		final_bitmap = original;
		original = copy_of_the_preview_bitmap;
	}
	
	uint32 *source_bits; 
	uint32 *target_bits;
		
	float starting_width = bounds.Width() + 1;
	float starting_height = bounds.Height() + 1;

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

	BBitmap *scale_x_bitmap = NULL;
	BBitmap *scale_y_bitmap = NULL;
	
	if (new_width != starting_width) {
		float bitmapWidth = new_width - 1;
		float bitmapHeight = starting_height - 1;
		scale_x_bitmap = new BBitmap(BRect(0, 0, bitmapWidth, bitmapHeight), 
			B_RGBA32);
		if (scale_x_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();
			
		target_bits = (uint32*)scale_x_bitmap->Bits();
		int32 target_bpr = scale_x_bitmap->BytesPerRow() / 4;
		source_bits = (uint32*)original->Bits();
		int32 source_bpr = original->BytesPerRow() / 4;
		float diff = starting_width / new_width;
		float accumulation = 0;
		int32 left = (int32)bounds.left;
		int32 top = (int32)bounds.top;
		int32 right = left + new_width - 1;
		int32 bottom = (int32)bounds.bottom;

		for (int32 y = 0; y < starting_height; y++)
			for (int32 x = 0; x < target_bpr; x++)
				*(target_bits + x + y * target_bpr) = 
					background.word;
					
		if (diff < 1) {
			// Enlarge in x direction.
			for (int32 y = 0; y < starting_height; y++) {
				accumulation = 0;
				uint32* src_bits = source_bits + left + (y + top) * source_bpr;
				
				for (int32 x = 0; x < target_bpr; x++) {
					// 'mix_2_pixels' doesn't work -- doesn't take rounding errors into account.
					*(target_bits + x + y * target_bpr) = 
						src_over_fixed (*(target_bits + x + y * target_bpr),
							mix_2_pixels_fixed(*(src_bits + (int32)floor(accumulation)),
									*(src_bits + (int32)ceil(accumulation)),
									(uint32)(32768 * (ceil(accumulation) - accumulation)))
						);
					accumulation += diff;
					
					if (accumulation > source_bpr - 1) 
						accumulation = source_bpr - 1;
				}
				
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		} else if (diff > 1) {
			// Make smaller in x direction.
			for (int32 y = 0; y <= starting_height - 1; y++) {
				uint32* src_bits = source_bits + (y + top) * source_bpr;
				float fx = left;
				for (int32 x = 0; x < target_bpr; x++) {
					// Here we average the original pixels between accumulation and accumulation+diff.
					// The pixels at end get a little lower coefficients than the other pixels.
					// But for now we just settle for averaging the pixels between floor(accumulation)
					// and floor(accumulation+diff):
					float coeff = 1.0;
					int32 x_diff = floor(fx + diff);
					float coeff_diff = 0;
					if (x_diff - x != 0) 
						coeff_diff = 1.0 / (float)(x_diff - x);
					uint32 target_value = 0x00000000;
					for (int32 i = (int32)floor(fx); i < x_diff; i++) {
						target_value = mix_2_pixels_fixed(*(src_bits + i),
							target_value, (uint32)(32768 * coeff));
						coeff -= coeff_diff;
					}
					
					*(target_bits + x + y * target_bpr) = target_value;
					fx += diff;
				}
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		} else {
			for (int32 y = 0; y <= starting_height; y++) {
				uint32* src_bits = source_bits + y * source_bpr;
				
				for (int32 x = 0; x < target_bpr; x++) {
					// Just copy it straight
					*(target_bits + x + y * target_bpr) = *(src_bits + x);
				}
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}
	} else {
		scale_x_bitmap = original;
	}

	if (new_height != starting_height) {
		float bitmapWidth = new_width - 1;
		float bitmapHeight = new_height - 1;

		scale_y_bitmap = new BBitmap(BRect(0, 0 , bitmapWidth, 
			bitmapHeight), B_RGBA32);
		if (scale_y_bitmap->IsValid() == FALSE)
			throw std::bad_alloc();

		target_bits = (uint32*)scale_y_bitmap->Bits();
		int32 target_bpr = scale_y_bitmap->BytesPerRow() / 4;
		source_bits = (uint32*)scale_x_bitmap->Bits();
		int32 source_bpr = scale_x_bitmap->BytesPerRow() / 4;
		
		for (int32 y = 0; y < new_height; y++) 
			for (int32 x = 0; x < new_width; x++) 
				*(target_bits + x + y * target_bpr) = 
					background.word;	
						
		int32 top = (int32)bounds.top;
		int32 left = (int32)bounds.left;
		int32 bottom = (int32)scale_y_bitmap->Bounds().bottom;	
		int32 right = (int32)scale_y_bitmap->Bounds().right;
		
		if (scale_x_bitmap != original) {
			left = 0;
			top = 0;
		}
		
		float diff = (starting_height - 1) / new_height;
		float accumulation = 0;
		if (diff < 1) {
			// Make larger in y direction.
			float fy = top;
			for (int32 y = 0; y <= new_height - 1; y++) {
				int32 y_diff = (int32)floor(fy + diff);
				for (int32 x = 0; x < new_width - 1; x++) {
					*(target_bits + x + y * target_bpr) = 
						mix_2_pixels_fixed(*(source_bits + x + left + (int32)floor(fy) * source_bpr),
							*(source_bits + x + left + y_diff * source_bpr),
							(uint32)(32768 * (ceil(accumulation) - accumulation)));
				}
				fy += diff;
				accumulation += diff;
				if (accumulation > source_bpr - 1) 
					accumulation = source_bpr - 1;
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		} else if (diff > 1) {
			// Make smaller in y direction.
			float fy = top;
			for (int32 y = 0; y <= new_height - 1; y++) {
				for (int32 x = 0; x < new_width - 1; x++) {
					// Here we average the original pixels between accumulation and accumulation+diff.
					// The pixels at end get a little lower coefficients than the other pixels.
					// But for now we just settle for averaging the pixels between floor(accumulation)
					// and floor(accumulation+diff):
					uint32 target_value = 0x00000000;
					float coeff = 1.0;
					int32 y_diff = floor(fy + diff);
					float coeff_diff = 0;
					if (y_diff - y != 0)
						coeff_diff = 1.0 / (y_diff - y);
					for (int32 i = (int32)floor(fy); i < y_diff; i++) {
						target_value = mix_2_pixels_fixed(*(source_bits + x + left + i * source_bpr),
							target_value, (uint32)(32768 * coeff));
						coeff -= coeff_diff;
					}
					*(target_bits + x + y * target_bpr) = target_value;				
				}
				fy += diff;
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		} else {
			for (int32 y = 0; y <= bottom; y++) {
				for (int32 x = 0; x < target_bpr; x++) {
					// Just copy it straight
					*target_bits++ = *source_bits++;
				}
				if ((y % 10 == 0) && (status_bar != NULL)) {
					progress_message.ReplaceFloat("delta",(100.0)/bottom*10.0/2);
					status_bar->Window()->PostMessage(&progress_message,status_bar);
				}
			}
		}

		if (scale_x_bitmap != original)
			delete scale_x_bitmap;
	}
	
	if (scale_y_bitmap != NULL)
		return scale_y_bitmap;
	
	return scale_x_bitmap;	
}


int32 ScaleCanvasManipulator::PreviewBitmap(bool, BRegion* region)
{
	if (preview_bitmap == NULL)
		return 0;

	union color_conversion white;
	white.bytes[0] = 0xFF;
	white.bytes[1] = 0xFF;
	white.bytes[2] = 0xFF;
	white.bytes[3] = 0x00;

	// Here do a DDA-scaling from copy_of_the_preview_bitmap to preview_bitmap.
	uint32 width = preview_bitmap->Bounds().IntegerWidth();
	uint32 height = preview_bitmap->Bounds().IntegerHeight();

	if (width == 0 || height == 0)
		return 0;

	uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	uint32 *target_bits = (uint32*)preview_bitmap->Bits();
	int32 bpr = preview_bitmap->BytesPerRow()/4;

	float width_coeff = 1.0 / settings->width_coefficient;
	float height_coeff = 1.0 / settings->height_coefficient;

	int32 *source_x_table = new int32[width + 1];

	for (int32 i = 0; i <= width; i++)
		source_x_table[i] = (int32)floor(i * width_coeff);

	for (int32 y = 0; y <= height; y++) {
		int32 source_y = (int32)floor(y * height_coeff);
		int32 y_times_bpr = y * bpr;
		int32 source_y_times_bpr = source_y * bpr;
		for (int32 x = 0; x <= width; x++) {
			int32 source_x = source_x_table[x];
			if (source_x > width || source_y > height)
				*(target_bits + x + y_times_bpr) = white.word;
			else
				*(target_bits + x + y_times_bpr) =
					*(source_bits + source_x + source_y_times_bpr);
		}
	}

	region->Set(preview_bitmap->Bounds());
	
	delete[] source_x_table;
	return 1;
}


void
ScaleCanvasManipulator::MouseDown(BPoint point, uint32 buttons, BView* image_view,
	bool first_click)
{
	BPoint normPoint(point);
	
	if (normPoint.x > 0.0 && normPoint.y > 0.0) {
		settings->width_coefficient = normPoint.x / original_width;
		settings->height_coefficient = normPoint.y / original_height;

		if (configuration_view) {
			if (configuration_view->MaintainProportions()) {
				settings->width_coefficient = max_c(settings->width_coefficient,
					settings->height_coefficient);
				settings->height_coefficient = settings->width_coefficient;
			}

			configuration_view->SetValues(settings->width_coefficient
				* original_width, settings->height_coefficient * original_height);
		}

		if (first_click == true) {
			configuration_view->SetValues(settings->width_coefficient
				* original_width, settings->height_coefficient * original_height);
		}
	}
}


void
ScaleCanvasManipulator::SetValues(float width, float height)
{
	settings->width_coefficient = width / original_width;
	settings->height_coefficient = height / original_height;
}


void
ScaleCanvasManipulator::Reset()
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		if (source != NULL)
			memcpy(target, source, preview_bitmap->BitsLength());
	}
}


void
ScaleCanvasManipulator::SetPreviewBitmap(BBitmap *bitmap)
{
	if (bitmap) {
		original_width = bitmap->Bounds().Width() + 1.0;
		original_height = bitmap->Bounds().Height() + 1.0;

		if (configuration_view)
			configuration_view->SetValues(original_width, original_height);
	}

	if (!bitmap || !preview_bitmap
		|| bitmap->Bounds() != preview_bitmap->Bounds()) {
		try {
			if (preview_bitmap)
				delete copy_of_the_preview_bitmap;

			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;

			if (bitmap) {
				preview_bitmap = bitmap;
				copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
			}
		} catch (std::bad_alloc e) {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
			throw e;
		}
	} else {
		// Just update the copy_of_the_preview_bitmap
		preview_bitmap = bitmap;
		uint32 *source = (uint32*)preview_bitmap->Bits();
		uint32 *target = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bitslength = min_c(preview_bitmap->BitsLength(),
				copy_of_the_preview_bitmap->BitsLength());
		memcpy(target, source, bitslength);
	}
}


void
ScaleCanvasManipulator::SetSelection(Selection* new_selection)
{
	selection = new_selection;
};


ManipulatorSettings*
ScaleCanvasManipulator::ReturnSettings()
{
	return new (std::nothrow) ScaleCanvasManipulatorSettings(settings);
}


BView*
ScaleCanvasManipulator::MakeConfigurationView(const BMessenger& target)
{
	configuration_view = new (std::nothrow) ScaleCanvasManipulatorView(this, target);
	if (configuration_view)
		configuration_view->SetValues(original_width, original_height);
	return configuration_view;
}


const char*
ScaleCanvasManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Scale: Drag to resize or use number-fields.");
}


const char*
ScaleCanvasManipulator::ReturnName()
{
	return B_TRANSLATE("Scale" B_UTF8_ELLIPSIS);
}


// #pragma mark -- ScaleCanvasManipulatorView


ScaleCanvasManipulatorView::ScaleCanvasManipulatorView(ScaleCanvasManipulator* manipulator,
		const BMessenger& target)
	: WindowGUIManipulatorView()
	, fTarget(target)
	, fManipulator(manipulator)
{
	original_width = -1;
	original_height = -1;
	maintain_proportions = true;

	width_control = new NumberControl(B_TRANSLATE("Width:"),
		"", new BMessage(WIDTH_CHANGED), 5);

	height_control = new NumberControl(B_TRANSLATE("Height:"),
		"", new BMessage(HEIGHT_CHANGED), 5);

	SetLayout(new BGroupLayout(B_HORIZONTAL, 5.0));
	AddChild(BGridLayoutBuilder(5.0, 5.0)
		.Add(width_control, 0, 0)
		.Add(_MakeButton("x2", MULTIPLY_WIDTH, 2.0), 1, 0)
		.Add(_MakeButton("/2", MULTIPLY_WIDTH, 0.5), 2, 0)
		.Add(new BButton(B_TRANSLATE("Restore"), new BMessage(RESTORE_WIDTH)), 3, 0)
		.Add(height_control, 0, 2)
		.Add(_MakeButton("x2", MULTIPLY_HEIGHT, 2.0), 1, 2)
		.Add(_MakeButton("/2", MULTIPLY_HEIGHT, 0.5), 2, 2)
		.Add(new BButton(B_TRANSLATE("Restore"), new BMessage(RESTORE_HEIGHT)), 3, 2)
	);

	BCheckBox* proportion_box = new BCheckBox(B_TRANSLATE_COMMENT("Lock", "Keep the aspect ratio"),
		new BMessage(PROPORTION_CHANGED));
	proportion_box->SetValue(B_CONTROL_ON);
	AddChild(proportion_box);
}


void
ScaleCanvasManipulatorView::AttachedToWindow()
{
	_SetTarget(this);

	width_control->MakeFocus(true);
	WindowGUIManipulatorView::AttachedToWindow();
}


void
ScaleCanvasManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case WIDTH_CHANGED: {
		case HEIGHT_CHANGED:
//			message->PrintToStream();
			if (message->what == WIDTH_CHANGED) {
				current_width = width_control->Value();
				current_width = max_c(1.0, ceil(current_width));
				// Need to round the height correctly to the nearest pixel
				if (maintain_proportions) {
					current_height = max_c(1.0, floor(original_height
						* (current_width / original_width) + 0.5));
					height_control->SetValue(int32(current_height));
				}
				width_control->SetValue(int32(current_width));
			} else {
				current_height = height_control->Value();
				current_height = max_c(1.0, ceil(current_height));
				if (maintain_proportions) {
					current_width = max_c(1.0, floor(original_width
						* (current_height / original_height) + 0.5));
					width_control->SetValue(int32(current_width));
				}
				height_control->SetValue(int32(current_height));
			}

			if (fManipulator)
				fManipulator->SetValues(current_width, current_height);
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;

		case MULTIPLY_WIDTH: {
		case MULTIPLY_HEIGHT:
			float coefficient;
//			message->PrintToStream();
			if (message->FindFloat("coefficient", &coefficient) == B_OK) {
				if (message->what == MULTIPLY_WIDTH) {
					current_width = max_c(1.0, ceil(coefficient * current_width));
					if (maintain_proportions) {
						current_height = max_c(1.0, floor(original_height
							* (current_width / original_width) + 0.5));
						height_control->SetValue(int32(current_height));
					}
					width_control->SetValue(int32(current_width));
					Window()->PostMessage(WIDTH_CHANGED, this);
				} else {
					current_height = max_c(1.0, ceil(coefficient * current_height));
					if (maintain_proportions) {
						current_width = max_c(1.0, floor(original_width
							* (current_height / original_height) + 0.5));
						width_control->SetValue(int32(current_width));
					}
					height_control->SetValue(int32(current_height));
					Window()->PostMessage(HEIGHT_CHANGED, this);
				}
			}
		}	break;

		case RESTORE_HEIGHT: {
		case RESTORE_WIDTH:
//			message->PrintToStream();
			if (message->what == RESTORE_WIDTH) {
				width_control->SetValue(int32(original_width));
				Window()->PostMessage(WIDTH_CHANGED,this);
			} else if (message->what == RESTORE_HEIGHT) {
				height_control->SetValue(int32(original_height));
				Window()->PostMessage(HEIGHT_CHANGED,this);
			}
		}	break;

		case PROPORTION_CHANGED: {
			maintain_proportions = !maintain_proportions;
			if (maintain_proportions) {
				current_height = max_c(1.0, floor(original_height
					* (current_width / original_width) + 0.5));
				if (fManipulator) {
					fManipulator->SetValues(current_width, current_height);
					width_control->SetValue(int32(current_width));
					height_control->SetValue(int32(current_height));
				}
			}
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;

		default: {
			WindowGUIManipulatorView::MessageReceived(message);
		}	break;
	}
}


void
ScaleCanvasManipulatorView::SetValues(float width, float height)
{
	current_width = round(width);
	current_height = round(height);

	if (original_width <= 0) {
		original_width = current_width;
		original_height = current_height;
	}

	// TODO: check why we try locking first and on error just access
	if (LockLooper()) {
		_SetValues(current_width, current_height);
		UnlockLooper();
	} else {
		_SetValues(current_width, current_height);
	}
}


void
ScaleCanvasManipulatorView::_SetTarget(BView* view)
{
	for (int32 i = 0; i < view->CountChildren(); ++i) {
		BView* child = view->ChildAt(i);
		if (child->CountChildren() > 0)
			_SetTarget(child);
		if (BControl* control = dynamic_cast<BControl*> (child))
			control->SetTarget(this);
	}
}


void
ScaleCanvasManipulatorView::_SetValues(float width, float height)
{
	// TODO: check why we cast to BControl here
	BControl *text_control = dynamic_cast<BControl*> (width_control);
	if (text_control)
		text_control->SetValue(int32(current_width));

	text_control = dynamic_cast<BControl*> (height_control);
	if (text_control != NULL)
		text_control->SetValue(int32(current_height));
}


BButton*
ScaleCanvasManipulatorView::_MakeButton(const char* label, uint32 what,
	float coefficient)
{
	BMessage* message = new BMessage(what);
	message->AddFloat("coefficient", coefficient);

	BButton* button = new BButton(label, message);

	float width, height;
	button->GetPreferredSize(&width, &height);

	// TODO: this sucks, file bugreport for HAIKU
	button->SetExplicitMaxSize(BSize(height, B_SIZE_UNSET));
	button->SetExplicitMinSize(BSize(height, B_SIZE_UNSET));

	return button;
}
