/*

	Filename:	ImageUpdater.cpp
	Contents:	ImageUpdater-class definitions
	Author:		Heikki Suhonen

*/

#include <stdio.h>
#include <Window.h>

#include "ImageUpdater.h"
#include "ImageView.h"

ImageUpdater::ImageUpdater(ImageView *v,double update_interval)
{
	view = v;
	interval = update_interval;
	continue_updating = TRUE;

	benaphore_count = 0;
	benaphore_mutex = create_sem(0,"benaphore_mutex");

	if (interval > 0) {
		updater_thread = spawn_thread(updater_entry,"image_updater_thread",B_DISPLAY_PRIORITY,this);
		resume_thread(updater_thread);
	}
}


ImageUpdater::~ImageUpdater()
{
	if (interval > 0) {
		EnterCS();
		suspend_thread(updater_thread);
		continue_updating = FALSE;
		int32 return_value;
		snooze(1000);	// This is a precaution. See BeBook threads-chapter for more info.
		wait_for_thread(updater_thread,&return_value);
		ExitCS();
	}
	delete_sem(benaphore_mutex);
}



void ImageUpdater::AddRect(BRect rect)
{
	EnterCS();
	if (updated_rect.IsValid() == FALSE)
		updated_rect = rect;
	else
		updated_rect = updated_rect | rect;
	ExitCS();
}

void ImageUpdater::ForceUpdate()
{
	EnterCS();

	if (updated_rect.IsValid() == true) {
		if (view->LockLooper() == true) {
			updated_rect.left = floor(updated_rect.left);
			updated_rect.top = floor(updated_rect.top);
			updated_rect.right = ceil(updated_rect.right);
			updated_rect.bottom = ceil(updated_rect.bottom);

			if (updated_rect.IsValid() == true) {
				view->UpdateImage(updated_rect);
				view->Sync();
			}
			view->UnlockLooper();
		}
		updated_rect = BRect();
	}
	ExitCS();
}



int32 ImageUpdater::updater_entry(void *data)
{
	ImageUpdater *this_pointer = (ImageUpdater*)data;

	return this_pointer->updater_function();
}


int32 ImageUpdater::updater_function()
{
	while (continue_updating) {
		ForceUpdate();
		snooze((bigtime_t)interval);
	}

	return B_NO_ERROR;
}

bool ImageUpdater::EnterCS()
{
	int32 previous = atomic_add(&benaphore_count, 1);
	if (previous >= 1)
		if (acquire_sem(benaphore_mutex) != B_NO_ERROR)
			return FALSE;

	return TRUE;
}


bool ImageUpdater::ExitCS()
{
	int32 previous = atomic_add(&benaphore_count, -1);
	if (previous > 1)  {
		release_sem(benaphore_mutex);
	}
	return TRUE;
}
