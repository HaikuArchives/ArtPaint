/* 

	Filename:	UndoQueue.h
	Contents:	UndoQueue-class declaration	
	Author:		Heikki Suhonen
	
*/




#ifndef UNDO_QUEUE_H
#define UNDO_QUEUE_H

#include <MenuItem.h>
#include <File.h>

#include "ImageView.h"
#include "UndoEvent.h"

class Selection;

#define	INFINITE_QUEUE_DEPTH	-1

class UndoQueue {
		friend class UndoAction;

		BBitmap		**layer_bitmaps;
		int32		layer_bitmap_count;

		// The events form a doubly linked list.
		UndoEvent	*first_event;
		UndoEvent	*last_event;
		UndoEvent	*current_event;
			
static	int32		maximum_queue_depth;
		int32		current_queue_depth;
		
//		int32		event_count;
//		int32		max_event_count;

		BMenuItem	*undo_menu_item;
		BMenuItem	*redo_menu_item;				
		ImageView	*image_view;

		void		UpdateMenuItems();
		void		HandleLowMemorySituation();

		void		TruncateQueue();

static	BList		*queue_list;
const	char*		ReturnUndoEventName();
const	char*		ReturnRedoEventName();


SelectionData		*selection_data;			
		
public:
			UndoQueue(BMenuItem*,BMenuItem*,ImageView*);
			UndoQueue(BFile&);

		
			~UndoQueue();
		

UndoEvent*	AddUndoEvent(const char*,const BBitmap*,bool remove_tail=TRUE);
UndoEvent*	ReturnCurrentEvent() { return current_event; }

status_t	RemoveEvent(UndoEvent*);
int32		WriteUndoQueue(BFile&);	


BBitmap*	ReturnLayerSpareBitmap(int32 layer_id,BBitmap *layer_bitmap);
status_t	ChangeLayerSpareBitmap(int32 layer_id,BBitmap *layer_bitmap);

UndoEvent*	Undo();
UndoEvent*	Redo();



static	void		SetQueueDepth(int32);
static	int32		ReturnDepth() { return maximum_queue_depth; }


void		SetMenuItems(BMenuItem*,BMenuItem*);

// This function can be used to infor the queue about new layers for example
// when loading a project-file.
void		RegisterLayer(int32 layer_id,BBitmap *layer_bitmap);


SelectionData*	ReturnSelectionData() { return selection_data; }
void	SetSelectionData(const SelectionData*);
};

#endif