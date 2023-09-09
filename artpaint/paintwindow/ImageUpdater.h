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
#ifndef IMAGE_UPDATER_H
#define	IMAGE_UPDATER_H

#include <OS.h>
#include <Region.h>


class ImageView;


class ImageUpdater {
public:
							ImageUpdater(ImageView* imageView,
								bigtime_t updateInterval = 50000);
							~ImageUpdater();

			void			ForceUpdate();
			void			AddRect(const BRect& rect);

private:
			int32			_Update();
	static	int32			_ThreadFunc(void* data);

			bool			EnterCriticalSection();
			void			LeaveCriticalSection();

private:
			ImageView*		fImageView;
			BRect			fUpdatedRect;
			bigtime_t		fUpdateInterval;
			bool			fContinueUpdating;

			int32			fBenaphoreCount;
			sem_id			fBenaphoreMutex;
			thread_id		fUpdaterThreadId;
};


#endif // IMAGE_UPDATER_H
