/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <ClassInfo.h>
#include <new>
#include <StatusBar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Window.h>


#include "ImageView.h"
#include "MessageConstants.h"
#include "NumberControl.h"
#include "Selection.h"
#include "StringServer.h"
#include "TranslationManipulator.h"


using ArtPaint::Interface::NumberControl;


TranslationManipulator::TranslationManipulator(BBitmap *bm)
	:	StatusBarGUIManipulator()
	, copy_of_the_preview_bitmap(NULL)
{
	preview_bitmap = bm;
	if (preview_bitmap != NULL)
		copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);

	settings = new TranslationManipulatorSettings();
	previous_x_translation = 0;
	previous_y_translation = 0;

	last_calculated_resolution = 0;
	lowest_available_quality = 4;
}


TranslationManipulator::~TranslationManipulator()
{
	delete settings;
	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}

}


void TranslationManipulator::MouseDown(BPoint point,uint32,BView*,bool first)
{
	if (first)
		previous_point = point;
	else {
//		previous_x_translation = settings->x_translation;
//		previous_y_translation = settings->y_translation;

		settings->x_translation += point.x - previous_point.x;
		settings->y_translation += point.y - previous_point.y;

		previous_point = point;

		if (config_view != NULL)
			config_view->SetValues(settings->x_translation,settings->y_translation);
	}
}


BBitmap* TranslationManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
//	printf("TranslationManipulator::ManipulateBitmap\n");
	TranslationManipulatorSettings *new_settings = cast_as(set,TranslationManipulatorSettings);

	if ((status_bar != NULL) && (status_bar->Window() != NULL)) {
		if (status_bar->Window()->LockWithTimeout(0) == B_OK) {
			status_bar->Update(100);
			status_bar->Window()->Unlock();
		}
	}

	if (new_settings == NULL) {
		return NULL;
	}
	if (original == NULL) {
		return NULL;
	}
	if ((new_settings->x_translation == 0) && (new_settings->y_translation == 0)) {
		return NULL;
	}

	BBitmap *new_bitmap;
	if (original != preview_bitmap) {
		original->Lock();
		BRect bitmap_frame = original->Bounds();
		new_bitmap = new BBitmap(bitmap_frame,B_RGB_32_BIT);
		original->Unlock();
		if (new_bitmap->IsValid() == FALSE) {
			throw std::bad_alloc();
		}
	}
	else {
		new_bitmap = original;
		original = copy_of_the_preview_bitmap;
	}

	union {
		uint8	bytes[4];
		uint32	word;
	} background;
	// Transparent background.
	background.bytes[0] = 0xFF;
	background.bytes[1] = 0xFF;
	background.bytes[2] = 0xFF;
	background.bytes[3] = 0x00;


	// This function assumes that the both bitmaps are of same size.
	uint32 *target_bits = (uint32*)new_bitmap->Bits();
	uint32 *source_bits = (uint32*)original->Bits();
	uint32 target_bpr = new_bitmap->BytesPerRow()/4;
	uint32 source_bpr = original->BytesPerRow()/4;
//	printf("test5\n");
	int32 width = (int32)min_c(new_bitmap->Bounds().Width()+1,original->Bounds().Width()+1);
	int32 height = (int32)min_c(new_bitmap->Bounds().Height()+1,original->Bounds().Height()+1);
//	printf("%d %d\n",width,height);

	// We have to copy translations so that we do translation for all pixels
	// with the same values
	int32 x_translation_local = ((int32)new_settings->x_translation);
	int32 y_translation_local = ((int32)new_settings->y_translation);
//	printf("%d %d\n",x_translation_local,y_translation_local);
	if (selection->IsEmpty()) {
//		printf("Selection is empty\n");
		// First clear the target bitmap. Here the clearing of the whole bitmap is not usually necessary.
		// Actually this loop combined with the next should only set each pixel in the bitmap exactly once.
		for (int32 y=0;y<height;y++) {
			for (int32 x=0;x<width;x++) {
				*(target_bits + x + y*target_bpr) = background.word;
			}
		}
//		printf("Cleared the target\n");
		// Copy only the points that are within the intersection of
		// original_bitmap->Bounds() and original_bitmap->Bounds().OffsetBy(x_translation,y_translation)
		int32 target_x_offset = max_c(0,x_translation_local);
		int32 target_y_offset = max_c(0,y_translation_local);
		int32 source_x_offset = max_c(0,-x_translation_local);
		int32 source_y_offset = max_c(0,-y_translation_local);

		for (int32 y = 0;y<height - abs(y_translation_local);y++) {
			for (int32 x = 0;x<width - abs(x_translation_local);x++) {
				*(target_bits + x+target_x_offset + (y + target_y_offset)*target_bpr)
				 = *(source_bits + x+source_x_offset + (y + source_y_offset)*source_bpr);
			}
		}
	}
	else {
		// First reset the picture
		for (int32 y=0;y<height;y++) {
			for (int32 x=0;x<width;x++) {
				*(target_bits + x + y*target_bpr) = *(source_bits + x + y*source_bpr);
			}
		}

		// Then clear the selection
		BRect selection_bounds = selection->GetBoundingRect();
		int32 left = (int32)selection_bounds.left;
		int32 right = (int32)selection_bounds.right;
		int32 top = (int32)selection_bounds.top;
		int32 bottom = (int32)selection_bounds.bottom;
		for (int32 y=top;y<=bottom;y++) {
			for (int32 x=left;x<=right;x++) {
				if (selection->ContainsPoint(x,y))
					*(target_bits + x + y*target_bpr) = background.word;
			}
		}


		selection_bounds = selection->GetBoundingRect();
		selection_bounds.OffsetBy(settings->x_translation,settings->y_translation);
		selection_bounds = selection_bounds & original->Bounds() & new_bitmap->Bounds();
		left = (int32)selection_bounds.left;
		right = (int32)selection_bounds.right;
		top = (int32)selection_bounds.top;
		bottom = (int32)selection_bounds.bottom;
		for (int32 y=top;y<=bottom;y++) {
			for (int32 x=left;x<=right;x++) {
				int32 new_x = (int32)(x - new_settings->x_translation);
				int32 new_y = (int32)(y - new_settings->y_translation);
				if (selection->ContainsPoint(new_x,new_y))
					*(target_bits + x + y*target_bpr) = *(source_bits + new_x + new_y*source_bpr);
			}
		}
//		printf("Did the translation\n");
	}

	return new_bitmap;
}


int32 TranslationManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	if ((preview_bitmap == NULL) || (copy_of_the_preview_bitmap == NULL))
		return 0;
	// First decide the resolution of the bitmap
	if ((previous_x_translation == settings->x_translation) && (previous_y_translation == settings->y_translation) && (full_quality == FALSE)) {
		if (last_calculated_resolution <= highest_available_quality) {
			last_calculated_resolution = 0;
			if (full_quality == TRUE) {
				updated_region->Set(preview_bitmap->Bounds());
				return 1;
			}
			else
				return 0;
		}
		else {
			if (full_quality == FALSE)
				last_calculated_resolution = last_calculated_resolution / 2;
			else
				last_calculated_resolution = min_c(1,last_calculated_resolution/2);
		}
	}
	else if (full_quality == TRUE)
		last_calculated_resolution = 1;
	else
		last_calculated_resolution = lowest_available_quality;

	if (last_calculated_resolution > 0) {
		union {
			uint8	bytes[4];
			uint32	word;
		} background;
		// Transparent background.
		background.bytes[0] = 0xFF;
		background.bytes[1] = 0xFF;
		background.bytes[2] = 0xFF;
		background.bytes[3] = 0x00;


		// This function assumes that the both bitmaps are of same size.
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 target_bpr = preview_bitmap->BytesPerRow()/4;
		uint32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow()/4;

		int32 width = (int32)min_c(preview_bitmap->Bounds().Width()+1,copy_of_the_preview_bitmap->Bounds().Width()+1);
		int32 height = (int32)min_c(preview_bitmap->Bounds().Height()+1,copy_of_the_preview_bitmap->Bounds().Height()+1);

		// We have to copy translations so that we do translation for all pixels
		// with the same values
		int32 x_translation_local = ((int32)settings->x_translation)/last_calculated_resolution*last_calculated_resolution;
		int32 y_translation_local = ((int32)settings->y_translation)/last_calculated_resolution*last_calculated_resolution;
		if (selection->IsEmpty()) {
			// First clear the target bitmap.
			BRegion to_be_cleared;
			to_be_cleared.Set(uncleared_rect);
//			uncleared_rect.PrintToStream();
			uncleared_rect.left = max_c(0,x_translation_local);
			uncleared_rect.top = max_c(0,y_translation_local);
			uncleared_rect.right = min_c(width-1,width-1+x_translation_local);
			uncleared_rect.bottom = min_c(height-1,height-1+y_translation_local);
			to_be_cleared.Exclude(uncleared_rect);
			for (int32 i=0;i<to_be_cleared.CountRects();i++) {
				BRect rect = to_be_cleared.RectAt(i);
				int32 left = (int32)(floor(rect.left / last_calculated_resolution) * last_calculated_resolution);
				int32 top = (int32)(floor(rect.top / last_calculated_resolution) * last_calculated_resolution);
				int32 right = (int32)(ceil(rect.right / last_calculated_resolution) * last_calculated_resolution);
				int32 bottom = (int32)(ceil(rect.bottom / last_calculated_resolution) * last_calculated_resolution);
		//		printf("left %d, top %d, res: %d\n",left,top,last_calculated_resolution);
				for (int32 y=top;y<=bottom;y += last_calculated_resolution) {
					for (int32 x=left;x<=right;x += last_calculated_resolution) {
						*(target_bits + x + y*target_bpr) = background.word;
					}
				}
			}
			// Copy only the points that are within the intersection of
			// original_bitmap->Bounds() and original_bitmap->Bounds().OffsetBy(x_translation,y_translation)
			int32 target_x_offset = max_c(0,x_translation_local);
			int32 target_y_offset = max_c(0,y_translation_local);
			int32 source_x_offset = max_c(0,-x_translation_local);
			int32 source_y_offset = max_c(0,-y_translation_local);

			for (int32 y = 0;y<height - abs(y_translation_local);y += last_calculated_resolution) {
				for (int32 x = 0;x<width - abs(x_translation_local);x += last_calculated_resolution) {
					*(target_bits + x+target_x_offset + (y + target_y_offset)*target_bpr)
					 = *(source_bits + x+source_x_offset + (y + source_y_offset)*source_bpr);
				}
			}

			updated_region->Set(uncleared_rect);
			updated_region->Include(&to_be_cleared);
//			updated_region->Set(preview_bitmap->Bounds());
		}
		else {
			selection->Translate(x_translation_local-previous_x_translation,y_translation_local-previous_y_translation);

			// First reset the picture
			BRegion to_be_cleared;
			to_be_cleared.Set(uncleared_rect);
			uncleared_rect = selection->GetBoundingRect();
			uncleared_rect.OffsetBy(x_translation_local,y_translation_local);
			uncleared_rect.InsetBy(-1,-1);
			uncleared_rect = uncleared_rect & preview_bitmap->Bounds();
			for (int32 i=0;i<to_be_cleared.CountRects();i++) {
				BRect rect = to_be_cleared.RectAt(i);
				int32 left = (int32)(floor(rect.left / last_calculated_resolution) * last_calculated_resolution);
				int32 top = (int32)(floor(rect.top / last_calculated_resolution) * last_calculated_resolution);
				int32 right = (int32)(floor(rect.right / last_calculated_resolution) * last_calculated_resolution);
				int32 bottom = (int32)(floor(rect.bottom / last_calculated_resolution) * last_calculated_resolution);

				for (int32 y=top;y<=bottom;y += last_calculated_resolution) {
					for (int32 x=left;x<=right;x += last_calculated_resolution) {
						*(target_bits + x + y*target_bpr) = *(source_bits + x + y*source_bpr);
					}
				}
			}

			// Then do the selection
			BRect selection_bounds = selection->GetBoundingRect();
			selection_bounds.left = ceil(selection_bounds.left / last_calculated_resolution)*last_calculated_resolution;
			selection_bounds.top = ceil(selection_bounds.top / last_calculated_resolution)*last_calculated_resolution;
			int32 left = (int32)selection_bounds.left;
			int32 right = (int32)selection_bounds.right;
			int32 top = (int32)selection_bounds.top;
			int32 bottom = (int32)selection_bounds.bottom;
			for (int32 y=top;y<=bottom;y += last_calculated_resolution) {
				for (int32 x=left;x<=right;x += last_calculated_resolution) {
					if (selection->ContainsPoint(x,y))
						*(target_bits + x + y*target_bpr) = background.word;
				}
			}



			selection_bounds = selection->GetBoundingRect();
			selection_bounds.OffsetBy(settings->x_translation,settings->y_translation);
			selection_bounds = selection_bounds & preview_bitmap->Bounds();
			selection_bounds.left = ceil(selection_bounds.left / last_calculated_resolution)*last_calculated_resolution;
			selection_bounds.top = ceil(selection_bounds.top / last_calculated_resolution)*last_calculated_resolution;
			left = (int32)selection_bounds.left;
			right = (int32)selection_bounds.right;
			top = (int32)selection_bounds.top;
			bottom = (int32)selection_bounds.bottom;
			for (int32 y=top;y<=bottom;y += last_calculated_resolution) {
				for (int32 x=left;x<=right;x += last_calculated_resolution) {
					int32 new_x = (int32)(x - settings->x_translation);
					int32 new_y = (int32)(y - settings->y_translation);
					if (selection->ContainsPoint(new_x,new_y))
						*(target_bits + x + y*target_bpr) = *(source_bits + new_x + new_y*source_bpr);
				}
			}

			updated_region->Set(uncleared_rect);
			updated_region->Include(&to_be_cleared);
		}
		previous_x_translation = x_translation_local;
		previous_y_translation = y_translation_local;
	}
	return last_calculated_resolution;
}

ManipulatorSettings* TranslationManipulator::ReturnSettings()
{
	return new TranslationManipulatorSettings(settings);
}


void TranslationManipulator::Reset(Selection *sel)
{
	sel->Translate(int32(-settings->x_translation), int32(-settings->y_translation));
	settings->x_translation = 0;
	settings->y_translation = 0;
	previous_x_translation = 0;
	previous_y_translation = 0;

	if (preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);

		uncleared_rect = preview_bitmap->Bounds();
	}
}


void TranslationManipulator::SetPreviewBitmap(BBitmap *bm)
{
	delete copy_of_the_preview_bitmap;
	preview_bitmap = bm;
	if (preview_bitmap != NULL) {
		copy_of_the_preview_bitmap = DuplicateBitmap(preview_bitmap);
	}
	else {
		copy_of_the_preview_bitmap = NULL;
	}

	if (preview_bitmap != NULL) {
		double speed = GetSystemClockSpeed() / 1000;

		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width()+1) * (bounds.Height() + 1);
		lowest_available_quality = 1;

		while ((2*num_pixels/lowest_available_quality/lowest_available_quality) > speed)
			lowest_available_quality *= 2;

		highest_available_quality = max_c(lowest_available_quality/2,1);

		uncleared_rect = preview_bitmap->Bounds();
	}
	else {
		lowest_available_quality = 1;
		highest_available_quality = 1;
	}
	last_calculated_resolution = lowest_available_quality;
}



BView*
TranslationManipulator::MakeConfigurationView(float width, float height,
	const BMessenger& target)
{
	config_view = new TranslationManipulatorView(BRect(0, 0, width - 1, height - 1)
		,this, target);
	config_view->SetValues(settings->x_translation, settings->y_translation);
	return config_view;
}


void
TranslationManipulator::SetValues(float x, float y)
{
	settings->x_translation = x;
	settings->y_translation = y;
}


const char*
TranslationManipulator::ReturnName()
{
	return StringServer::ReturnString(TRANSLATE_STRING);
}


const char*
TranslationManipulator::ReturnHelpString()
{
	return StringServer::ReturnString(DO_TRANSLATE_HELP_STRING);
}


// #pragma mark -- TranslationManipulatorView


TranslationManipulatorView::TranslationManipulatorView(BRect rect,
		TranslationManipulator* manipulator, const BMessenger& target)
	: BView(rect, "configuration_view", B_FOLLOW_ALL, B_WILL_DRAW)
	, fTarget(target)
	, fManipulator(manipulator)
{
	fXControl = new NumberControl("X:", "9999˚",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);
	AddChild(fXControl);
	fXControl->MoveTo(rect.LeftTop());
	fXControl->ResizeToPreferred();
	float divider = fXControl->Divider();
	fXControl->ResizeBy(fXControl->TextView()->StringWidth("99999")
		- fXControl->TextView()->Bounds().Width(), 0);
	fXControl->TextView()->ResizeBy(fXControl->TextView()->StringWidth("99999")
		- fXControl->TextView()->Bounds().Width(), 0);

	fXControl->SetDivider(divider);

	BRect frame_rect = fXControl->Frame();
	frame_rect.OffsetBy(frame_rect.Width() + 10,0);

	fYControl = new NumberControl("Y:", "9999˚",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);
	AddChild(fYControl);
	fYControl->MoveTo(frame_rect.LeftTop());
	fYControl->ResizeToPreferred();
	divider = fYControl->Divider();
	fYControl->ResizeBy(fYControl->TextView()->StringWidth("99999")
		- fYControl->TextView()->Bounds().Width(), 0);
	fYControl->TextView()->ResizeBy(fYControl->TextView()->StringWidth("99999")
		- fYControl->TextView()->Bounds().Width(), 0);
	fYControl->SetDivider(divider);

	ResizeTo(min_c(fYControl->Frame().right, rect.Width()),
		min_c(fYControl->Frame().Height(), rect.Height()));
}


void
TranslationManipulatorView::AttachedToWindow()
{
	fXControl->SetTarget(this);
	fYControl->SetTarget(this);

	if (BView* parent = Parent()) {
		SetLowColor(parent->LowColor());
		SetViewColor(parent->ViewColor());
	}

	fXControl->MakeFocus(true);
}


void
TranslationManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_MANIPULATOR_ADJUSTING_FINISHED: {
			if (fManipulator)
				fManipulator->SetValues(fXControl->Value(), fYControl->Value());

			if (fTarget.IsValid())
				fTarget.SendMessage(message);
		}	break;

		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


void
TranslationManipulatorView::SetValues(float x, float y)
{
	fXControl->SetValue(int32(x));
	fYControl->SetValue(int32(y));
}


void
TranslationManipulatorView::SetTarget(const BMessenger& target)
{
	fTarget = target;
}
