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


BLayoutItem*
CustomGridLayout::AddView(BView* child)
{
	return BGridLayout::AddView(child);
}


BLayoutItem*
CustomGridLayout::AddView(int32 index, BView* child)
{
	return BGridLayout::AddView(index, child);
}


BLayoutItem*
CustomGridLayout::AddView(BView* child, int32 column, int32 row,
		int32 columnCount, int32 rowCount)
{
	return BGridLayout::AddView(child, column, row, columnCount, rowCount);
}


bool
CustomGridLayout::AddItem(BLayoutItem* item)
{
	return BGridLayout::AddItem(item);
}


bool
CustomGridLayout::AddItem(int32 index, BLayoutItem* item)
{
	return BGridLayout::AddItem(index, item);
}


bool
CustomGridLayout::AddItem(BLayoutItem* item, int32 column,
	int32 row, int32 columnCount, int32 rowCount)
{
	return BGridLayout::AddItem(item, column, row, columnCount, rowCount);
}


status_t
CustomGridLayout::Archive(BMessage* into, bool deep) const
{
	return BGridLayout::Archive(into, deep);
}


status_t
CustomGridLayout::Perform(perform_code d, void* arg)
{
	return BGridLayout::Perform(d, arg);
}


status_t
CustomGridLayout::AllArchived(BMessage* into) const
{
	return BGridLayout::AllArchived(into);
}


status_t
CustomGridLayout::AllUnarchived(const BMessage* from)
{
	return BGridLayout::AllUnarchived(from);
}


status_t
CustomGridLayout::ItemArchived(BMessage* into,
	BLayoutItem* item, int32 index) const
{
	return BGridLayout::ItemArchived(into, item, index);
}

status_t
CustomGridLayout::ItemUnarchived(const BMessage* from,
	BLayoutItem* item, int32 index)
{
	return BGridLayout::ItemUnarchived(from, item, index);
}


bool
CustomGridLayout::ItemAdded(BLayoutItem* item, int32 atIndex)
{
	return BGridLayout::ItemAdded(item, atIndex);
}


void
CustomGridLayout::ItemRemoved(BLayoutItem* item, int32 fromIndex)
{
	BGridLayout::ItemRemoved(item, fromIndex);
}


bool
CustomGridLayout::HasMultiColumnItems()
{
	return BGridLayout::HasMultiColumnItems();
}


bool
CustomGridLayout::HasMultiRowItems()
{
	return BGridLayout::HasMultiRowItems();
}


int32
CustomGridLayout::InternalCountColumns()
{
	return BGridLayout::InternalCountColumns();
}


int32
CustomGridLayout::InternalCountRows()
{
	return BGridLayout::InternalCountRows();
}


void
CustomGridLayout::GetColumnRowConstraints(
	orientation orientation, int32 index,
	ColumnRowConstraints* constraints)
{
	BGridLayout::GetColumnRowConstraints(orientation, index, constraints);
}


void
CustomGridLayout::GetItemDimensions(BLayoutItem* item,
	Dimensions* dimensions)
{
	BGridLayout::GetItemDimensions(item, dimensions);
}
