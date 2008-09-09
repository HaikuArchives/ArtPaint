/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef PREVIEW_VIEW_H
#define	PREVIEW_VIEW_H

#include <View.h>

class PreviewView : public BView {
		BBitmap	*preview_bitmap;
		BPoint	bitmap_offset;
public:
		PreviewView(BRect frame,int32 preview_width,int32 preview_height);
virtual	~PreviewView();

void	Draw(BRect);

void		RedisplayBitmap();
BBitmap*	ReturnBitmap();
};


#endif
