/*

	Filename:	SymbolImageServer.cpp
	Contents:	SymbolImageServer-class definitions.
	Author:	Heikki Suhonen

*/

#include <Application.h>
#include <Bitmap.h>
#include <Resources.h>
#include <Roster.h>
#include <stdio.h>
#include <View.h>

#include "SymbolImageServer.h"

BResources* SymbolImageServer::app_resource = NULL;

BPicture* SymbolImageServer::ReturnSymbolAsPicture(symbol_types type,int32 &width, int32 &height)
{
	int32 id;
	get_size_and_id(type,width,height,id);

	return CreatePicture(id,width,height);
}


BBitmap* SymbolImageServer::ReturnSymbolAsBitmap(symbol_types type,int32 &width,int32 &height)
{
	int32 id;
	get_size_and_id(type,width,height,id);

	return CreateBitmap(id,width,height);
}


void SymbolImageServer::get_size_and_id(symbol_types type,int32 &width,int32 &height,int32 &id)
{
	switch (type) {
		case LEFT_ARROW:
			id = 30032;
			width = 9;
			height = 13;
			break;
		case LEFT_ARROW_PUSHED:
			id = 30031;
			width = 9;
			height = 13;
			break;

		case RIGHT_ARROW:
			id = 30042;
			width = 9;
			height = 13;
			break;
		case RIGHT_ARROW_PUSHED:
			id = 30041;
			width = 9;
			height = 13;
			break;

		case OK_BUTTON:
			id = 1103;
			width = 16;
			height = 16;
			break;
		case OK_BUTTON_PUSHED:
			id = 1104;
			width = 16;
			height = 16;
			break;

		case CANCEL_BUTTON:
			id = 2103;
			width = 16;
			height = 16;
			break;

		case CANCEL_BUTTON_PUSHED:
			id = 2104;
			width = 16;
			height = 16;
			break;

		case POP_UP_LIST:
			id = 30002;
			width = 10;
			height = 20;
			break;

		case POP_UP_LIST_PUSHED:
			id = 30001;
			width = 10;
			height = 20;
			break;

		default:
			id = 0;
			width = 0;
			height = 0;
			break;
	}
}



BPicture* SymbolImageServer::CreatePicture(int32 id,int32 width,int32 height)
{
	if (app_resource == NULL)
		initialize_resource();

	BBitmap *symbol_map = new BBitmap(BRect(0,0,1,1),B_RGB_32_BIT,TRUE);
	BView *symbol_view = new BView(BRect(0,0,1,1),"symbol_view",B_FOLLOW_NONE,B_WILL_DRAW);
	symbol_map->AddChild(symbol_view);

	BBitmap *resource_map = new BBitmap(BRect(0,0,width-1,height-1),B_CMAP8);

	// Read the data from the resources. Row at a time. Add padding to full word alignment
	int32 row_bytes = resource_map->BytesPerRow();
	for (int i=0;i<height;i++)
		app_resource->ReadResource(B_COLOR_8_BIT_TYPE,id,(uchar*)resource_map->Bits()+i*row_bytes,i*width,width);

	BPicture *picture = new BPicture();
	symbol_map->Lock();
	symbol_view->BeginPicture(picture);
	symbol_view->DrawBitmap(resource_map);
	picture = symbol_view->EndPicture();
	symbol_view->RemoveSelf();
	delete symbol_view;
	symbol_map->Unlock();

	delete symbol_map;
	delete resource_map;

	return picture;
}


BBitmap* SymbolImageServer::CreateBitmap(int32 id,int32 width,int32 height)
{
	if (app_resource == NULL)
		initialize_resource();

	BBitmap *resource_map = new BBitmap(BRect(0,0,width-1,height-1),B_CMAP8);
	// Read the data from the resources. Row at a time. Add padding to full word alignment
	int32 row_bytes = resource_map->BytesPerRow();
	for (int i=0;i<height;i++)
		app_resource->ReadResource(B_COLOR_8_BIT_TYPE,id,(uchar*)resource_map->Bits()+i*row_bytes,i*width,width);

	return resource_map;
}



void SymbolImageServer::initialize_resource()
{
	app_info info;
	BFile app_file;

	be_app->GetAppInfo(&info);
	app_file.SetTo(&(info.ref),B_READ_ONLY);
	app_resource = new BResources(&app_file);
}
