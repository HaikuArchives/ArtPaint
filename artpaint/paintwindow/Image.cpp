/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "Image.h"


#include "BitmapUtilities.h"
#include "ImageView.h"
#include "Layer.h"
#include "PixelOperations.h"
#include "ProjectFileFunctions.h"
#include "UndoEvent.h"
#include "UndoQueue.h"
#include "UtilityClasses.h"
#include "Selection.h"
#include "SettingsServer.h"


#include <Alert.h>
#include <Catalog.h>
#include <Bitmap.h>
#include <ByteOrder.h>
#include <File.h>
#include <Screen.h>
#include <StopWatch.h>


#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Image"


color_entry* Image::color_candidates = NULL;
int32 Image::color_candidate_users = 0;
rgb_color* Image::color_list = new rgb_color[256];


Image::Image(ImageView *view,float width, float height, UndoQueue *q)
{
	image_view = view;
	undo_queue = q;
	rendered_image = NULL;
	next_layer_id = 0;
	current_layer_index = 0;
	thumbnail_image = new BBitmap(BRect(0,0,64,64),B_RGB32);
	layer_list = new BList(10);
	layer_id_list = NULL;

	image_width = width;
	image_height = height;

	full_fixed_alpha = 32768;

	dithered_image = NULL;
	dithered_users = new BList();

	dithered_up_to_date = FALSE;

	system_info info;
	get_system_info(&info);
	number_of_cpus = info.cpu_count;
}


Image::~Image()
{
	for (int32 i=0;i<layer_list->CountItems();i++) {
		layer_id_list[i] = NULL;
	}
	delete[] layer_id_list;

	while (layer_list->CountItems() > 0) {
		Layer *l = (Layer*)layer_list->RemoveItem((int32)0);
		delete l;
	}
	delete layer_list;

	delete dithered_users;
	delete dithered_image;
	delete rendered_image;
	delete thumbnail_image;
}


void
Image::Render(bool bg)
{
	Render(rendered_image->Bounds(), bg);
}

#define	ALPHA	(*s_bits & 0xff)


void
Image::Render(BRect area, bool bg)
{
	dithered_up_to_date = FALSE;
	area = area & rendered_image->Bounds();

	int32 number_of_threads = 1;	// At least one thread

	// Only start multiple threads if the area is big enough
	if ((area.Height() > number_of_cpus) && (area.Height()*area.Width()>2500)) {
		number_of_threads = number_of_cpus;
	}

	thread_id *threads = new thread_id[number_of_threads];

	int32 height = area.IntegerHeight() / number_of_threads + 1;
	for (int32 i=0;i<number_of_threads;i++) {
		if (bg)
			threads[i] = spawn_thread(enter_render,"render_thread",B_NORMAL_PRIORITY,this);
		else
			threads[i] = spawn_thread(enter_render_nobg,"render_thread",B_NORMAL_PRIORITY,this);

		resume_thread(threads[i]);

		BRect rect = area;
		rect.top = min_c(area.bottom, area.top + i*height);
		rect.bottom = min_c(area.bottom,rect.top+height-1);
		send_data(threads[i],0,&rect,sizeof(BRect));
	}

	for (int32 i=0;i<number_of_threads;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}
	if (dithered_image != NULL) {
		for (int32 i=0;i<number_of_threads;i++) {
			threads[i] = spawn_thread(enter_dither,"dither_thread",B_NORMAL_PRIORITY,this);
			resume_thread(threads[i]);

			BRect rect = area;
			rect.top = min_c(area.bottom, area.top + i*height);
			rect.bottom = min_c(area.bottom,rect.top+height-1);

			send_data(threads[i],0,&rect,sizeof(BRect));
		}

		bool is_ready = TRUE;
		for (int32 i=0;i<number_of_threads;i++) {
			int32 return_value;
			wait_for_thread(threads[i],&return_value);
			if (return_value != B_OK)
				is_ready = FALSE;
		}
		if (is_ready == TRUE)
			dithered_up_to_date = TRUE;
		else
			dithered_up_to_date = FALSE;
	}
	delete[] threads;

	// finally call the function that creates the mini-pictures of layers
	// and the rendered_image
	CalculateThumbnails();
}


void
Image::RenderPreview(BRect area, int32 resolution)
{
	dithered_up_to_date = FALSE;
	area.top = floor(area.top/resolution);
	area.top *= resolution;
	area.bottom = ceil(area.bottom/resolution);
	area.bottom *= resolution;

	area.left = floor(area.left/resolution);
	area.left *= resolution;
	area.right = ceil(area.right/resolution);
	area.right *= resolution;

	area = area & rendered_image->Bounds();

	int32 number_of_threads = 1;	// At least one thread

	// Only start multiple threads if the area is big enough
	if ((area.Height() > number_of_cpus) && (area.Height()*area.Width()>2500)) {
		number_of_threads = number_of_cpus;
	}

	thread_id *threads = new thread_id[number_of_threads];

	int32 height = (area.IntegerHeight() + 1) / number_of_threads;
	for (int32 i=0;i<number_of_threads;i++) {
		threads[i] = spawn_thread(enter_render_preview,"render_preview_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);

		BRect rect = area;
		rect.top = min_c(area.bottom, area.top + i*height);
		rect.bottom = min_c(area.bottom,rect.top+height-1);

		rect.top = floor(rect.top/resolution)*resolution;
		rect.bottom = ceil(rect.bottom/resolution)*resolution;

		// This calculation really needs to be fixed to be faster and more accurate
		rect = rect & rendered_image->Bounds();

		send_data(threads[i],resolution,&rect,sizeof(BRect));
	}

	for (int32 i=0;i<number_of_threads;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;
}


void
Image::RenderPreview(BRegion &region, int32 resolution)
{
	// Start a thread for each rectangle in the region
	int32 rect_count = region.CountRects();
	if (rect_count > 0) {
		thread_id *threads = new thread_id[rect_count];

		for (int32 i=0;i<rect_count;i++) {
			threads[i] = spawn_thread(enter_render_preview,
				"render_preview_thread", B_NORMAL_PRIORITY, this);
			resume_thread(threads[i]);
			BRect rect = region.RectAt(i);
			send_data(threads[i],resolution, &rect, sizeof(BRect));
		}

		for (int32 i = 0;i < rect_count;i++) {
			int32 return_value;
			wait_for_thread(threads[i], &return_value);
		}
		delete[] threads;
	}
}


void
Image::MultiplyRenderedImagePixels(int32 blocksize)
{
	if (rendered_image != NULL) {
		dithered_up_to_date = FALSE;
		if (blocksize > 1) {
			int32 height = rendered_image->Bounds().IntegerHeight();
			int32 width = rendered_image->Bounds().IntegerWidth();

			uint32 *bits = (uint32*)rendered_image->Bits();
			uint32 bpr = rendered_image->BytesPerRow()/4;
			// Duplicate the pixels
			for (int32 y=0;y<=height;y += blocksize) {
				for (int32 x=0;x<=width;x += blocksize) {
					int32 x_dimension = min_c(blocksize,width+1-x);
					int32 y_dimension = min_c(blocksize,height+1-y);
					uint32 value = *(bits + x + y*bpr);
					uint32 y_offset = y*bpr;
					for (int32 dy=0;dy<y_dimension;dy++) {
						for (int32 dx=0;dx<x_dimension;dx++) {
							*(bits + x+dx + y_offset) = value;
						}
						y_offset += bpr;
					}
				}
			}
		}
	}
}


bool
Image::SetImageSize()
{
	// Take the minimum dimensions of layers as the new image dimensions.
	float width = 1000000;
	float height = 1000000;
	for (int32 i=0;i<layer_list->CountItems();i++) {
		Layer *layer = (Layer*)layer_list->ItemAt(i);
		width = min_c(layer->Bitmap()->Bounds().Width(), width);
		height = min_c(layer->Bitmap()->Bounds().Height(), height);
	}
	image_width = width+1;
	image_height = height+1;

	if (layer_list->CountItems() >= 1) {
		if (rendered_image == NULL) {
			// Here we create new composite picture.
			rendered_image = new BBitmap(BRect(0, 0,
				image_width - 1, image_height - 1), B_RGBA32);
		}
		else if ((rendered_image->Bounds().Width() != (image_width - 1)) ||
			(rendered_image->Bounds().Height() != (image_height - 1))) {
			// Here we change the bitmap for the composite picture. Also
			// if the dithered picture is required we change that too.
			delete rendered_image;
			rendered_image = new BBitmap(BRect(0, 0,
				image_width - 1, image_height - 1), B_RGBA32);
			if (dithered_image != NULL) {
				delete dithered_image;
				dithered_image = new BBitmap(BRect(0, 0,
					image_width - 1, image_height - 1), B_CMAP8);
			}

			if (rendered_image->IsValid() == FALSE) {
				rendered_image = NULL;
				throw std::bad_alloc();
			}
			if ((dithered_image != NULL) &&
				(dithered_image->IsValid() == FALSE)) {
				dithered_image = NULL;
				throw std::bad_alloc();
			}

			// If the image size changes, clear the selection.
			image_view->GetSelection()->Clear();
			image_view->GetSelection()->ImageSizeChanged(
				rendered_image->Bounds());

			image_view->adjustSize();
			image_view->adjustPosition();
			image_view->adjustScrollBars();
		}
	}

	return FALSE;
}


Layer*
Image::AddLayer(BBitmap *bitmap, Layer *next_layer, bool add_to_front,
	float layer_transparency_coefficient, BRect* offset)
{
	bool create_layer = TRUE;
	if (layer_list->CountItems() != 0) {
		((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(FALSE);
	}
	int32 layer_id = atomic_add(&next_layer_id,1);
	Layer **new_layer_id_list = new Layer*[layer_id+1];
	for (int32 i=0;i<layer_id;i++) {
		new_layer_id_list[i] = layer_id_list[i];
		layer_id_list[i] = NULL;
	}

	delete[] layer_id_list;
	layer_id_list = new_layer_id_list;
	// add a layer to correct position in the list
	Layer *new_layer;
	try {
		new_layer = new Layer(BRect(0,0,image_width-1,image_height-1),
			layer_id,image_view, HS_NORMAL_LAYER, bitmap, offset);
	}
	catch (std::bad_alloc e) {
		delete bitmap;
		throw e;
	}
	delete bitmap;

	if (create_layer == TRUE) {
		if (layer_transparency_coefficient != 1.0)
			new_layer->SetTransparency(layer_transparency_coefficient);

		if (next_layer == NULL) {
			layer_list->AddItem(new_layer);
		}
		else {
			int32 index = layer_list->IndexOf(next_layer);
			if (add_to_front == TRUE)
				index++;

			layer_list->AddItem(new_layer,index);
		}

		layer_id_list[layer_id] = new_layer;

		// make that new item the active layer, and also visible.
		current_layer_index = layer_list->IndexOf(new_layer);
		new_layer->ActivateLayer(TRUE);
		new_layer->SetVisibility(TRUE);
		new_layer->AddToImage(this);

		// if this is the first layer we should create the composite picture
		if ((layer_list->CountItems() == 1) && (rendered_image == NULL) ) {
			rendered_image = new BBitmap(BRect(0,0,image_width-1,image_height-1),B_RGBA32);
			if (rendered_image->IsValid() == FALSE) {
				// If the creation of composite picture fails we should remove the newly added
				// layer and inform the user that we cannot add a layer. The method that called
				// this function will take care of informing the user.
				delete rendered_image;
				rendered_image = NULL;
				layer_id_list[layer_id] = NULL;
				layer_list->RemoveItem(new_layer);
				delete new_layer;
				throw std::bad_alloc();
			}
		}

		Render();

		// Store the undo.
		if (undo_queue != NULL) {
			UndoEvent *new_event = undo_queue->AddUndoEvent(B_TRANSLATE("Add layer"),ReturnThumbnailImage());
			if (new_event != NULL) {
				for (int32 i=0;i<layer_list->CountItems();i++) {
					Layer *layer = (Layer*)layer_list->ItemAt(i);
					UndoAction *new_action;
					if (layer->Id() != layer_id)
						new_action = new UndoAction(layer->Id());
					else {
						new_action = new UndoAction(layer->Id(),ADD_LAYER_ACTION,layer->Bitmap()->Bounds());
					}
					new_event->AddAction(new_action);
					new_action->StoreUndo(layer->Bitmap());
				}
			}
		}
	}

	return new_layer;
}


bool
Image::ChangeActiveLayer(Layer *activated_layer, int32)
{
	if (((Layer*)(layer_list->ItemAt(current_layer_index))) != activated_layer) {
		if (layer_list->IndexOf(activated_layer) >= 0) {
			((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(FALSE);
			current_layer_index = layer_list->IndexOf(activated_layer);
			((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(TRUE);

			return TRUE;
		}
	}

	return FALSE;
}


bool
Image::ChangeActiveLayer(int32 layer_index)
{
	if (current_layer_index != layer_index) {
		if (layer_index >= 0 && layer_index < layer_list->CountItems()) {
			((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(FALSE);
			current_layer_index = layer_index;
			((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(TRUE);

			return TRUE;
		}
	}

	return FALSE;
}


bool
Image::ChangeLayerPosition(Layer *changed_layer, int32,
	int32 positions_moved)
{
	int32 original_position = layer_list->IndexOf(changed_layer);

	if ((original_position >= 0) && (positions_moved != 0)) {
		layer_list->RemoveItem(changed_layer);
		// Make sure the new index is within list's range.
		int32 new_position = min_c(layer_list->CountItems(),max_c(0,original_position+positions_moved));
		layer_list->AddItem(changed_layer,new_position);
		Render();

		((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(FALSE);
		current_layer_index = new_position;
		((Layer*)(layer_list->ItemAt(current_layer_index)))->ActivateLayer(TRUE);

		return TRUE;
	}

	return FALSE;
}


bool
Image::ClearCurrentLayer(rgb_color &c)
{
	Layer *cleared_layer = (Layer*)layer_list->ItemAt(current_layer_index);
	cleared_layer->Clear(c);

	// Store the undo.
	UndoEvent *new_event = undo_queue->AddUndoEvent(B_TRANSLATE("Clear layer"),ReturnThumbnailImage());
	if (new_event != NULL) {
		for (int32 i=0;i<layer_list->CountItems();i++) {
			Layer *layer = (Layer*)layer_list->ItemAt(i);
			UndoAction *new_action;
			if (layer->Id() == cleared_layer->Id()) {
				new_action = new UndoAction(layer->Id(),CLEAR_LAYER_ACTION,layer->Bitmap()->Bounds());
			}
			else
				new_action = new UndoAction(layer->Id());

			new_event->AddAction(new_action);
			new_action->StoreUndo(layer->Bitmap());
		}
	}

	// If the added UndoEvent is empty then destroy it. This is because there was nothing
	// to be cleared.
	if ((new_event != NULL) && (new_event->IsEmpty() == TRUE)) {
		undo_queue->RemoveEvent(new_event);
		delete new_event;
		return FALSE;
	}

	Render();
	return TRUE;
}


bool
Image::ClearLayers(rgb_color &c)
{
	for (int32 i=0;i<layer_list->CountItems();i++) {
		((Layer*)layer_list->ItemAt(i))->Clear(c);
	}

	// Store the undo.
	UndoEvent *new_event = undo_queue->AddUndoEvent(B_TRANSLATE("Clear canvas"),ReturnThumbnailImage());
	if (new_event != NULL) {
		for (int32 i=0;i<layer_list->CountItems();i++) {
			Layer *layer = (Layer*)layer_list->ItemAt(i);
			UndoAction *new_action;
			new_action = new UndoAction(layer->Id(),CLEAR_LAYER_ACTION,layer->Bitmap()->Bounds());
			new_event->AddAction(new_action);
			new_action->StoreUndo(layer->Bitmap());
			thread_id a_thread = spawn_thread(Layer::CreateMiniatureImage,"create mini picture",B_LOW_PRIORITY,layer);
			resume_thread(a_thread);
		}
	}

	// If the added UndoEvent is empty then destroy it. This is because there was nothing
	// to be cleared.
	if ((new_event != NULL) && (new_event->IsEmpty() == TRUE)) {
		undo_queue->RemoveEvent(new_event);
		delete new_event;
		return FALSE;
	}

	Render();
	return TRUE;
}


bool
Image::DuplicateLayer(Layer *duplicated_layer, int32)
{
	if (duplicated_layer != NULL) {
		BBitmap *new_bitmap = new BBitmap(duplicated_layer->Bitmap());
		AddLayer(new_bitmap, duplicated_layer, TRUE,
			duplicated_layer->GetTransparency());

		return TRUE;
	}

	return FALSE;
}


bool
Image::MergeLayers(Layer *merged_layer, int32, bool merge_with_upper)
{
	Layer *target;
	Layer *other;

	if (merge_with_upper) {
		target = merged_layer;
		other = (Layer*)layer_list->ItemAt(
			layer_list->IndexOf(merged_layer) + 1);
	}
	else {
		other = merged_layer;
		target = (Layer*)layer_list->ItemAt(
			layer_list->IndexOf(merged_layer) - 1);
	}

	if ((target != NULL) && (other != NULL)) {
		target->Merge(other);

		// Store the undo.
		UndoEvent *new_event;
		if (merge_with_upper)
			new_event = undo_queue->AddUndoEvent(
				B_TRANSLATE("Merge with layer above"),
				ReturnThumbnailImage());
		else
			new_event = undo_queue->AddUndoEvent(
				B_TRANSLATE("Merge down"),
				ReturnThumbnailImage());

		if (new_event != NULL) {
			for (int32 i=0;i<layer_list->CountItems();i++) {
				Layer *layer = (Layer*)layer_list->ItemAt(i);
				UndoAction *new_action;
				if (layer->Id() == target->Id()) {
					new_action = new UndoAction(layer->Id(),other->Id(),layer->Bitmap()->Bounds());
				}
				else if (layer->Id() == other->Id()) {
					new_action = new UndoAction(layer->Id(),DELETE_LAYER_ACTION,layer->Bitmap()->Bounds());
				}
				else
					new_action = new UndoAction(layer->Id());

				new_event->AddAction(new_action);
				new_action->StoreUndo(layer->Bitmap());
			}
		}
		// Then actually remove the layer
		//int32 removed_layer_index = layer_list->IndexOf(other);
		layer_id_list[other->Id()] = NULL;

		layer_list->RemoveItem(other);

		SetImageSize();
		if (!merge_with_upper)
			--current_layer_index;

		current_layer_index = min_c(current_layer_index, layer_list->CountItems()-1);

		if (current_layer_index >= 0) {
			((Layer*)layer_list->ItemAt(current_layer_index))->ActivateLayer(TRUE);
		}

		Render();
		delete other;
		thread_id a_thread = spawn_thread(Layer::CreateMiniatureImage,"create mini picture",B_LOW_PRIORITY,target);
		resume_thread(a_thread);
	}

	return TRUE;
}


bool
Image::RemoveLayer(Layer *removed_layer, int32 removed_layer_id)
{
	// Things we should check here:
	//	1.	Do not remove the only layer.
	//	2.	Are there any other visible layers.
	//	3.	Will the same current layer-index still be good or should we reduce it by one.
	//	4.	Is the deleted layer the current layer.
	//	5.	If the layer count goes to 1, we should also delete the composite picture.
	//
	bool active_layer_changed = FALSE;

	if (layer_list->CountItems() <= 1) {
		// The UI actually prevents this from ever happening.
		// But lets keep this just in case there is something wrong with the UI.
		BAlert *an_alert;
		an_alert = new BAlert("",B_TRANSLATE(
			"Cannot delete the only layer. Create another picture to get a fresh start."),
			B_TRANSLATE("OK"));
		an_alert->Go();
		return FALSE;
	}
	else {
		// Store the undo.
		UndoEvent *new_event = undo_queue->AddUndoEvent(B_TRANSLATE("Delete layer"),rendered_image);
		if (new_event != NULL) {
			for (int32 i=0;i<layer_list->CountItems();i++) {
				Layer *layer = (Layer*)layer_list->ItemAt(i);
				UndoAction *new_action;
				if (layer->Id() != removed_layer_id)
					new_action = new UndoAction(layer->Id());
				else {
					new_action = new UndoAction(layer->Id(),DELETE_LAYER_ACTION,layer->Bitmap()->Bounds());
				}
				new_event->AddAction(new_action);
				new_action->StoreUndo(layer->Bitmap());
			}
		}
		// Then actually remove the layer
		removed_layer_id = removed_layer->Id();
		//int32 removed_layer_index = layer_list->IndexOf(removed_layer);
		layer_id_list[removed_layer_id] = NULL;
		layer_list->RemoveItem(removed_layer);
		SetImageSize();

		if (removed_layer->IsActive() == TRUE) {
			current_layer_index = min_c(current_layer_index,layer_list->CountItems()-1);
			if (current_layer_index >= 0) {
				((Layer*)layer_list->ItemAt(current_layer_index))->ActivateLayer(TRUE);
			}
			active_layer_changed = TRUE;
		}
		else {
			for (int32 i=0;i<layer_list->CountItems();i++) {
				if (((Layer*)layer_list->ItemAt(i))->IsActive() == TRUE)
					current_layer_index = i;
			}
		}

		Render();
		delete removed_layer;
	}

	return active_layer_changed;
}


bool
Image::ToggleLayerVisibility(Layer *toggled_layer, int32)
{
	bool layer_visibility = toggled_layer->IsVisible();
	int32 number_of_visible_layers = 0;
	for (int32 i=0;i<layer_list->CountItems();i++) {
		if (((Layer*)(layer_list->ItemAt(i)))->IsVisible() == true)
			number_of_visible_layers++;
	}

	if ((number_of_visible_layers >= 2) && (layer_visibility == true)) {
		toggled_layer->SetVisibility(false);
		Render();
		return true;
	}
	else if (layer_visibility == false) {
		toggled_layer->SetVisibility(true);
		Render();
		return true;
	}
	else if (number_of_visible_layers <= 1) {
		// cannot make invisible the only layer
		toggled_layer->SetVisibility(true);
		return false;
	}
	return false;
}


Layer*
Image::ReturnLowerLayer(Layer *l)
{
	int32 index = layer_list->IndexOf(l);
	if (index > 0) {
		return (Layer*)layer_list->ItemAt(index-1);
	}
	else
		return NULL;
}


Layer*
Image::ReturnUpperLayer(Layer *l)
{
	int32 index = layer_list->IndexOf(l);
	if (index >= 0) {
		if (index < layer_list->CountItems()-1)
			return (Layer*)layer_list->ItemAt(index+1);
		else
			return NULL;
	}
	else
		return NULL;
}


status_t
Image::InsertLayer(BBitmap *layer_bitmap)
{
	AddLayer(layer_bitmap,NULL,TRUE);
	return B_OK;
}


void
Image::UpdateImageStructure(UndoEvent *event)
{
	Layer *current_layer = (Layer*)layer_list->ItemAt(current_layer_index);
	bool current_layer_deleted = FALSE;

	// First empty the layer_list
	for (int32 i=layer_list->CountItems()-1;i>=0;i--) {
		layer_list->RemoveItem(i);
	}
	UndoAction **actions = event->ReturnActions();
	BRect updated_rect(1000000,1000000,-1000000,-1000000);
	for (int32 i=0;i<event->ActionCount();i++) {
		int32 layer_id = actions[i]->LayerId();
		Layer *layer = layer_id_list[layer_id];
		BRect a_rect;
		if (layer != NULL){
			BBitmap *bitmap = actions[i]->ApplyUndo(layer->Bitmap(),a_rect);
			if (bitmap == NULL) {
				// The layer should be deleted
				if (layer == current_layer)
					current_layer_deleted = TRUE;

				layer_id_list[layer_id] = NULL;
				delete layer;
				layer = NULL;
			}
			else if (bitmap != layer->Bitmap()) {
				// The bitmap has changed size
				layer->ChangeBitmap(bitmap);
			}
			if (layer != NULL) {
				// This should be here in order to crete a miniature picture for
				// each layer that changes.
				thread_id a_thread = spawn_thread(Layer::CreateMiniatureImage,
					"create mini picture", B_LOW_PRIORITY, layer);
				resume_thread(a_thread);
			}
		}
		else {
			// We should add a layer
			BBitmap *bitmap = actions[i]->ApplyUndo(NULL,a_rect);
			if (bitmap != NULL) {
				layer_id_list[layer_id] = new Layer(bitmap->Bounds(),
					layer_id, image_view, HS_NORMAL_LAYER, bitmap);
				layer_id_list[layer_id]->SetVisibility(TRUE);
				layer_id_list[layer_id]->AddToImage(this);
				thread_id a_thread = spawn_thread(Layer::CreateMiniatureImage,
					"create mini picture", B_LOW_PRIORITY,
					layer_id_list[layer_id]);
				resume_thread(a_thread);
			}
			delete bitmap;
		}
		updated_rect.left = min_c(updated_rect.left, a_rect.left);
		updated_rect.right = max_c(updated_rect.right, a_rect.right);
		updated_rect.top = min_c(updated_rect.top, a_rect.top);
		updated_rect.bottom = max_c(updated_rect.bottom, a_rect.bottom);
	}
	// Here we copy the layers back to layer list
	for (int32 i = 0;i < event->ActionCount();i++) {
		int32 layer_id = actions[i]->LayerId();
		if (layer_id_list[layer_id] != NULL)
			layer_list->AddItem(layer_id_list[layer_id]);
	}
	SetImageSize();

	// Change the current layer to be right layer.
	if (current_layer_deleted) {
		current_layer_index = min_c(current_layer_index,layer_list->CountItems()-1);
	}
	else {
		current_layer_index = layer_list->IndexOf(current_layer);
	}
	Layer *active_layer = ((Layer*)(layer_list->ItemAt(current_layer_index)));
	if (active_layer != NULL)
		active_layer->ActivateLayer(TRUE);

	Render(updated_rect);
}


void
Image::RegisterLayersWithUndo()
{
	if (undo_queue != NULL) {
		for (int32 i=0;i<layer_list->CountItems();i++) {
			Layer *layer = (Layer*)layer_list->ItemAt(i);
			undo_queue->RegisterLayer(layer->Id(),layer->Bitmap());
		}
	}
}


BBitmap*
Image::ReturnThumbnailImage()
{
	return thumbnail_image;
}


BBitmap*
Image::ReturnRenderedImage()
{
	return rendered_image;
}


BBitmap*
Image::ReturnActiveBitmap()
{
	return ((Layer*)layer_list->ItemAt(current_layer_index))->Bitmap();
}


bool
Image::ContainsLayer(Layer *layer)
{
	if (layer_list->IndexOf(layer) < 0)
		return FALSE;
	else
		return TRUE;
}


status_t
Image::ReadLayers(BFile &file)
{
	// The previous is the old way of reading layers. This new way is not
	// compatible with the old files. Old system should be supported somehow
	// though. First thing to do is to check the endianness of the file.
	file.Seek(0,SEEK_SET);
	int32 lendian;
	if (file.Read(&lendian,sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int64 length = FindProjectFileSection(file,PROJECT_FILE_LAYER_SECTION_ID);

	if (length == 0)
		return B_ERROR;

	// Read the layer-count
	int32 count;
	if (file.Read(&count,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (lendian == 0x00000000)
		count = B_BENDIAN_TO_HOST_INT32(count);
	else if (uint32(lendian) == 0xFFFFFFFF)
		count = B_LENDIAN_TO_HOST_INT32(count);
	else
		return B_ERROR;

	// Read the used compression-method. Though currently it is unused.
	int32 compression_method;
	if (file.Read(&compression_method,sizeof(int32)) != sizeof(int32))
		return B_ERROR;
	if (lendian == 0x00000000)
		compression_method = B_BENDIAN_TO_HOST_INT32(compression_method);
	else if (uint32(lendian) == 0xFFFFFFFF)
		compression_method = B_LENDIAN_TO_HOST_INT32(compression_method);

	for (int32 i=0;i<count;i++) {
		// Here read the layers
		Layer *layer = Layer::readLayer(file, image_view,
			atomic_add(&next_layer_id, 1), uint32(lendian) == 0xFFFFFFFF,
			compression_method);

		if (layer != NULL) {
			// Change the layer's id-number
			layer_list->AddItem(layer);
			layer->AddToImage(this);
			// Also inform the undo-queue about this layer.
			undo_queue->RegisterLayer(layer->Id(),layer->Bitmap());

			layer->ActivateLayer(FALSE);	// We activate only the first read layer
			// if this is the first layer we should create the composite picture
			if ((layer_list->CountItems() == 1) && (rendered_image == NULL) ) {
				rendered_image = new BBitmap(BRect(0,0,image_width-1,image_height-1),B_RGBA32);
				if (rendered_image->IsValid() == FALSE) {
					// If the creatioin of composite picture fails we should remove the newly added
					// layer and inform the user that we cannot add a layer. The method that called
					// this function will take care of informing the user.
					delete rendered_image;
					rendered_image = NULL;
					layer_id_list[layer->Id()] = NULL;
					layer_list->RemoveItem(layer);
					delete layer;
					throw std::bad_alloc();
				}
			}
		}
	}
	layer_id_list = new Layer*[next_layer_id];
	for (int32 i=0;i<layer_list->CountItems();i++) {
		Layer *layer = (Layer*)layer_list->ItemAt(i);
		layer_id_list[layer->Id()] = layer;
	}

	if (layer_list->CountItems() == 0)
		return B_ERROR;
	else {
		((Layer*)layer_list->ItemAt(0))->ActivateLayer(TRUE);
		current_layer_index = 0;
		return B_OK;
	}
}


int64
Image::WriteLayers(BFile &file)
{
	int64 written_bytes = 0;
	int32 layer_count = layer_list->CountItems();
	if (file.Write(&layer_count,sizeof(int32)) != sizeof(int32))
		return written_bytes;

	written_bytes += sizeof(int32);

	int32 compression_method = NO_COMPRESSION;

	if (file.Write(&compression_method,sizeof(int32)) != sizeof(int32))
		return written_bytes;

	written_bytes += sizeof(int32);

	for (int32 i=0;i<layer_count;i++) {
		Layer *layer = (Layer*)layer_list->ItemAt(i);
		written_bytes += layer->writeLayer(file,compression_method);
	}

	return written_bytes;
}


status_t
Image::ReadLayersOldStyle(BFile &file,int32 count)
{
	Layer *layer = NULL;
	// If count is not within reasonable limits we must presume that the file is corrupt.
	if ( (count<1) || (count > 10000) ) {
		return B_ERROR;
	}

	int32 max_layer_id = -2;
	while ( count > 0 ){
		// If a layer is NULL we should actually stop reading.
		layer = Layer::readLayerOldStyle(file,image_view,atomic_add(&next_layer_id,1));
		if (layer != NULL) {
			// Change the layer's id-number
			layer_list->AddItem(layer);
			max_layer_id = max_c(max_layer_id,layer->Id());
			// Also inform the undo-queue about this layer.
			undo_queue->RegisterLayer(layer->Id(),layer->Bitmap());

			layer->ActivateLayer(FALSE);	// We activate only the first read layer
			// if this is the first layer we should create the composite picture
			if ((layer_list->CountItems() == 1) && (rendered_image == NULL) ) {
				rendered_image = new BBitmap(BRect(0,0,image_width-1,image_height-1),B_RGBA32);
				if (rendered_image->IsValid() == FALSE) {
					// If the creatioin of composite picture fails we should remove the newly added
					// layer and inform the user that we cannot add a layer. The method that called
					// this function will take care of informing the user.
					delete rendered_image;
					rendered_image = NULL;
					layer_id_list[layer->Id()] = NULL;
					layer_list->RemoveItem(layer);
					delete layer;
					throw std::bad_alloc();
				}
			}
		}
		count--;
	}

	layer_id_list = new Layer*[max_layer_id+1];
	for (int32 i=0;i<layer_list->CountItems();i++) {
		layer = (Layer*)layer_list->ItemAt(i);
		layer_id_list[layer->Id()] = layer;
	}

	// We return failior if we could not read a single layer.
	if (layer_list->CountItems() == 0)
		return B_ERROR;
	else {
		((Layer*)layer_list->ItemAt(0))->ActivateLayer(TRUE);
		current_layer_index = 0;
		return B_OK;
	}
}


void
Image::CalculateThumbnails()
{
	thread_id a_thread = spawn_thread(Layer::CreateMiniatureImage,"create mini picture",B_LOW_PRIORITY,layer_list->ItemAt(current_layer_index));
	resume_thread(a_thread);

	a_thread = spawn_thread(calculate_thumbnail_image,"create mini picture",B_LOW_PRIORITY,this);
	resume_thread(a_thread);
}


int32
Image::calculate_thumbnail_image(void *data)
{
	Image *this_pointer = (Image*)data;

	BBitmap *from = this_pointer->rendered_image;
	BBitmap *to = this_pointer->thumbnail_image;

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	color.bytes[0] = 0xFF;
	color.bytes[1] = 0xFF;
	color.bytes[2] = 0xFF;
	color.bytes[3] = 0x00;

	int32 miniature_width = (int32)((to->Bounds().Width()+1) * (min_c(from->Bounds().Width()/from->Bounds().Height(),1)));
	int32 miniature_height = (int32)((to->Bounds().Height()+1) * (min_c(from->Bounds().Height()/from->Bounds().Width(),1)));

	// Here we copy the contents of the_bitmap to miniature image.
	// by using a DDA-scaling algorithm first take the dx and dy variables
	float dx = 	(from->Bounds().Width() + 1)/(float)miniature_width;
	float dy = 	(from->Bounds().Height() + 1)/(float)miniature_height;
	int32 x=0,y=0;

	int32 x_offset_left = (int32)floor((float)(to->Bounds().Width()+1-miniature_width)/2.0);
	int32 x_offset_right = (int32)ceil((float)(to->Bounds().Width()+1-miniature_width)/2.0);

	int32 y_offset = (int32)((to->Bounds().Height()+1-miniature_height)/2);

	// The bitmap might be changed and deleted while we are accessing it.
	int32	b_bpr = from->BytesPerRow()/4;
	uint32 *big_image;
	uint32 *small_image = (uint32*)to->Bits();
	big_image = (uint32*)from->Bits();
	// Clear the parts that we do not set.
	for (int32 i=0;i<(to->Bounds().Width()+1)*y_offset;i++)
		*small_image++ = color.word;

	while (y < miniature_height) {
		for (int32 i=0;i<x_offset_left;i++)
			*small_image++ = color.word;

		while (x < miniature_width) {
			*small_image++ = *(big_image + ((int32)(y*dy))*b_bpr + (int32)(x*dx));
			x++;
		}
		y++;

		for (int32 i=0;i<x_offset_right;i++)
			*small_image++ = color.word;

		x = 0;
	}

	// Clear the rest of the image
	while (small_image != ((uint32*)to->Bits() + to->BitsLength()/4))
		*small_image++ = color.word;

	return B_OK;
}


status_t
Image::RegisterDitheredUser(void *user)
{
	if (dithered_users->CountItems() == 0) {
		dithered_image = new BBitmap(BRect(0,0,image_width-1,image_height-1),B_CMAP8);
		if (dithered_image->IsValid() == FALSE) {
			delete dithered_image;
			dithered_image = NULL;
			return FALSE;
		}

		int32 previous_users = atomic_add(&color_candidate_users,1);

		if ((previous_users == 0) && (color_candidates == NULL)) {
			// Create the candidate-map. This is done in a separate thread
			thread_id candidate_creator_thread = spawn_thread(candidate_creator,"candidate_creator_thread",B_NORMAL_PRIORITY,this);
			resume_thread(candidate_creator_thread);
		}

		DoDither(dithered_image->Bounds());
	}

	if (dithered_users->HasItem(user) == FALSE)
		dithered_users->AddItem(user);

	return B_OK;
}


status_t
Image::UnregisterDitheredUser(void *user)
{
	dithered_users->RemoveItem(user);

	if (dithered_users->CountItems() == 0) {
		delete dithered_image;
		dithered_image = NULL;
	}

	return B_OK;
}


int32
Image::candidate_creator(void*)
{
	if (color_candidates == NULL) {
		BScreen *screen = new BScreen();
		const color_map *map = screen->ColorMap();
		delete screen;
		for (int32 i=0;i<256;i++) {
			color_list[i] = map->color_list[i];
		}

		color_entry *candidates = new color_entry[2*32768];
		rgb_color *rgb_color_map = new rgb_color[32768];
		for (int32 i=0;i<32768;i++) {
//			rgb_color_map[i].blue = 255 * (float)((i) & 0x1F) / (float)0x1F;
//			rgb_color_map[i].green = 255 * (float)((i >> 5) & 0x1F) / (float)0x1F;
//			rgb_color_map[i].red = 255 * (float)((i >> 10) & 0x1F) / (float)0x1F;
			rgb_color_map[i].blue = (i & 0x1F) << 3;
			rgb_color_map[i].green = ((i>>5) & 0x1F) << 3;
			rgb_color_map[i].red = ((i>>10) & 0x1F) << 3;

		}

		for (int32 i=0;i<32768;i++) {
			// Find the two closest color-indexes for the RGB15 color i.
			uint8 c1 = 0, c2 = 0;	// The color indexes
			float p1,p2;	// and their probabilities.

			int32 dist1 = 1000000;
			int32 dist2 = 1000000;

			rgb_color color1,color2;
			color1 = rgb_color_map[i];
			for (int32 j=0;j<256;j++) {
				color2 = color_list[j];
				int32 new_dist = abs(color1.red-color2.red)+abs(color1.green-color2.green)+abs(color1.blue-color2.blue);
				if (new_dist < dist1) {
					c2 = c1;
					c1 = j;
					dist2 = dist1;
					dist1 = new_dist;
				}
				else if (new_dist < dist2) {
					c2 = j;
					dist2 = new_dist;
				}
			}

			// Here calculate the probabilities for the colors.
			if (dist2 > 3*dist1) {
				p1 = 1.0;
				p2 = 0.0;
			}
			else {
				p1 = 0.5 + (float)(dist1-dist2)/(float)(3*dist1)*0.5;
				p2 = 1.0-p1;
			}

			candidates[2*i].value = c1;
			candidates[2*i].probability = p1;

			candidates[2*i+1].value = c2;
			candidates[2*i+1].probability = p1+p2;
		}
		color_candidates = candidates;
		delete[] rgb_color_map;
	}

	return B_OK;
}


int32
Image::enter_render(void *data)
{
	Image *this_pointer = (Image*)data;

	BRect rect;

	thread_id sender;
	receive_data(&sender,&rect,sizeof(BRect));

	return this_pointer->DoRender(rect);
}


int32
Image::enter_render_nobg(void *data)
{
	Image *this_pointer = (Image*)data;

	BRect rect;

	thread_id sender;
	receive_data(&sender,&rect,sizeof(BRect));

	return this_pointer->DoRender(rect, false);
}


int32
Image::DoRender(BRect area, bool bg)
{
	// The area is stated in rendered_image's coordinate system
	// this function will combine all the visible layers into the
	// rendered_image for the area part

	// Ensure that bounds of rendered_image are not exceeded.
	area = area & rendered_image->Bounds();

	// these variables are for row-length of the bitmaps in uint32
	// e.g how many 32-bit groups there are in a row
	int32 srl;
	int32 drl = rendered_image->BytesPerRow()/4;

	// these variables are for width and height of area
	int32 width = area.IntegerWidth()+1;
	int32 height = area.IntegerHeight()+1;

	// these variables are for source and destination bitmaps' start-coordinates
	int32 s_start_x,s_start_y;
	int32 d_start_x,d_start_y;
	d_start_x = (int32)area.left;
	d_start_y = (int32)area.top;

	// these are the pointers to source and destination bitmaps.
	uint32 *s_bits;
	uint32 *d_bits;

	int32 layer_count = layer_list->CountItems();
	Layer *layer;
	int32 layer_number = 0;

	// First clear the image for the required part.
	if (bg) {
		int32 gridSize;
		uint32 color1;
		uint32 color2;

		gridSize = 20;
		rgb_color rgb1, rgb2;
		rgb1.red = rgb1.green = rgb1.blue = 0xBB;
		rgb2.red = rgb2.green = rgb2.blue = 0x99;
		rgb1.alpha = rgb2.alpha = 0xFF;
		color1 = RGBColorToBGRA(rgb1);
		color2 = RGBColorToBGRA(rgb2);

		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);

			gridSize = settings.GetInt32(skBgGridSize, gridSize);
			color1 = settings.GetUInt32(skBgColor1, color1);
			color2 = settings.GetUInt32(skBgColor2, color2);
		}

		BitmapUtilities::CheckerBitmap(rendered_image,
			color1, color2, gridSize, &area);
	} else {
		union color_conversion white_alpha;
		white_alpha.word = 0xFFFFFFFF;
		white_alpha.bytes[3] = 0x00;

		BitmapUtilities::ClearBitmap(rendered_image, white_alpha.word, &area);
	}

	// Then mix each layer over the previous ones.
	const uint32 *FixedAlphaTable;

	while (layer_number < layer_count) {
		layer = (Layer*)layer_list->ItemAt(layer_number);

		if (layer->IsVisible()) {
			FixedAlphaTable = layer->ReturnFixedAlphaTable();

			srl = layer->Bitmap()->BytesPerRow()/4;
			s_start_x = d_start_x;
			s_start_y = d_start_y;
			s_bits = (uint32*)layer->Bitmap()->Bits();
			d_bits = (uint32*)rendered_image->Bits();
			uint32 s;
			uint32 d;
			uint32 target;

			// adjust the pointers to correct starting-positions
			s_bits += srl*s_start_y + s_start_x;
			d_bits += drl*d_start_y + d_start_x;

			uint32 As;
			uint32 Ad;

			// Alpha-value is presence of pixel, hence 0x00 is transparent and 0xff for alpha
			// is fully visible.
			for (int32 y=0;y<height;++y) {
				for (int32 x=0;x<width;++x) {
					s = *s_bits;
					d = *d_bits;

					union color_conversion src;
					src.word = s;
					src.bytes[3] *= layer->GetTransparency();

					target = src_over_fixed_blend(d, src.word, layer->GetBlendMode());
					*d_bits++ = target;
					++s_bits;
				}

				s_bits += srl - width;
				d_bits += drl - width;
			}
		}
		layer_number++;
	}

	return B_OK;
}


int32
Image::enter_dither(void *data)
{
	Image *this_pointer = (Image*)data;

	BRect rect;

	thread_id sender;
	receive_data(&sender,&rect,sizeof(BRect));

	return this_pointer->DoDither(rect);
}


int32
Image::DoDither(BRect area)
{
// Here we try to dither with the N-candidate method.
// The candidates for a color are in the array color_candidates.
// each color has four entries of struct candidate. We take a
// random number which decides which of those four entries to use.

	if (color_candidates != NULL) {
		if (dithered_image != NULL) {
			area = area & dithered_image->Bounds();
			if (area.IsValid() == TRUE) {
				uint8 *dithered_bits = (uint8*)dithered_image->Bits();
				uint32 dithered_bpr = dithered_image->BytesPerRow();

				uint32 *bgra_bits = (uint32*)rendered_image->Bits();
				uint32 bgra_bpr = rendered_image->BytesPerRow()/4;

				int32 left = (int32)area.left;
				int32 right = (int32)area.right;
				int32 top = (int32)area.top;
				int32 bottom = (int32)area.bottom;

				float one_per_1024 = 1.0/1024;

				union {
					uint8 bytes[4];
					uint32 word;
				} color;

				for (int32 y=top;y<=bottom;y++) {
					for (int32 x=left;x<=right;x++) {
						float random_number = (rand() % 1024) * one_per_1024;

						color.word = *(bgra_bits + x + y*bgra_bpr);

						int32 rgb15 = ( ((color.bytes[2] & 0xf8) << 7) |
										((color.bytes[1] & 0xf8) << 2) |
										((color.bytes[0] & 0xf8) >> 3) );
//						int32 rgb15 = ( ((int32)((color.bytes[2] / 255.0)*0x1f)<<10) |
//										((int32)((color.bytes[1] / 255.0)*0x1f)<<5) |
//										((int32)(color.bytes[0] / 255.0)*0x1f) );
						int8 value;
						if (color_candidates[2*rgb15].probability < random_number)
							value = color_candidates[2*rgb15].value;
						else
							value = color_candidates[2*rgb15+1].value;

						*(dithered_bits + x + y*dithered_bpr) = value;
					}
				}

			}
		}
		return B_OK;
	}
	else {
		return B_ERROR;
	}
}


int32
Image::enter_render_preview(void *data)
{
	Image *this_pointer = (Image*)data;

	BRect rect;
	int32 resolution;
	thread_id sender;
	resolution = receive_data(&sender,&rect,sizeof(BRect));

	return this_pointer->DoRenderPreview(rect,resolution);
}


int32
Image::DoRenderPreview(BRect area,int32 resolution)
{
	if (layer_list->CountItems() >= 1){
		// We take pointers to bitmaps of all visible layers.
		uint32 **layer_bits = new uint32*[layer_list->CountItems()];
		uint32 *layer_bprs = new uint32[layer_list->CountItems()];
		float *alpha = new float[layer_list->CountItems()];
		uint8 *blend = new uint8[layer_list->CountItems()];

		int32 visible_layer_count = 0;
		for (int32 i=0;i<layer_list->CountItems();i++) {
			Layer *layer = (Layer*)layer_list->ItemAt(i);
			if (layer->IsVisible() == TRUE) {
				layer_bits[visible_layer_count] = (uint32*)layer->Bitmap()->Bits();
				layer_bprs[visible_layer_count] = layer->Bitmap()->BytesPerRow()/4;
				alpha[visible_layer_count] = layer->GetTransparency();
				blend[visible_layer_count] = layer->GetBlendMode();
				visible_layer_count++;
			}
		}

		uint32 *composite_bits = (uint32*)rendered_image->Bits();
		uint32	composite_bpr = rendered_image->BytesPerRow()/4;
		// Then we iterate through all the pixels in the composite image and
		// set them properly.
		int32 width = rendered_image->Bounds().IntegerWidth();
		int32 height = rendered_image->Bounds().IntegerHeight();

		int32 left = (int32)area.left;
		int32 top = (int32)area.top;
		int32 right = (int32)area.right;
		int32 bottom = (int32)area.bottom;

		int32 gridSize;
		uint32 color1;
		uint32 color2;

		gridSize = 20;
		rgb_color rgb1, rgb2;
		rgb1.red = rgb1.green = rgb1.blue = 0xBB;
		rgb2.red = rgb2.green = rgb2.blue = 0x99;
		rgb1.alpha = rgb2.alpha = 0xFF;
		color1 = RGBColorToBGRA(rgb1);
		color2 = RGBColorToBGRA(rgb2);

		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);

			gridSize = settings.GetInt32(skBgGridSize, gridSize);
			color1 = settings.GetUInt32(skBgColor1, color1);
			color2 = settings.GetUInt32(skBgColor2, color2);
		}

		BitmapUtilities::CheckerBitmap(rendered_image,
			color1, color2, gridSize, &area);

		for (int32 y=top;y<=bottom;y+=resolution) {
			for (int32 x=left;x<=right;x+=resolution) {
				// Get the target value by combining the layers' pixels
				uint32 target = *(composite_bits + x + y*composite_bpr);
				for (int32 j=0;j<visible_layer_count;j++) {
					uint32 layer = layer_bits[j][x + y*layer_bprs[j]];

					union color_conversion src;
					src.word = layer;
					src.bytes[3] *= alpha[j];

					target = src_over_fixed_blend(target, src.word, blend[j]);
				}
				// Then copy the target-value to proper places in the composite picture
				int32 x_dimension = min_c(resolution,width+1-x);
				int32 y_dimension = min_c(resolution,height+1-y);
				for (int32 dy=0;dy<y_dimension;dy++) {
					for (int32 dx=0;dx<x_dimension;dx++) {
						composite_bits[x+dx + (y+dy)*composite_bpr] = target;
					}
				}
			}
		}

		for (int32 i=0;i<visible_layer_count;i++) {
			layer_bits[i] = NULL;
		}

		delete[] blend;
		delete[] alpha;
		delete[] layer_bits;
		delete[] layer_bprs;
	}

	return B_OK;
}
