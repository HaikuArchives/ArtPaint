/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Ralf Schuelke <teammaui@web.de>
 *
 */
#include <Bitmap.h>
#include <Resources.h>
#include <Roster.h>
#include <View.h>
#include <IconUtils.h>

#include "ToolImages.h"
#include "Tools.h"
#include "PaintApplication.h"

// here initialize the static pointer variable
ToolImages* ToolImages::first_tool = NULL;

ToolImages::ToolImages(int32 type, BPicture *picture_off_big, BPicture *picture_on_big, BPicture *picture_off_small, BPicture *picture_on_small)
{
	tool_type = type;
	off_big = picture_off_big;
	on_big = picture_on_big;
	off_small = picture_off_small;
	on_small = picture_on_small;

	if (ToolImages::first_tool == NULL) {
		next_tool = NULL;
	}
	else {
		next_tool = ToolImages::first_tool;
	}

	ToolImages::first_tool = this;
}


BPicture* ToolImages::getPicture(int32 type,int32 picture_size,int32 picture_number)
{
	ToolImages *list_pointer = first_tool;

	// the NULL test must be before the other test in the while loop
	while ((list_pointer != NULL) && (list_pointer->tool_type != type)) {
		list_pointer = list_pointer->next_tool;
	}

	// we should always return a copy of the picture so that it can be deleted
	// by the caller (i.e. we should change this pointer returning)
	if (list_pointer != NULL) {
		// if we found a predefined image
		if (picture_number == 0) {
			if (picture_size == BIG_TOOL_PICTURE_SIZE) {
				return new BPicture(*(list_pointer->off_big));
			}
			else {
				return new BPicture(*(list_pointer->off_small));
			}
		}
		else {
			if (picture_size == BIG_TOOL_PICTURE_SIZE) {
				return new BPicture(*(list_pointer->on_big));
			}
			else {
				return new BPicture(*(list_pointer->on_small));
			}
		}

	}
	else {
		// if no picture is found we will create and return a default picture
		BView *a_view = new BView(BRect(0,0,picture_size-1,picture_size-1),"picture creation view",B_FOLLOW_NONE,0);
		BBitmap *a_bitmap = new BBitmap(BRect(0,0,picture_size-1,picture_size-1),B_COLOR_8_BIT,TRUE);

		BPicture *pic;

		a_bitmap->AddChild(a_view);

		a_bitmap->Lock();

		// the picture will be two lines going from corner to corner
		a_view->BeginPicture(new BPicture);
		a_view->SetHighColor(255,255,255);
		a_view->FillRect(a_view->Bounds());
		a_view->SetHighColor(0,0,0);
		a_view->StrokeLine(BPoint(0,0),BPoint(picture_size-1,picture_size-1));
		a_view->StrokeLine(BPoint(0,picture_size-1),BPoint(picture_size-1,0));
		if (picture_number != 0) {
			a_view->SetDrawingMode(B_OP_INVERT);
			a_view->FillRect(a_view->Bounds());
		}
		pic = a_view->EndPicture();

		a_bitmap->Unlock();

		return pic;
	}
}


void ToolImages::createToolImages()
{
	// Here we read from this app's resources the images that are available there.
	// They are stored as 32-bit BGRA-data.

	// First let's find the app's file and create a BResources object pointing to it.
	app_info info;
	BFile app_file;
	BResources *app_resources;

	be_app->GetAppInfo(&info);
	app_file.SetTo(&(info.ref),B_READ_ONLY);
	app_resources = new BResources(&app_file);

	// For each set of tool-images found in the resource-file we must create
	// a new ToolImages-object. Before creating it we must convert the bitmaps to
	// BPictures. When we create a ToolImages-object it will add itself to the
	// tool-images list. We will use a dedicated function to read the images and
	// create new ToolImages.

	ReadVICNImage(app_resources, 1000, FREE_LINE_TOOL);
	ReadVICNImage(app_resources, 1010, STRAIGHT_LINE_TOOL);
	ReadVICNImage(app_resources, 1020, RECTANGLE_TOOL);
	ReadVICNImage(app_resources, 1030, ELLIPSE_TOOL);
	ReadVICNImage(app_resources, 1040, BRUSH_TOOL);
	ReadVICNImage(app_resources, 1050, HAIRY_BRUSH_TOOL);
	ReadVICNImage(app_resources, 1060, AIR_BRUSH_TOOL);
	ReadVICNImage(app_resources, 1070, BLUR_TOOL);
	ReadVICNImage(app_resources, 1080, FILL_TOOL);
	ReadVICNImage(app_resources, 1090, TEXT_TOOL);
	ReadVICNImage(app_resources, 1100, TRANSPARENCY_TOOL);
	ReadVICNImage(app_resources, 1110, ERASER_TOOL);
	ReadVICNImage(app_resources, 1120, SELECTOR_TOOL);
	ReadVICNImage(app_resources, 1130, COLOR_SELECTOR_TOOL);

	// Finally destroy the pointer to resources.
	delete app_resources;
}

void ToolImages::ReadVICNImage(BResources *res, int32 id, int32 tool_type)
{
	size_t length ;
	const void *data;
	data = res->LoadResource(B_VECTOR_ICON_TYPE, id, &length);
	BBitmap *big_map = NULL, *big_map_on = NULL, *big_map_off = NULL, *small_map = NULL;
	BPicture *big_on = NULL, *big_off = NULL, *small_on = NULL, *small_off = NULL;

	//VIcon to bitmap
	big_map = new BBitmap(BRect(0, 0, BIG_TOOL_PICTURE_SIZE -10, BIG_TOOL_PICTURE_SIZE -10), 0, B_RGBA32); //-3
	BIconUtils::GetVectorIcon((uint8*)data, length, big_map);

	//selectet bitmap
	big_map_on = new BBitmap(BRect(0, 0, BIG_TOOL_PICTURE_SIZE -1, BIG_TOOL_PICTURE_SIZE -1), B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
	BView *on_view = new BView(BRect(0,0,BIG_TOOL_PICTURE_SIZE -1,BIG_TOOL_PICTURE_SIZE -1),"view",B_FOLLOW_NONE,B_WILL_DRAW);
	big_map_on->Lock();
	big_map_on->AddChild(on_view);
	on_view->SetHighColor(0,0,0);
	on_view->FillRect(on_view->Bounds());
	on_view->SetHighColor(255,255,255);
	on_view->FillRect(BRect(1,1,BIG_TOOL_PICTURE_SIZE -2,BIG_TOOL_PICTURE_SIZE -2));
	on_view->SetDrawingMode(B_OP_ALPHA);
	on_view->DrawBitmap(big_map,BPoint(5,5));
	on_view->Sync();
	big_map_on->Unlock();

	//non selectect bitmap
	big_map_off = new BBitmap(BRect(0, 0, BIG_TOOL_PICTURE_SIZE -1, BIG_TOOL_PICTURE_SIZE -1), B_BITMAP_ACCEPTS_VIEWS, B_RGBA32);
	BView *off_view = new BView(BRect(0,0,BIG_TOOL_PICTURE_SIZE -1,BIG_TOOL_PICTURE_SIZE -1),"view",B_FOLLOW_NONE,B_WILL_DRAW);
	big_map_off->Lock();
	big_map_off->AddChild(off_view);
	off_view->SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	off_view->FillRect(off_view->Bounds());
	off_view->SetDrawingMode(B_OP_ALPHA);
	off_view->DrawBitmap(big_map, BPoint(5,5));
	off_view->Sync();
	big_map_off->Unlock();

	if (big_map && big_map_off || big_map_on) {
		big_on = bitmap_to_picture(big_map_on);
		big_off = bitmap_to_picture(big_map_off);
	}

	small_map = new BBitmap(BRect(0, 0, SMALL_TOOL_PICTURE_SIZE -1, SMALL_TOOL_PICTURE_SIZE -1), 0, B_RGBA32);
	BIconUtils::GetVectorIcon((uint8*)data, length, small_map);
	if (small_map) {
		small_on = bitmap_to_picture(small_map);
		small_off = bitmap_to_picture(small_map);
	}

	delete big_map;
	delete big_map_on;
	delete big_map_off;
	delete small_map;

	if (big_on && big_off && small_on && small_off) {
		new ToolImages(tool_type,big_off,big_on,small_off,small_on);
	} else {
		delete big_on;
		delete big_off;
		delete small_on;
		delete small_off;
	}
}

BPicture* bitmap_to_picture(BBitmap *bitmap)
{
	// Create an off-screen bitmap for the purpose of recording picture.
	BBitmap *off_screen = new BBitmap(BRect(0,0,0,0),B_RGB_32_BIT,TRUE);

	// Create a view that records the picture.
	BView *off_screen_view = new BView(BRect(0,0,0,0),"picture maker view",B_FOLLOW_NONE,B_WILL_DRAW);
	off_screen_view->SetDrawingMode(B_OP_ALPHA);
	off_screen->AddChild(off_screen_view);

	BPicture *picture = new BPicture();
	off_screen->Lock();
	off_screen_view->BeginPicture(picture);
	off_screen_view->DrawBitmap(bitmap);
	picture = off_screen_view->EndPicture();
	off_screen->Unlock();
	delete off_screen;

	return picture;
}
