/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MAGNIFICATION_VIEW_H
#define	MAGNIFICATION_VIEW_H

#include <MessageFilter.h>
#include <StringView.h>
#include <View.h>

class MagnificationView : public BView {
		BButton	*plus_button;
		BButton	*minus_button;

		BStringView	*string_view;
		BBox		*string_box;
public:
		MagnificationView(BRect);

void	AttachedToWindow();
void	Draw(BRect);

void	SetMagnificationLevel(float);

void	SetTarget(BMessenger&);
};


class MagStringView : public BStringView {
public:
		MagStringView(BRect,char*,char*);

void	MouseMoved(BPoint,uint32,const BMessage*);
};

filter_result filter1(BMessage*,BHandler**,BMessageFilter*);
#endif
