/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "CropManipulator.h"

#include "Controls.h"
#include "MessageConstants.h"
#include "StringServer.h"


#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <StatusBar.h>
#include <Window.h>


#include <new>


CropManipulator::CropManipulator(BBitmap *bm)
	:	WindowGUIManipulator()
{
	settings = new CropManipulatorSettings();
	config_view = NULL;

	min_x = 0;
	max_x = 0;
	min_y = 0;
	max_y = 0;

	if (bm != NULL) {
		BRect rect = bm->Bounds();
		settings->left = min_x = rect.left;
		settings->right = max_x = rect.right;
		settings->top = min_y = rect.top;
		settings->bottom = max_y = rect.bottom;
	}
}



CropManipulator::~CropManipulator()
{
	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}

	delete settings;
}


void CropManipulator::MouseDown(BPoint point,uint32,BView*,bool first)
{
	previous_left = settings->left;
	previous_right = settings->right;
	previous_top = settings->top;
	previous_bottom = settings->bottom;

	point.x = floor(point.x);
	point.y = floor(point.y);

	if (first == TRUE) {
		// Here we select which grabbles to move
		move_left = move_right = move_top = move_bottom = FALSE;
		if (fabs(point.x - settings->left) < 50)
			if (fabs(point.x-settings->left)<fabs(point.x-(settings->left+(settings->right-settings->left)/2)))
				move_left = TRUE;
		if (fabs(point.x - settings->right) < 50)
			if (fabs(point.x-settings->right)<fabs(point.x-(settings->left+(settings->right-settings->left)/2)))
				move_right = TRUE;
		if ((move_left == TRUE) && (move_right == TRUE)) {
			if (fabs(point.x - settings->left) > fabs(point.x - settings->right))
				move_left = FALSE;
			else
				move_right = FALSE;
		}

		if (fabs(point.y - settings->top) < 50)
			if (fabs(point.y-settings->top)<fabs(point.y-(settings->top+(settings->bottom-settings->top)/2)))
				move_top = TRUE;
		if (fabs(point.y - settings->bottom) < 50)
			if (fabs(point.y-settings->bottom)<fabs(point.y-(settings->top+(settings->bottom-settings->top)/2)))
				move_bottom = TRUE;
		if ((move_top == TRUE) && (move_bottom == TRUE)) {
			if (fabs(point.y - settings->top) > fabs(point.y - settings->bottom))
				move_top = FALSE;
			else
				move_bottom = FALSE;
		}


		if ((move_left == FALSE) && (move_top == FALSE) && (move_right == FALSE) && (move_bottom == FALSE))
			move_all = TRUE;
		else
			move_all = FALSE;


	}
	else {
		if (move_left == TRUE)
			settings->left = min_c(point.x,settings->right);
		if (move_right == TRUE)
			settings->right = max_c(settings->left,point.x);

		if (move_top == TRUE)
			settings->top = min_c(point.y,settings->bottom);
		if (move_bottom == TRUE)
			settings->bottom = max_c(settings->top,point.y);


		if (move_all == TRUE) {
			float width = settings->right - settings->left;
			float height = settings->bottom - settings->top;

			float new_left,new_top,new_right,new_bottom;

			new_left = point.x-width/2;
			new_right = new_left + width;
			new_top = point.y-height/2;
			new_bottom = new_top + height;

			settings->left = new_left;
			settings->top = new_top;
			settings->right = new_right;
			settings->bottom = new_bottom;

		}
	}

	if ((previous_left != settings->left) || (previous_right != settings->right) ||
		(previous_top != settings->top) || (previous_bottom != settings->bottom))
		if (config_view != NULL)
			config_view->SetValues(settings->left,settings->right,settings->top,settings->bottom);
}


BRegion CropManipulator::Draw(BView *view,float mag_scale)
{
	int32 DRAGGER_SIZE = 5;
	// Draw all the data that needs to be drawn
	BRect bounds = BRect(mag_scale*settings->left,mag_scale*settings->top,mag_scale*(settings->right+1)-1,mag_scale*(settings->bottom+1)-1);
	bounds.left = floor(bounds.left);
	bounds.top = floor(bounds.top);
	bounds.right = ceil(bounds.right);
	bounds.bottom = ceil(bounds.bottom);

	bool draw_draggers = FALSE;
	BRect dragger_rect = BRect(0,0,DRAGGER_SIZE-1,DRAGGER_SIZE-1);
	float f_bottom = bounds.bottom;
	float f_top = bounds.top;
	float f_left = bounds.left;
	float f_right = bounds.right;

	if ( (f_bottom-f_top> 3*DRAGGER_SIZE+10) && (f_right-f_left > 3*DRAGGER_SIZE+10) ) {
		draw_draggers = TRUE;
	}
	view->StrokeRect(bounds,B_MIXED_COLORS);
	if (draw_draggers == TRUE) {
		dragger_rect.OffsetTo(bounds.LeftTop());
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.LeftTop()+BPoint((f_right-f_left)/2-DRAGGER_SIZE/2,0));
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.LeftTop()+BPoint(f_right-f_left-DRAGGER_SIZE+1,0));
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.LeftTop()+BPoint(0,(f_bottom-f_top)/2-DRAGGER_SIZE/2));
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.RightTop()+BPoint(-DRAGGER_SIZE+1,(f_bottom-f_top)/2-DRAGGER_SIZE/2));
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.LeftBottom()+BPoint(0,-DRAGGER_SIZE+1));
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.LeftBottom()+BPoint((f_right-f_left)/2-DRAGGER_SIZE/2,-DRAGGER_SIZE+1));
		view->FillRect(dragger_rect,B_MIXED_COLORS);

		dragger_rect.OffsetTo(bounds.LeftBottom()+BPoint(f_right-f_left-DRAGGER_SIZE+1,-DRAGGER_SIZE+1));
		view->FillRect(dragger_rect,B_MIXED_COLORS);
	}

	BRegion updated_region;

	bounds.InsetBy(-1,-1);
	updated_region.Set(bounds);
	bounds.InsetBy(DRAGGER_SIZE+2,DRAGGER_SIZE+2);
	updated_region.Exclude(bounds);

	return updated_region;
}



BBitmap* CropManipulator::ManipulateBitmap(ManipulatorSettings *set,
	BBitmap *original, Selection*, BStatusBar *status_bar)
{
	CropManipulatorSettings* new_settings =
		dynamic_cast<CropManipulatorSettings*> (set);

	if (new_settings == NULL)
		return NULL;

	float left = new_settings->left;
	float right = new_settings->right;
	float top = new_settings->top;
	float bottom = new_settings->bottom;

	if ( ((right-left) == original->Bounds().Width()) && ((bottom-top) == original->Bounds().Height()))
		return NULL;

	// Create a new bitmap here and copy the appropriate part from original.
	// Return the new bitmap.
	BBitmap *new_bitmap;
	new_bitmap = new BBitmap(BRect(0,0,right-left,bottom-top),B_RGB_32_BIT);
	if (new_bitmap->IsValid() == FALSE)
		throw std::bad_alloc();

	uint32 *orig_bits = (uint32*)original->Bits();
	uint32 *new_bits = (uint32*)new_bitmap->Bits();
	int32 original_bpr = original->BytesPerRow()/4;
	float height = bottom-top+1;
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

	for (int32 y=(int32)top;y<=bottom;y++) {
		for (int32 x=(int32)left;x<=right;x++) {
			if ((x>=0) && (y>=0) && (x<=original->Bounds().right) && (y<=original->Bounds().bottom))
				*new_bits++ = *(orig_bits + x +y*original_bpr);
			else
				*new_bits++ = background.word;
		}
		if ((y%10 == 0) && (status_bar != NULL)) {
			progress_message.ReplaceFloat("delta",(100.0)/height*10.0);
			status_bar->Window()->PostMessage(&progress_message,status_bar);
		}
	}

	return new_bitmap;
}

int32 CropManipulator::PreviewBitmap(Selection*,bool,BRegion*)
{
	if ((previous_left == settings->left) && (previous_right == settings->right) &&
		(previous_top == settings->top) && (previous_bottom == settings->bottom))
		return DRAW_NOTHING;
	else
		return DRAW_ONLY_GUI;
}

void CropManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (bm != NULL) {
		BRect rect = bm->Bounds();
		settings->left = min_x = rect.left;
		settings->right = max_x = rect.right;
		settings->top = min_y = rect.top;
		settings->bottom = max_y = rect.bottom;
	}
	else {
		settings->left = min_x = 0;
		settings->right = max_x = 0;
		settings->top = min_y = 0;
		settings->bottom = max_y = 0;
	}

	if (config_view != NULL) {
		config_view->SetValues(settings->left,settings->right,settings->top,settings->bottom);
	}
}

void CropManipulator::SetValues(float x1, float x2, float y1, float y2)
{
	previous_left = settings->left;
	previous_right = settings->right;
	previous_top = settings->top;
	previous_bottom = settings->bottom;

	settings->left = x1;
	settings->right = x2;
	settings->top = y1;
	settings->bottom = y2;

	settings->left = min_c(settings->left,settings->right);
	settings->right = max_c(settings->left,settings->right);

	settings->top = min_c(settings->top,settings->bottom);
	settings->bottom = max_c(settings->top,settings->bottom);

	if (config_view != NULL) {
		config_view->SetValues(settings->left,settings->right,settings->top,settings->bottom);
	}
}


ManipulatorSettings*
CropManipulator::ReturnSettings()
{
	return new CropManipulatorSettings(settings);
}


BView*
CropManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new CropManipulatorView(this, target);
	config_view->SetValues(settings->left, settings->right, settings->top,
		settings->bottom);
	return config_view;
}


const char*
CropManipulator::ReturnName()
{
	return StringServer::ReturnString(CROP_STRING);
}


const char*
CropManipulator::ReturnHelpString()
{
	return StringServer::ReturnString(DO_CROP_HELP_STRING);
}


// #pragma mark -- CropManipulatorView


CropManipulatorView::CropManipulatorView(CropManipulator* manipulator,
		const BMessenger& target)
	: WindowGUIManipulatorView()
	, fTarget(target)
	, fManipulator(manipulator)
{
	fTopCrop = new NumberControl(StringServer::ReturnString(TOP_STRING), "",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);
	fLeftCrop = new NumberControl(StringServer::ReturnString(LEFT_STRING), "",
		new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5, true);
	fRightCrop = new NumberControl(StringServer::ReturnString(RIGHT_STRING),
		"", new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5);
	fBottomCrop = new NumberControl(StringServer::ReturnString(BOTTOM_STRING),
		"", new BMessage(HS_MANIPULATOR_ADJUSTING_FINISHED), 5);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fTopCrop)
			.AddGlue()
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fLeftCrop)
			.AddGlue()
			.AddStrut(80.0)
			.AddGlue()
			.Add(fRightCrop)
			.AddGlue()
		.End()
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fBottomCrop)
			.AddGlue()
		.End()
	);

	fTopCrop->SetValue(0);
	fLeftCrop->SetValue(0);
	fRightCrop->SetValue(0);
	fBottomCrop->SetValue(0);
}


void
CropManipulatorView::AttachedToWindow()
{
	fTopCrop->SetTarget(this);
	fTopCrop->MakeFocus(true);

	fLeftCrop->SetTarget(this);
	fRightCrop->SetTarget(this);
	fBottomCrop->SetTarget(this);

	WindowGUIManipulatorView::AttachedToWindow();
}


void
CropManipulatorView::SetValues(float x1, float x2, float y1, float y2)
{
	BWindow* window = Window();
	if (window && window->Lock()) {
		left = x1;
		right = x2;
		top = y1;
		bottom = y2;

		if (left != fLeftCrop->Value())
			fLeftCrop->SetValue(int32(left));

		if (right != fRightCrop->Value())
			fRightCrop->SetValue(int32(right));

		if (top != fTopCrop->Value())
			fTopCrop->SetValue(int32(top));

		if (bottom != fBottomCrop->Value())
			fBottomCrop->SetValue(int32(bottom));

		window->Unlock();
	}
}


void
CropManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case HS_MANIPULATOR_ADJUSTING_FINISHED: {
			fManipulator->SetValues(fLeftCrop->Value(),
				fRightCrop->Value(), fTopCrop->Value(),
				fBottomCrop->Value());
			fTarget.SendMessage(message);
		}	break;

		default: {
			WindowGUIManipulatorView::MessageReceived(message);
		}	break;
	}
}
