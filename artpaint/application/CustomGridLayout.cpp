/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "CustomGridLayout.h"

#include <GridLayout.h>
#include <Message.h>
#include <Point.h>
#include <View.h>


CustomGridLayout::CustomGridLayout(BMessage* message)
	: BGridLayout(message)
{
}


CustomGridLayout::CustomGridLayout(float horizontal, float vertical)
	: BGridLayout(horizontal, vertical)
{
}


CustomGridLayout::~CustomGridLayout()
{
}


BPoint
CustomGridLayout::GetViewPosition(BView* view)
{
	int32 index = IndexOfView(view);
	BLayoutItem* layoutItem = ((BLayout*)this)->ItemAt(index);
	Dimensions itemDim;
	GetItemDimensions(layoutItem, &itemDim);

	return BPoint(itemDim.x, itemDim.y);
}


void
CustomGridLayout::SwapViews(BView* view1, BView* view2)
{
	BPoint pos1 = GetViewPosition(view1);
	BPoint pos2 = GetViewPosition(view2);

	RemoveView(view1);
	RemoveView(view2);
	AddView(view1, (int32)pos2.x, (int32)pos2.y);
	AddView(view2, (int32)pos1.x, (int32)pos1.y);
}
