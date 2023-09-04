/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "ResourceServer.h"


#include <Application.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <Picture.h>
#include <Roster.h>
#include <TranslationUtils.h>
#include <View.h>


#include <new>


BLocker ResourceServer::fLocker;
ResourceServer* ResourceServer::fResourceServer = NULL;


ResourceServer::ResourceServer()
	: fResource(NULL)
{
}


ResourceServer::~ResourceServer()
{
	delete fResource;
	fResourceServer = NULL;
}


ResourceServer*
ResourceServer::Instance()
{
	return Instantiate();
}


status_t
ResourceServer::GetBitmap(int32 id, BBitmap** bitmap)
{
	float width;
	float height;
	type_code type;

	if (_GetResourceInfo(id, &type, &width, &height) == B_OK)
		return GetBitmap(type, id, width, height, bitmap);

	return B_ERROR;
}


status_t
ResourceServer::GetBitmap(type_code type, int32 id, float width, float height, BBitmap** bitmap)
{
	if (!_Init())
		return B_ERROR;

	size_t length;
	const void* data = fResource->LoadResource(type, id, &length);
	if (data) {
		if (type == B_VECTOR_ICON_TYPE) {
			*bitmap = new (std::nothrow) BBitmap(BRect(0.0, 0.0, width - 1.0,
				height - 1.0), B_RGBA32);
			if (BIconUtils::GetVectorIcon((uint8*)data, length, *bitmap) != B_OK) {
				delete *bitmap;
				*bitmap = NULL;
			}
		} else if (type == B_COLOR_8_BIT_TYPE) {
			*bitmap = new (std::nothrow) BBitmap(BRect(0.0, 0.0, width - 1.0,
				height - 1.0), B_CMAP8);
			if (*bitmap)
				(*bitmap)->ImportBits(data, int32(length), width, 0, B_CMAP8);
		} else {
			BMemoryIO memio(data, length);
			*bitmap = BTranslationUtils::GetBitmap(&memio);
		}
		if (*bitmap)
			return B_OK;
	}

	return B_ERROR;
}


status_t
ResourceServer::GetPicture(int32 id, BPicture* picture)
{
	status_t status = B_ERROR;

	if (!_Init() && !picture)
		return status;

	float width;
	float height;
	type_code type;

	if (_GetResourceInfo(id, &type, &width, &height) == B_OK) {
		BBitmap* bitmap = NULL;
		if (GetBitmap(type, id, width, height, &bitmap) == B_OK)
			status = _FillPicture(bitmap, picture);
		delete bitmap;
	}

	return status;
}


status_t
ResourceServer::GetPicture(int32 id, BPicture** picture)
{
	float width;
	float height;
	type_code type;

	if (_GetResourceInfo(id, &type, &width, &height) == B_OK)
		return GetPicture(type, id, width, height, picture);

	return B_ERROR;
}


status_t
ResourceServer::GetPicture(type_code type, int32 id, float width, float height, BPicture** picture)
{
	status_t status = B_ERROR;

	if (!_Init())
		return status;

	BBitmap* bitmap = NULL;
	if (GetBitmap(type, id, width, height, &bitmap) == B_OK) {
		*picture = new (std::nothrow) BPicture();
		if ((status = _FillPicture(bitmap, *picture)) != B_OK) {
			delete *picture;
			*picture = NULL;
		}
		delete bitmap;
	}

	return status;
}


ResourceServer*
ResourceServer::Instantiate()
{
	if (fResourceServer == NULL) {
		BAutolock _(&fLocker);
		if (fResourceServer == NULL)
			fResourceServer = new (std::nothrow) ResourceServer();
	}
	return fResourceServer;
}


void
ResourceServer::DestroyServer()
{
	if (fResourceServer) {
		delete fResourceServer;
		fResourceServer = NULL;
	}
}


bool
ResourceServer::_Init()
{
	if (fResource)
		return true;

	app_info appInfo;
	be_app->GetAppInfo(&appInfo);

	BFile app(&(appInfo.ref), B_READ_ONLY);
	if (app.InitCheck() == B_OK)
		fResource = new (std::nothrow) BResources(&app);

	return (fResource != NULL);
}


status_t
ResourceServer::_FillPicture(const BBitmap* bitmap, BPicture* picture)
{
	status_t status = B_ERROR;

	if (!bitmap || !picture)
		return status;

	BBitmap* offscreenBitmap = new (std::nothrow) BBitmap(bitmap->Bounds(), B_RGBA32, true);
	if (offscreenBitmap) {
		BView* view = new (std::nothrow) BView(bitmap->Bounds(), "offscreen",
			B_FOLLOW_NONE, B_WILL_DRAW);
		if (view) {
			status = B_OK;
			offscreenBitmap->Lock();
			offscreenBitmap->AddChild(view);
			view->SetDrawingMode(B_OP_ALPHA);
			view->BeginPicture(picture);
			view->DrawBitmap(bitmap);
			picture = view->EndPicture();
			offscreenBitmap->Unlock();
		}
		delete offscreenBitmap;
	}

	return status;
}


status_t
ResourceServer::_GetResourceInfo(int32 id, type_code* type, float* width, float* height)
{
	status_t status = B_OK;

	switch (id) {
		case LEFT_ARROW:
		{
			case RIGHT_ARROW:
			case LEFT_ARROW_PUSHED:
			case RIGHT_ARROW_PUSHED:
				*width = 9.0;
				*height = 13.0;
				*type = B_COLOR_8_BIT_TYPE;
			break;
		}
		case OK_BUTTON:
		{
			case CANCEL_BUTTON:
			case OK_BUTTON_PUSHED:
			case CANCEL_BUTTON_PUSHED:
				*width = 16.0;
				*height = 16.0;
				*type = B_COLOR_8_BIT_TYPE;
			break;
		}
		case POP_UP_LIST:
		{
			case POP_UP_LIST_PUSHED:
				*width = 10.0;
				*height = 20.0;
				*type = B_COLOR_8_BIT_TYPE;
			break;
		}
		default:
		{
			*width = 0.0;
			*height = 0.0;
			status = B_ERROR;
			*type = B_ANY_TYPE;
			break;
		}
	}

	return status;
}
