/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <new>
#include <stdio.h>
#include <stdlib.h>

#include "UndoAction.h"
#include "UndoQueue.h"

UndoAction::UndoAction(int32 layer,action_type t,BRect rect)
{
	layer_id = layer;
	type = t;
	if (rect.IsValid() == TRUE)
		bounding_rect = rect;
	else
		type = NO_ACTION;

	tool_script = NULL;
	manipulator_settings = NULL;
	add_on_id = -1;

	undo_bitmaps = NULL;
	undo_rects = NULL;
	undo_bitmap_count = 0;
	queue = NULL;

	size_has_changed = FALSE;
}


UndoAction::UndoAction(int32 layer,int32 merged_layer,BRect rect)
{
	layer_id = layer;
	merged_layer_id = merged_layer;
	type = MERGE_LAYER_ACTION;
	if (rect.IsValid() == TRUE)
		bounding_rect = rect;
	else
		type = NO_ACTION;

	tool_script = NULL;
	manipulator_settings = NULL;
	add_on_id = -1;

	undo_bitmaps = NULL;
	undo_rects = NULL;
	undo_bitmap_count = 0;
	queue = NULL;

	size_has_changed = FALSE;
}

UndoAction::UndoAction(int32 layer,ToolScript *script,BRect rect)
{
	layer_id = layer;
	type = TOOL_ACTION;
	if (rect.IsValid() == TRUE)
		bounding_rect = rect;
	else
		type = NO_ACTION;

	tool_script = NULL;
	manipulator_settings = NULL;
	add_on_id = -1;
	queue = NULL;

	undo_bitmaps = NULL;
	undo_rects = NULL;
	undo_bitmap_count = 0;

	size_has_changed = FALSE;

	if (type == TOOL_ACTION) {
		tool_script = script;
	}
	else {
		delete script;
	}
}



UndoAction::UndoAction(int32 layer,ManipulatorSettings *settings,BRect rect,manipulator_type t,int32 aoid)
{
	layer_id = layer;
	type = MANIPULATOR_ACTION;
	if (rect.IsValid() == TRUE)
		bounding_rect = rect;
	else
		type = NO_ACTION;

	tool_script = NULL;
	manipulator_settings = NULL;
	add_on_id = -1;
	queue = NULL;
	undo_bitmaps = NULL;
	undo_rects = NULL;
	undo_bitmap_count = 0;

	size_has_changed = FALSE;

	if (type == MANIPULATOR_ACTION) {
		manipulator_settings = settings;
		manip_type = t;
		add_on_id = aoid;
	}
}


UndoAction::~UndoAction()
{
	if (undo_bitmaps != NULL) {
		for (int32 i=0;i<undo_bitmap_count;i++) {
			delete undo_bitmaps[i];
			undo_bitmaps[i] = NULL;
		}
		delete[] undo_bitmaps;
		delete[] undo_rects;
	}

	delete tool_script;
	delete manipulator_settings;
}

status_t UndoAction::StoreUndo(BBitmap *bitmap)
{
	bool success = FALSE;
	int32 tries = 0;
	while ((!success) && (tries < 2)) {
		try {
			if (queue == NULL) {
				return B_ERROR;
			}
			if ((type == CHANGE_LAYER_CONTENT_ACTION) || (type == TOOL_ACTION) || (type == CLEAR_LAYER_ACTION) || (type == MERGE_LAYER_ACTION)) {
				BBitmap *spare_bitmap = queue->ReturnLayerSpareBitmap(layer_id,bitmap);
				StoreDifferences(spare_bitmap,bitmap,bounding_rect);
				if (undo_bitmap_count == 0)
					type = NO_ACTION;
			}
			else if (type == ADD_LAYER_ACTION) {
				queue->ChangeLayerSpareBitmap(layer_id,bitmap);
				bounding_rect = bitmap->Bounds();
			}
			else if (type == DELETE_LAYER_ACTION) {
				// Store the last version of layer's bitmap here
				BBitmap *layer_bitmap = queue->ReturnLayerSpareBitmap(layer_id,NULL);
				queue->ChangeLayerSpareBitmap(layer_id,NULL);
				if (layer_bitmap != NULL) {
					undo_bitmaps = new BBitmap*[1];
					undo_rects = new BRect[1];
					undo_bitmaps[0] = layer_bitmap;
					undo_rects[0] = layer_bitmap->Bounds();
					undo_bitmap_count = 1;
				}
				if (undo_bitmap_count == 0)
					type = NO_ACTION;
			}
			else if (type == MANIPULATOR_ACTION) {
				// The size of the bitmap might have changed as might have the offset.
				// If the size has changed we store the whole previous bitmap. If the size
				// has not changed we behave like in the case of CHANGE_LAYER_CONTENT_ACTION.
				// The offset that might also change when size changes should somehow be recorded.
				// This is very important for the proper scripting of tools.
				if (bitmap != NULL) {
					BBitmap *layer_bitmap = queue->ReturnLayerSpareBitmap(layer_id,bitmap);
					if ((layer_bitmap != NULL) && (layer_bitmap->Bounds() != bitmap->Bounds())) {
						// The size has changed, we store the whole previous image.
						undo_bitmaps = new BBitmap*[1];
						undo_rects = new BRect[1];
						undo_bitmaps[0] = layer_bitmap;
						undo_rects[0] = layer_bitmap->Bounds();
						queue->ChangeLayerSpareBitmap(layer_id,bitmap);
						undo_bitmap_count = 1;
						size_has_changed = TRUE;
					}
					else if (layer_bitmap != NULL) {
						// The size has not changed, so store just the differences.
						StoreDifferences(layer_bitmap,bitmap,bounding_rect);
					}
					if (undo_bitmap_count == 0)
						type = NO_ACTION;
				}
			}

			success = TRUE;
			tries++;
		}
		catch (std::bad_alloc e) {
			queue->HandleLowMemorySituation();
		}
	}

	return B_NO_ERROR;
}


BBitmap* UndoAction::ApplyUndo(BBitmap *bitmap,BRect &updated_rect)
{
	try {
		if (queue == NULL)
			return NULL;

		if ((type == CHANGE_LAYER_CONTENT_ACTION) || (type == TOOL_ACTION) || (type == CLEAR_LAYER_ACTION) || (type == MERGE_LAYER_ACTION)) {
			if (undo_bitmaps == NULL)
				return NULL;

			BBitmap *spare_bitmap = queue->ReturnLayerSpareBitmap(layer_id,bitmap);
			updated_rect = RestoreDifference(bitmap,spare_bitmap);

			return bitmap;
		}
		else if (type == ADD_LAYER_ACTION) {
			if (undo_bitmaps == NULL) {
				// this is an undo, so store the layer
				undo_bitmaps = new BBitmap*[1];
				undo_rects = new BRect[1];
				undo_bitmaps[0] = queue->ReturnLayerSpareBitmap(layer_id,NULL);
				undo_rects[0] = undo_bitmaps[0]->Bounds();
				undo_bitmap_count = 1;
				// Then inform parties that the layer is gone
				queue->ChangeLayerSpareBitmap(layer_id,NULL);

				updated_rect = undo_bitmaps[0]->Bounds();

				return NULL;
			}
			else {
				// this is a redo, so transfer the layer's image to ImageView and UndoQueue
				BBitmap *layer_bitmap = undo_bitmaps[0];

				undo_bitmaps[0] = NULL;
				delete[] undo_bitmaps;
				delete[] undo_rects;
				undo_bitmaps = NULL;
				undo_rects = NULL;
				undo_bitmap_count = 0;
				queue->ChangeLayerSpareBitmap(layer_id,layer_bitmap);

				updated_rect = layer_bitmap->Bounds();
				return layer_bitmap;
			}
		}
		else if (type == DELETE_LAYER_ACTION) {
			// This is just the opposite of ADD_LAYER_ACTION
			if (undo_bitmaps == NULL) {
				// This is a redo, so store the layer again
				undo_bitmaps = new BBitmap*[1];
				undo_rects = new BRect[1];
				undo_bitmaps[0] = queue->ReturnLayerSpareBitmap(layer_id,NULL);
				undo_rects[0] = undo_bitmaps[0]->Bounds();
				undo_bitmap_count = 1;

				// Then inform parties that the layer is gone
				queue->ChangeLayerSpareBitmap(layer_id,NULL);

				updated_rect = undo_bitmaps[0]->Bounds();

				return NULL;
			}
			else {
				// This is an undo, so the layer reappears
				BBitmap *layer_bitmap = undo_bitmaps[0];
				undo_bitmaps[0] = NULL;
				delete[] undo_bitmaps;
				delete[] undo_rects;
				undo_bitmaps = NULL;
				undo_rects = NULL;
				undo_bitmap_count = 0;
				queue->ChangeLayerSpareBitmap(layer_id,layer_bitmap);

				updated_rect = layer_bitmap->Bounds();
				return layer_bitmap;
			}
		}
		else if (type == MANIPULATOR_ACTION) {
			// If the manipulator changed the size of the bitmap we return a
			// pointer to that bitmap and also swap the stored bitmap with
			// the queue's current bitmap. If the size did not change we do
			// the same as in the case of CHANGE_BITMAP_CONTENT_ACTION.
			if (size_has_changed) {
				// Just swap the stored bitmap with the one in the queue
				// and return the old one.
				BBitmap *old_bitmap = undo_bitmaps[0];
				undo_bitmaps[0] = queue->ReturnLayerSpareBitmap(layer_id,bitmap);
				undo_rects[0] = undo_bitmaps[0]->Bounds();
				queue->ChangeLayerSpareBitmap(layer_id,old_bitmap);
				updated_rect = old_bitmap->Bounds();
				return old_bitmap;
			}
			else {
				// The size has not changed, just copy the differences like in the
				// case of CHANGE_LAYER_CONTENT_ACTION.
				if (undo_bitmaps == NULL)
					return NULL;

				BBitmap *spare_bitmap = queue->ReturnLayerSpareBitmap(layer_id,bitmap);
				RestoreDifference(bitmap,spare_bitmap);
				updated_rect = bitmap->Bounds();
				return bitmap;
			}
		}

		else if (type == NO_ACTION) {
			updated_rect = BRect(0,0,-1,-1);
			return bitmap;
		}
		return NULL;
	}
	catch (std::bad_alloc e) {
		queue->HandleLowMemorySituation();
		throw e;
	}

	return NULL;
}



void UndoAction::StoreDifferences(BBitmap *old, BBitmap *current, BRect area)
{
	try {
		area.left = floor(area.left);
		area.top = floor(area.top);
		area.right = ceil(area.right);
		area.bottom = ceil(area.bottom);
		area = area & old->Bounds();
		uint32 *bits = (uint32*)current->Bits();
		uint32 *old_bits = (uint32*)old->Bits();
		uint32	bpr = current->BytesPerRow()/4;

		int32 left = (int32)area.left;
		int32 right = (int32)area.right;
		int32 top = (int32)area.top;
		int32 bottom = (int32)area.bottom;
		int32 difference;
		if ((area.Width() < 16) || (area.Height() < 16)) {
			// If the area is small enough we examine it thoroughly to
			// see if there are any differences that need to be stored.
			difference = NO_DIFFERENCE;
			for (int32 y=top;(y<=bottom) && (difference == NO_DIFFERENCE);y++) {
				for (int32 x=left;(x<=right) && (difference == NO_DIFFERENCE);x++) {
	//				different = different || (*(bits + x + y*bpr) == *(old_bits + x + y*bpr) ? FALSE : TRUE);
					if (*(bits + x + y*bpr) != *(old_bits + x + y*bpr))
						difference = GREAT_DIFFERENCE;
				}
			}
		}
		else {
			// If the area is large enough we examine it randomly to see
			// if it is different enough to be stored completely
			difference = SLIGHT_DIFFERENCE;
			int32 width = area.IntegerWidth() + 1;
			int32 height = area.IntegerHeight() + 1;
			int32 left = (int32)area.left;
			int32 top = (int32)area.top;
			float number_of_tests = 10;
			float difference_count = 0;
			for (int32 i=0;i<number_of_tests;i++) {
				int32 dx = (int32)((float)rand() / (float)RAND_MAX * width);
				int32 dy = (int32)((float)rand() / (float)RAND_MAX * height);
				int32 x = left + dx;
				int32 y = top + dy;
				if (*(bits + x + y*bpr) != *(old_bits + x + y*bpr)) {
					difference_count++;
				}
			}
			if ( (difference_count/number_of_tests) >= 0.25)
				difference = GREAT_DIFFERENCE;
		}

		if (difference == GREAT_DIFFERENCE) {
			BBitmap **new_undo_bitmaps = new BBitmap*[undo_bitmap_count + 1];
			BRect *new_undo_rects = new BRect[undo_bitmap_count + 1];

			if (undo_bitmaps != NULL) {
				for (int32 i=0;i<undo_bitmap_count;i++) {
					new_undo_bitmaps[i] = undo_bitmaps[i];
					undo_bitmaps[i] = NULL;
					new_undo_rects[i] = undo_rects[i];
				}
				delete[] undo_bitmaps;
				delete[] undo_rects;
			}
			undo_bitmaps = new_undo_bitmaps;
			undo_rects = new_undo_rects;
			undo_bitmap_count++;

			BRect bitmap_rect = area;
			bitmap_rect.OffsetTo(0,0);
			undo_bitmaps[undo_bitmap_count - 1] = new BBitmap(bitmap_rect,B_RGB32);
			if (undo_bitmaps[undo_bitmap_count-1]->IsValid() == FALSE)
				throw std::bad_alloc();

			undo_rects[undo_bitmap_count - 1] = area;
			uint32 *undo_bits = (uint32*)undo_bitmaps[undo_bitmap_count - 1]->Bits();
			for (int32 y=(int32)area.top;y<=area.bottom;y++) {
				for (int32 x=(int32)area.left;x<=area.right;x++) {
					*undo_bits++ = *(old_bits + x + y*bpr);
					*(old_bits + x + y*bpr) = *(bits + x + y*bpr);
				}
			}
		}
		else if (difference == SLIGHT_DIFFERENCE) {
			// Here we divide the area into four parts and call this function recursively.
			// This may be seen as a quadtree-like approach.
			BRect rect1;
			BRect rect2;
			BRect rect3;
			BRect rect4;
			int32 middle_x = (int32)floor(area.left + (area.right-area.left)/2);
			int32 middle_y = (int32)floor(area.top + (area.bottom - area.top)/2);
			rect1 = BRect(area.left,area.top,middle_x,middle_y);
			rect2 = BRect(middle_x+1,area.top,area.right,middle_y);
			rect3 = BRect(area.left,middle_y+1,middle_x,area.bottom);
			rect4 = BRect(middle_x+1,middle_y+1,area.right,area.bottom);

			StoreDifferences(old,current,rect1);
			StoreDifferences(old,current,rect2);
			StoreDifferences(old,current,rect3);
			StoreDifferences(old,current,rect4);
		}
	}
	catch (std::bad_alloc e) {
		throw e;
	}
}



BRect UndoAction::RestoreDifference(BBitmap *bitmap1,BBitmap *bitmap2)
{
	BRect bitmap_rect;
	BRect bounds = bitmap1->Bounds();
	uint32 *bits1 = (uint32*)bitmap1->Bits();
	uint32	bpr = bitmap1->BytesPerRow()/4;
	uint32 *bits2 = (uint32*)bitmap2->Bits();

	BRect updated_rect(1000000,1000000,-1000000,-1000000);

	for (int32 i=0;i<undo_bitmap_count;i++) {
		uint32 *undo_bits = (uint32*)undo_bitmaps[i]->Bits();
		uint32	spare_value;
		bitmap_rect = undo_rects[i];
		updated_rect.left = min_c(updated_rect.left,bitmap_rect.left);
		updated_rect.right = max_c(updated_rect.right,bitmap_rect.right);
		updated_rect.top = min_c(updated_rect.top,bitmap_rect.top);
		updated_rect.bottom = max_c(updated_rect.bottom,bitmap_rect.bottom);
		if (bounds.Contains(bitmap_rect)) {
			for (int32 y=(int32)bitmap_rect.top;y<=bitmap_rect.bottom;y++) {
				for (int32 x=(int32)bitmap_rect.left;x<=bitmap_rect.right;x++) {
					spare_value = *(bits1 + x + y*bpr);
					*(bits1 + x + y*bpr) = *undo_bits;
					*(bits2 + x +y*bpr) = *undo_bits;
					*undo_bits++ = spare_value;
				}
			}
		}
		else if (bounds.Intersects(bitmap_rect)) {
			for (int32 y=(int32)bitmap_rect.top;y<=bitmap_rect.bottom;y++) {
				for (int32 x=(int32)bitmap_rect.left;x<=bitmap_rect.right;x++) {
					if (bounds.Contains(BPoint(x,y))) {
						spare_value = *(bits1 + x + y*bpr);
						*(bits1 + x + y*bpr) = *undo_bits;
						*(bits2 + x + y*bpr) = *undo_bits;
						*undo_bits = spare_value;
					}
					undo_bits++;
				}
			}
		}
	}

	updated_rect = updated_rect & bounds;
	return updated_rect;
}
