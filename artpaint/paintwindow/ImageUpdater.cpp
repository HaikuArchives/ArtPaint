/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2010, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "ImageUpdater.h"
#include "ImageView.h"


#include <Window.h>


ImageUpdater::ImageUpdater(ImageView* imageView, bigtime_t updateInterval)
	: fImageView(imageView)
	, fUpdatedRect(BRect())
	, fUpdateInterval(updateInterval)
	, fContinueUpdating(true)
	, fBenaphoreCount(0)
	, fBenaphoreMutex(create_sem(0, "BenaphoreMutex"))
{
	if (fUpdateInterval > 0) {
		fUpdaterThreadId = spawn_thread(_ThreadFunc, "ImageUpdaterThread",
			B_DISPLAY_PRIORITY, this);
		resume_thread(fUpdaterThreadId);
	}
}


ImageUpdater::~ImageUpdater()
{
	if (fUpdateInterval > 0) {
		EnterCriticalSection();

		int32 value;
		fContinueUpdating = false;
		wait_for_thread(fUpdaterThreadId, &value);

		LeaveCriticalSection();
	}
	delete_sem(fBenaphoreMutex);
}


void
ImageUpdater::AddRect(const BRect& rect)
{
	EnterCriticalSection();

	if (fUpdatedRect.IsValid())
		fUpdatedRect = fUpdatedRect | rect;
	else
		fUpdatedRect = rect;

	LeaveCriticalSection();
}


void
ImageUpdater::ForceUpdate()
{
	EnterCriticalSection();

	if (fUpdatedRect.IsValid()) {
		if (fImageView->LockLooper()) {
			fUpdatedRect.left = floor(fUpdatedRect.left);
			fUpdatedRect.top = floor(fUpdatedRect.top);
			fUpdatedRect.right = ceil(fUpdatedRect.right);
			fUpdatedRect.bottom = ceil(fUpdatedRect.bottom);

			if (fUpdatedRect.IsValid()) {
				fImageView->UpdateImage(fUpdatedRect);
				fImageView->Sync();
			}
			fImageView->UnlockLooper();
		}
		fUpdatedRect = BRect();
	}

	LeaveCriticalSection();
}


int32
ImageUpdater::_Update()
{
	while (fContinueUpdating) {
		ForceUpdate();
		snooze(fUpdateInterval);
	}
	return B_OK;
}


int32
ImageUpdater::_ThreadFunc(void* data)
{
	ImageUpdater* imageUpdater = static_cast<ImageUpdater*> (data);
	return imageUpdater->_Update();
}


bool
ImageUpdater::EnterCriticalSection()
{
	if (atomic_add(&fBenaphoreCount, 1) >= 1) {
		if (acquire_sem(fBenaphoreMutex) != B_OK)
			return false;
	}
	return true;
}


void
ImageUpdater::LeaveCriticalSection()
{
	if (atomic_add(&fBenaphoreCount, -1) > 1)
		release_sem(fBenaphoreMutex);
}
