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

public:
		BackgroundView(BRect frame);
void	Draw(BRect);
void	FrameResized(float,float);
void	MouseDown(BPoint);
};

#endif
