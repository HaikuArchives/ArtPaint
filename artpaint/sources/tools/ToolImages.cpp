/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Resources.h>
#include <Roster.h>
#include <View.h>

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
	ReadImages(app_resources,10,NO_TOOL);
	ReadImages(app_resources,20,FREE_LINE_TOOL);
	ReadImages(app_resources,30,SELECTOR_TOOL);
	ReadImages(app_resources,40,FILL_TOOL);
	ReadImages(app_resources,50,RECTANGLE_TOOL);
	ReadImages(app_resources,60,ELLIPSE_TOOL);
	ReadImages(app_resources,70,STRAIGHT_LINE_TOOL);
	ReadImages(app_resources,80,BRUSH_TOOL);
	ReadImages(app_resources,90,BLUR_TOOL);
	ReadImages(app_resources,100,COLOR_SELECTOR_TOOL);
	ReadImages(app_resources,110,TRANSPARENCY_TOOL);
	ReadImages(app_resources,120,AIR_BRUSH_TOOL);
	ReadImages(app_resources,130,SELECTOR_TOOL);
	ReadImages(app_resources,140,HAIRY_BRUSH_TOOL);
	ReadImages(app_resources,150,ERASER_TOOL);
	ReadImages(app_resources,160,TEXT_TOOL);

	// Finally destroy the pointer to resources.
	delete app_resources;
}


status_t ToolImages::ReadImages(BResources *res,int32 base, int32 tool_type)
{
	// If some of the images is not found we should return NULL and also delete any created pictures.
	BBitmap *big_map = new BBitmap(BRect(0,0,31,31),B_COLOR_8_BIT);
	BBitmap *small_map = new BBitmap(BRect(0,0,15,15),B_COLOR_8_BIT);
	BPicture *big_on,*big_off,*small_on,*small_off;
	big_on = NULL;
	big_off = NULL;
	small_on = NULL;
	small_off = NULL;

	status_t error = B_OK;
	base = base*100;
	char buffer[1024];

	// Read the BIG_ON-image
	if ( (res->ReadResource(B_COLOR_8_BIT_TYPE,base+1,buffer,0,1024)) == B_OK ) {
		big_map->SetBits(buffer,1024,0,B_COLOR_8_BIT);
		big_on = bitmap_to_picture(big_map);
	}
	else {
		error = B_ERROR;
	}
	// Read the BIG_OFF-image
	if ( (res->ReadResource(B_COLOR_8_BIT_TYPE,base+2,buffer,0,1024)) == B_OK ) {
		big_map->SetBits(buffer,1024,0,B_COLOR_8_BIT);
		big_off = bitmap_to_picture(big_map);
	}
	else {
		error = B_ERROR;
	}
	// Read the SMALL_ON-image
	if ( (res->ReadResource(B_COLOR_8_BIT_TYPE,base+3,buffer,0,256)) == B_OK ) {
		small_map->SetBits(buffer,256,0,B_COLOR_8_BIT);
		small_on = bitmap_to_picture(small_map);
	}
	else {
		error = B_ERROR;
	}
	// Read the SMALL_OFF-image
	if ( (res->ReadResource(B_COLOR_8_BIT_TYPE,base+4,buffer,0,256)) == B_OK ) {
		small_map->SetBits(buffer,256,0,B_COLOR_8_BIT);
		small_off = bitmap_to_picture(small_map);
	}
	else {
		error = B_ERROR;
	}

	delete big_map;
	delete small_map;

	if (error != B_OK) {
		delete big_on;
		delete big_off;
		delete small_on;
		delete small_off;
	}
	else {
		new ToolImages(tool_type,big_off,big_on,small_off,small_on);
	}

	return error;
}
BPicture* bitmap_to_picture(BBitmap *bitmap)
{
	// Create an off-screen bitmap for the purpose of recording picture.
	BBitmap *off_screen = new BBitmap(BRect(0,0,0,0),B_RGB_32_BIT,TRUE);

	// Create a view that records the picture.
	BView *off_screen_view = new BView(BRect(0,0,0,0),"picture maker view",B_FOLLOW_NONE,B_WILL_DRAW);
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
