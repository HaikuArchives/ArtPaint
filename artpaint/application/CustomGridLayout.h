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
	virtual	~CustomGridLayout();

	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);
	virtual	BLayoutItem*		AddView(BView* child, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);
	virtual	bool				AddItem(BLayoutItem* item, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);
	virtual status_t			ItemArchived(BMessage* into,
									BLayoutItem* item, int32 index) const;
	virtual status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);

	virtual	bool				HasMultiColumnItems();
	virtual	bool				HasMultiRowItems();

	virtual	int32				InternalCountColumns();
	virtual	int32				InternalCountRows();
	virtual	void				GetColumnRowConstraints(
									orientation orientation,
									int32 index,
									ColumnRowConstraints* constraints);
	virtual	void				GetItemDimensions(BLayoutItem* item,
									Dimensions* dimensions);
public:
			BPoint 				GetViewPosition(BView* view);
			void				SwapViews(BView* view1, BView* view2);
};


#endif

