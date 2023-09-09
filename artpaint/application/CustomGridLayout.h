/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef CUSTOM_GRID_LAYOUT_H
#define CUSTOM_GRID_LAYOUT_H

#include <GridLayout.h>
#include <LayoutItem.h>
#include <Message.h>
#include <Point.h>
#include <View.h>


class CustomGridLayout : public BGridLayout {
public:
				CustomGridLayout(BMessage* message);
				CustomGridLayout(float horizontal, float vertical);

public:
	BPoint 		GetViewPosition(BView* view);
	void		SwapViews(BView* view1, BView* view2);
};


#endif // CUSTOM_GRID_LAYOUT_H
