/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef _THRESHOLD_VIEW_H
#define	_THRESHOLD_VIEW_H

#include <Bitmap.h>
#include <Control.h>
#include <MenuField.h>
#include <Rect.h>

class ThresholdView : public BControl {
public:
		ThresholdView(BMessage*);

void	AttachedToWindow();

void	Draw(BRect);

status_t	Invoke(BMessage *msg=NULL);

void	MessageReceived(BMessage*);

void	MouseDown(BPoint);
void	MouseMoved(BPoint,uint32,const BMessage*);
void	MouseUp(BPoint);


void	SetBitmap(BBitmap *bitmap);

private:
BBitmap		*histogramBitmap;
int32		histogram[256];
BRect		histogramRect;

BBitmap		*analyzedBitmap;

int32		threshold;
int32		mode;

bool		isTracking;

BMenuField	*modeMenu;


void		CalculateHistogram();
};

enum {
	HISTOGRAM_MODE_INTENSITY = 'Hmin',
	HISTOGRAM_MODE_RED = 'Hmre',
	HISTOGRAM_MODE_GREEN = 'Hmgr',
	HISTOGRAM_MODE_BLUE = 'Hmbl'
};


#endif	// _THRESHOLD_VIEW_H
