/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	BACKGROUND_VIEW_H
#define	BACKGROUND_VIEW_H

#include <ScrollView.h>

// this view will be used to draw border around ImageView
// when necessary
// also used to help in resizing ImageView
class BackgroundView : public BScrollView {

// this will be used to create another thread for rotation or
// resizing
//static	int32	thread_entry(void *data);
//
//void	resizeImageBounds();

public:
		BackgroundView(BRect frame);
void	Draw(BRect);
void	FrameResized(float,float);
void	MouseDown(BPoint);
};



// these constants are used when starting a thread to rotate or
// resize the canvas
//#define	HS_ROTATE_CANVAS	'RoCv'
//#define	HS_RESIZE_CANVAS	'RsCv'


#endif
