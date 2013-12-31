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

#include <Locker.h>
#include <Resources.h>


class BBitmap;
class BPicture;


enum {
	POP_UP_LIST = 1,
	POP_UP_LIST_PUSHED = 2,
	LEFT_ARROW = 3,
	LEFT_ARROW_PUSHED = 4,
	RIGHT_ARROW = 5,
	RIGHT_ARROW_PUSHED = 6,
	OK_BUTTON = 7,
	OK_BUTTON_PUSHED = 8,
	CANCEL_BUTTON = 9,
	CANCEL_BUTTON_PUSHED = 10
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
