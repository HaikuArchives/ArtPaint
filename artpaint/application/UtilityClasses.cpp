/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "UtilityClasses.h"


#include <Bitmap.h>
#include <Screen.h>


BitmapView::BitmapView(BBitmap* bitmap, BRect frame)
	:
	BView(frame, "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fBitmap(bitmap)
{
}


BitmapView::BitmapView(BBitmap* bitmap, BPoint leftTop)
	:
	BView(BRect(leftTop, leftTop), "bitmap view", B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
	fBitmap(bitmap)
{
	if (fBitmap)
		ResizeTo(fBitmap->Bounds().Width(), fBitmap->Bounds().Height());
}


BitmapView::~BitmapView()
{
	delete fBitmap;
}


void
BitmapView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
BitmapView::Draw(BRect updateRect)
{
	if (fBitmap)
		DrawBitmap(fBitmap, updateRect, updateRect);
}


BBitmap*
BitmapView::Bitmap() const
{
	return fBitmap;
}


void
BitmapView::SetBitmap(BBitmap* bitmap)
{
	delete fBitmap;
	fBitmap = bitmap;
}


// primes 1021,2311
PointContainer::PointContainer()
	: hash_table_size(2311)
{
	hash_table = new BPoint*[hash_table_size];
	list_length_table = new int32[hash_table_size];
	for (int32 i = 0; i < hash_table_size; i++) {
		hash_table[i] = NULL;
		list_length_table[i] = 0;
	}
}


PointContainer::~PointContainer()
{
	int32 slots = 0;
	int32 hits = 0;
	for (int32 i = 0; i < hash_table_size; i++) {
		if (list_length_table[i] > 0) {
			slots++;
			hits += list_length_table[i];
		}
	}

	for (int32 i = 0; i < hash_table_size; i++) {
		delete[] hash_table[i];
		hash_table[i] = NULL;
	}
	delete[] hash_table;
	delete[] list_length_table;
}


void
PointContainer::InsertPoint(int32 x, int32 y)
{
	int32 key = hash_value(x, y);

	if (list_length_table[key] == 0) {
		hash_table[key] = new BPoint[1];
		list_length_table[key] = 1;
		hash_table[key][0] = BPoint(x, y);
	} else {
		BPoint* new_array = new BPoint[list_length_table[key] + 1];

		for (int32 i = 0; i < list_length_table[key]; i++)
			new_array[i] = hash_table[key][i];

		delete[] hash_table[key];
		hash_table[key] = new_array;
		hash_table[key][list_length_table[key]] = BPoint(x, y);
		list_length_table[key] += 1;
	}
}


bool
PointContainer::HasPoint(int32 x, int32 y)
{
	int32 key = hash_value(x, y);
	bool has = FALSE;
	BPoint point(x, y);
	for (int32 i = 0; i < list_length_table[key] && has == FALSE; i++) {
		if (hash_table[key][i] == point)
			has = TRUE;
	}

	return has;
}


int32
PointContainer::hash_value(int32 x, int32 y)
{
	int32 value;
	value = (x + (y << 8)) % hash_table_size;
	return value;
}


BRect
FitRectToScreen(BRect source)
{
	// This function returns the source moved and resized so that it fits onto
	// current screen.
	BRect result = source;
	BRect screenFrame = BScreen().Frame();

	if (!screenFrame.Contains(result.RightBottom()))
		result.OffsetTo(BPoint(15.0, 15.0));

	if (!screenFrame.Contains(result.RightBottom())) {
		result.bottom = min_c(screenFrame.bottom, result.bottom);
		result.right = min_c(screenFrame.right, result.right);
	}

	return result;
}


BRect
CenterRectOnScreen(BRect source)
{
	BRect screenFrame = BScreen().Frame();

	BPoint leftTop((screenFrame.Width() + source.Width()) / 2.0,
		(screenFrame.Height() + source.Height()) / 2.0);

	if (leftTop.x < 0.0)
		leftTop.x = 0.0;
	if (leftTop.y < 0.0)
		leftTop.y = 0.0;

	return source.OffsetToCopy(leftTop);
}


BRect
MakeRectFromPoints(const BPoint& point1, const BPoint& point2)
{
	return BRect(min_c(point1.x, point2.x), min_c(point1.y, point2.y),
		max_c(point1.x, point2.x), max_c(point1.y, point2.y));
}


float
SnapToAngle(const float snap_angle, const float src_angle, const float max_angle)
{
	float new_angle = src_angle;

	if (src_angle > max_angle)
		new_angle = -(max_angle - ((int32)src_angle % (int32)max_angle));
	if (src_angle < -max_angle)
		new_angle = max_angle - ((int32)-src_angle % (int32)max_angle);

	float abs_src_angle = fabs(new_angle);
	float abs_snap_angle = fabs(snap_angle);
	float half_angle = abs_snap_angle / 2.;

	float sign = 1;
	if (new_angle < 0)
		sign = -1;

	for (float i = 0; i <= max_angle; i += abs_snap_angle) {
		if (abs_src_angle > i - half_angle && abs_src_angle < i + half_angle) {
			new_angle = i;
			return new_angle * sign;
		}
	}

	return src_angle;
}
