/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#ifndef RESOURCESERVER_H
#define RESOURCESERVER_H

#include "PaintApplication.h"


#include <Locker.h>
#include <Resources.h>


class BBitmap;
class BPicture;


enum {
	LEFT_ARROW = 30032,
	LEFT_ARROW_PUSHED = 30031,
	RIGHT_ARROW = 30042,
	RIGHT_ARROW_PUSHED = 30041,
	OK_BUTTON = 1103,
	OK_BUTTON_PUSHED = 1104,
	CANCEL_BUTTON = 2103,
	CANCEL_BUTTON_PUSHED = 2104,
	POP_UP_LIST = 30002,
	POP_UP_LIST_PUSHED = 30001
};


class ResourceServer {
	friend class PaintApplication;

public:
	static	ResourceServer*		Instance();

			status_t			GetBitmap(int32 id, BBitmap** bitmap);
			status_t			GetBitmap(type_code type, int32 id,
									float width, float height, BBitmap** bitmap);

			status_t			GetPicture(int32 id, BPicture* picture);
			status_t			GetPicture(int32 id, BPicture** picture);
			status_t			GetPicture(type_code type, int32 id, float width,
									float height, BPicture** picture);

private:
								ResourceServer();
								ResourceServer(const ResourceServer& server);
								~ResourceServer();

			bool				_Init();
			status_t			_FillPicture(const BBitmap* bitmap,
									BPicture* picture);
			status_t			_GetResourceInfo(int32 id, type_code* type,
									float* width, float* height);

	static	ResourceServer*		Instantiate();
	static	void				DestroyServer();

private:
			BResources*			fResource;

	static	BLocker				fLocker;
	static	ResourceServer*		fResourceServer;
};

#endif	// RESOURCESERVER_H
