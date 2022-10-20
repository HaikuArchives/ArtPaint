/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "UndoQueue.h"

#include "Image.h"
#include "Selection.h"
#include "SettingsServer.h"


#include <Alert.h>
#include <Catalog.h>
#include <MenuItem.h>


#include <new>
#include <stdio.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "UndoQueue"

int32 UndoQueue::maximum_queue_depth = 10;
BList* UndoQueue::queue_list = new BList();


UndoQueue::UndoQueue(BMenuItem *undo_item,BMenuItem *redo_item,ImageView *iv)
{
	image_view = iv;
	layer_bitmaps = NULL;
	layer_bitmap_count = 0;

	undo_menu_item = undo_item;
	redo_menu_item = redo_item;
	current_event = NULL;
	first_event = NULL;
	last_event = NULL;

	current_queue_depth = 0;

	queue_list->AddItem(this);

	selection_data = new SelectionData();

	UpdateMenuItems();
}


UndoQueue::UndoQueue(BFile&)
{
}


UndoQueue::~UndoQueue()
{
	UndoEvent *spare_event = first_event;
	while (spare_event != NULL) {
		first_event = spare_event->next_event;
		delete spare_event;
		spare_event = first_event;
	}

	queue_list->RemoveItem(this);

	for (int32 i=0;i<layer_bitmap_count;i++) {
		delete layer_bitmaps[i];
		layer_bitmaps[i] = NULL;
	}
	delete[] layer_bitmaps;

	delete selection_data;
}


UndoEvent* UndoQueue::AddUndoEvent(const char *name,const BBitmap *thumbnail,bool remove_tail)
{
	if (maximum_queue_depth == 0)
		return NULL;

	UndoEvent *event = new UndoEvent(name,thumbnail);
	current_queue_depth++;

	if (remove_tail == FALSE) {
		// Insert the new event in between.
		if (current_event != NULL) {
			event->next_event = current_event->next_event;
			event->previous_event = current_event;
			event->next_event->previous_event = event;
			current_event->next_event = event;
		}
	}
	else {
		UndoEvent *spare_event;
		if (current_event != NULL)
			spare_event = current_event->next_event;
		else {
			spare_event = first_event;
			first_event = NULL;
		}
		// Delete the rest of the events
		while (spare_event != NULL) {
			UndoEvent *another_spare_event = spare_event->next_event;
			delete spare_event;
			spare_event = another_spare_event;
			current_queue_depth--;
		}

		if (current_event != NULL) {
			current_event->next_event = event;
			event->previous_event = current_event;
		}
	}

	if (first_event == NULL)
		first_event = event;
	current_event = event;

	event->SetQueue(this);

	TruncateQueue();

	UpdateMenuItems();
	return event;
}


status_t UndoQueue::RemoveEvent(UndoEvent *event)
{
	current_queue_depth--;

	UndoEvent *spare_event = first_event;
	while ((spare_event != event) && (spare_event != NULL)) {
		spare_event = spare_event->next_event;
	}
	if (spare_event == NULL)
		return B_ERROR;


	// If the current event is the event that is removed we must change the
	// current event also. At the moment this should happen always.
	if (current_event == spare_event) {
		if (event->previous_event != NULL)
			current_event = spare_event->previous_event;
		else
			current_event = spare_event->next_event;
	}


	// If the first event is the removed one, we should also update its
	// status.
	if (first_event == spare_event) {
		first_event = spare_event->next_event;
	}

	// Unlink the event
	if (spare_event->previous_event != NULL) {
		spare_event->previous_event->next_event = spare_event->next_event;
	}
	if (spare_event->next_event != NULL) {
		spare_event->next_event->previous_event = spare_event->previous_event;
	}


	UpdateMenuItems();
	return B_OK;
}

BBitmap* UndoQueue::ReturnLayerSpareBitmap(int32 layer_id,BBitmap *layer_bitmap)
{
	if (layer_id > layer_bitmap_count-1) {
		BBitmap **new_bitmaps = new BBitmap*[layer_id+1];
		for (int32 i=0;i<layer_id+1;i++)
			new_bitmaps[i] = NULL;

		for (int32 i=0;i<layer_bitmap_count;i++) {
			new_bitmaps[i] = layer_bitmaps[i];
			layer_bitmaps[i] = NULL;
		}

		delete[] layer_bitmaps;
		layer_bitmaps = new_bitmaps;
		layer_bitmap_count = layer_id+1;
	}

	if (layer_bitmaps[layer_id] == NULL) {
		if (layer_bitmap != NULL) {
			layer_bitmaps[layer_id] = new BBitmap(layer_bitmap->Bounds(),B_RGB32);
			if (layer_bitmaps[layer_id]->IsValid() == FALSE)
				throw std::bad_alloc();
		}
	}

	return layer_bitmaps[layer_id];
}


status_t UndoQueue::ChangeLayerSpareBitmap(int32 layer_id, BBitmap *layer_bitmap)
{
	// This function doesn't delete the old bitmap.
	if (layer_id > layer_bitmap_count-1) {
		BBitmap **new_bitmaps = new BBitmap*[layer_id+1];
		for (int32 i=0;i<layer_id+1;i++)
			new_bitmaps[i] = NULL;

		for (int32 i=0;i<layer_bitmap_count;i++) {
			new_bitmaps[i] = layer_bitmaps[i];
			layer_bitmaps[i] = NULL;
		}
		delete[] layer_bitmaps;
		layer_bitmaps = new_bitmaps;
		layer_bitmap_count = layer_id+1;
	}

	if (layer_bitmap != NULL) {
		layer_bitmaps[layer_id] = new BBitmap(layer_bitmap->Bounds(),B_RGB32);
		if (layer_bitmaps[layer_id]->IsValid() == FALSE)
			throw std::bad_alloc();

		uint32 *spare_bits = (uint32*)layer_bitmaps[layer_id]->Bits();
		uint32 *bits = (uint32*)layer_bitmap->Bits();
		uint32 bitslength = layer_bitmap->BitsLength()/4;
		for (uint32 i = 0; i < bitslength; i++)
			*spare_bits++ = *bits++;
	}
	else {
		layer_bitmaps[layer_id] = NULL;
	}
	return B_OK;
}



void UndoQueue::RegisterLayer(int32 layer_id,BBitmap *layer_bitmap)
{
	if ((maximum_queue_depth > 0) || (maximum_queue_depth == INFINITE_QUEUE_DEPTH)) {
		if (layer_id > layer_bitmap_count-1) {
			BBitmap **new_bitmaps = new BBitmap*[layer_id+1];
			for (int32 i=0;i<layer_id+1;i++)
				new_bitmaps[i] = NULL;

			for (int32 i=0;i<layer_bitmap_count;i++) {
				new_bitmaps[i] = layer_bitmaps[i];
				layer_bitmaps[i] = NULL;
			}
			delete[] layer_bitmaps;
			layer_bitmaps = new_bitmaps;
			layer_bitmap_count = layer_id+1;

		}

		if (layer_bitmaps[layer_id] == NULL) {
			if (layer_bitmap != NULL) {
				layer_bitmaps[layer_id] = new BBitmap(layer_bitmap->Bounds(),B_RGB32);
				if (layer_bitmaps[layer_id]->IsValid() == FALSE)
					throw std::bad_alloc();

				uint32	*source_bits = (uint32*)layer_bitmap->Bits();
				uint32	*target_bits = (uint32*)layer_bitmaps[layer_id]->Bits();
				int32 bitslength = layer_bitmap->BitsLength()/4;

				for (int32 i=0;i<bitslength;i++)
					*target_bits++ = *source_bits++;
			}
		}
	}
}


UndoEvent* UndoQueue::Undo()
{
	UndoEvent *returned_event = NULL;

	if (current_event != NULL) {
		returned_event = current_event;
		current_event = current_event->previous_event;
		if (returned_event->ActionCount() == 1) {
			if (returned_event->ReturnActions()[0]->type == ADD_LAYER_ACTION) {
				// This event should be removed.
				current_event = returned_event;
				returned_event = NULL;
			}
		}
	}

	UpdateMenuItems();
	return returned_event;
}


UndoEvent* UndoQueue::Redo()
{
	UndoEvent *returned_event = NULL;
	if (current_event == NULL)
		returned_event = current_event = first_event;
	else {
		if (current_event->next_event != NULL) {
			current_event = current_event->next_event;
			returned_event = current_event;
		}
	}
	UpdateMenuItems();
	return returned_event;
}



const char* UndoQueue::ReturnUndoEventName()
{
	const char *name = NULL;

	UndoEvent *named_event = current_event;
	if (named_event != NULL) {
		UndoAction **actions = named_event->ReturnActions();
		if (actions != NULL) {
			if ((named_event->ActionCount() > 1) || (actions[0]->type != ADD_LAYER_ACTION)) {
				name = named_event->ReturnName();
			}
		}
		else
			name = named_event->ReturnName();
	}

	return name;
}


const char* UndoQueue::ReturnRedoEventName()
{
	const char *name = NULL;

	UndoEvent *named_event;
	if (current_event != NULL) {
		named_event = current_event->next_event;
	}
	else
		named_event = first_event;

	if (named_event != NULL)
		name = named_event->ReturnName();

	return name;
}



void UndoQueue::UpdateMenuItems()
{
	const char *event_name;
	event_name = ReturnUndoEventName();

	if (undo_menu_item != NULL) {
		if (event_name != NULL) {
			char menu_text[256];
			sprintf(menu_text,"%s %s",B_TRANSLATE("Undo"),event_name);
			undo_menu_item->SetLabel(menu_text);
			undo_menu_item->SetEnabled(TRUE);
		}
		else {
			undo_menu_item->SetLabel(B_TRANSLATE("Undo"));
			undo_menu_item->SetEnabled(FALSE);
		}
	}

	event_name = ReturnRedoEventName();
	if (redo_menu_item != NULL) {
		if (event_name != NULL) {
			char menu_text[256];
			sprintf(menu_text,"%s %s",B_TRANSLATE("Redo"),event_name);
			redo_menu_item->SetLabel(menu_text);
			redo_menu_item->SetEnabled(TRUE);
		}
		else {
			redo_menu_item->SetLabel(B_TRANSLATE("Redo"));
			redo_menu_item->SetEnabled(FALSE);
		}
	}
}


void UndoQueue::SetMenuItems(BMenuItem *undo_item,BMenuItem *redo_item)
{
	undo_menu_item = undo_item;
	redo_menu_item = redo_item;
	UpdateMenuItems();
}


void UndoQueue::HandleLowMemorySituation()
{
	BAlert *memory_alert = new BAlert("memory_alert", B_TRANSLATE(
		"The undo-mechanism has run out of memory.\n"
		"The depth of undo will be limited so that the most recent events can be kept in memory. "
		"It is advisable to save your work at this point to avoid any loss of data in case the "
		"memory runs out completely.\n"
		"You may also want to adjust the undo-depth in the settings-window."),
		B_TRANSLATE("Bummer"), NULL, NULL, B_WIDTH_AS_USUAL,B_WARNING_ALERT);
	memory_alert->Go();

	// We may have to delete either redo-events or undo-events. Currently this function is called
	// only when there are no redo events so we only delete undo-events. This may change in the
	// future however.
	int32 deleted_event_count = 0;
	UndoEvent *deleted_event = first_event;
	while ((deleted_event_count < 5) && (deleted_event != NULL) && (deleted_event != current_event)) {
		first_event = deleted_event->next_event;
		first_event->previous_event = NULL;
		delete deleted_event;
		deleted_event = first_event;
		deleted_event_count++;
		current_queue_depth--;
	}
}


void
UndoQueue::SetQueueDepth(int32 depth)
{
	if ((depth != INFINITE_QUEUE_DEPTH)
		&& ((depth < maximum_queue_depth)
			|| (maximum_queue_depth == INFINITE_QUEUE_DEPTH))) {
		maximum_queue_depth = depth;
		for (int32 i = 0; i < queue_list->CountItems(); i++) {
			UndoQueue *queue = (UndoQueue*)queue_list->ItemAt(i);
			queue->TruncateQueue();
		}
	}

	if ((maximum_queue_depth == 0) && (depth != 0)) {
		maximum_queue_depth = depth;
		for (int32 i = 0; i < queue_list->CountItems(); i++) {
			UndoQueue *queue = (UndoQueue*)queue_list->ItemAt(i);
			queue->image_view->ReturnImage()->RegisterLayersWithUndo();
		}
	}

	maximum_queue_depth = depth;

	if (SettingsServer* server = SettingsServer::Instance())
		server->SetValue(SettingsServer::Application, skUndoQueueDepth, depth);
}


void
UndoQueue::TruncateQueue()
{
	if ((maximum_queue_depth != INFINITE_QUEUE_DEPTH) && (current_queue_depth > maximum_queue_depth)) {
		while (current_queue_depth > maximum_queue_depth) {
			// We should remove the events so that the nearest events to the current
			// event from both sides are removed last, and the furthest away events
			// first. If needed the actual current event is then removed at the end.

			// search the furhest away event from the current_event, or if current event
			// is NULL, from the first_event. If both are NULL, put the current_queue_length to
			// zero.
			UndoEvent *furthest_event = NULL;
			if (current_event != NULL) {
				UndoEvent *redo_direction=current_event;
				UndoEvent *undo_direction=current_event;
				while ((undo_direction != NULL) && (redo_direction != NULL)) {
					undo_direction = undo_direction->previous_event;
					redo_direction = redo_direction->next_event;
				}
				if (undo_direction != NULL) {
					while (undo_direction->previous_event != NULL)
						undo_direction = undo_direction->previous_event;
					furthest_event = undo_direction;
				}
				else if (redo_direction != NULL) {
					while (redo_direction->next_event != NULL)
						redo_direction = redo_direction->next_event;
					furthest_event = redo_direction;
				}
				else
					furthest_event = first_event;
			}
			else if (first_event != NULL) {
				furthest_event = first_event;
				while (furthest_event->next_event != NULL)
					furthest_event = furthest_event->next_event;
			}
			else
				current_queue_depth = 0;

			if (furthest_event == first_event) {
				if (first_event != NULL)
					first_event = first_event->next_event;
			}

			if (current_event == furthest_event)
				current_event = NULL;


			// Here we should unlink the furthest_event and delete it.
			// Also decrease the current undo-depth by one
			if (furthest_event != NULL) {
				if (furthest_event->next_event != NULL)
					furthest_event->next_event->previous_event = furthest_event->previous_event;
				if (furthest_event->previous_event != NULL)
					furthest_event->previous_event->next_event = furthest_event->next_event;

				furthest_event->next_event = NULL;
				furthest_event->previous_event = NULL;

				delete furthest_event;
			}
			current_queue_depth--;
		}
	}

	if (maximum_queue_depth == 0) {
		// Delete everything.
		for (int32 i=0;i<layer_bitmap_count;i++) {
			delete layer_bitmaps[i];
			layer_bitmaps[i] = NULL;
		}
		delete[] layer_bitmaps;
		layer_bitmap_count = 0;
		current_queue_depth = 0;
		layer_bitmaps = NULL;
	}

	UpdateMenuItems();
}


void
UndoQueue::SetSelectionData(const SelectionData *s)
{
	delete selection_data;
	selection_data = new SelectionData(s);
}
