/* 

	Filename:	ImageUpdater.h
	Contents:	ImageUpdater-class declarations.	
	Author:		Heikki Suhonen
	
*/


#ifndef IMAGE_UPDATER_H
#define	IMAGE_UPDATER_H

#include <OS.h>
#include <Region.h>

class ImageView;

class ImageUpdater {
		int32		benaphore_count;
		sem_id		benaphore_mutex;


		ImageView	*view;
		double		interval;
		BRect		updated_rect;
		bool		continue_updating;
		thread_id	updater_thread;
	
		bool	EnterCS();
		bool	ExitCS();

static	int32	updater_entry(void*);
		int32	updater_function();	

public:
		ImageUpdater(ImageView*,double update_interval=50000.0);
		~ImageUpdater();
	
void	AddRect(BRect);
void	ForceUpdate();
};


#endif