/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <InterfaceDefs.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <ScrollBar.h>

#include "FloaterManager.h"
#include "BrushStoreWindow.h"
#include "PaintApplication.h"
#include "Brush.h"
#include "Settings.h"
#include "UtilityClasses.h"
#include "DrawingTools.h"
#include "ToolSetupWindow.h"
#include "MessageFilters.h"
#include "BrushEditor.h"
#include "StringServer.h"
#include "ToolManager.h"

#define	BRUSH_INSET	2
#define	BRUSH_VAULT_WIDTH	(BRUSH_PREVIEW_WIDTH+2*BRUSH_INSET)
#define	BRUSH_VAULT_HEIGHT	(BRUSH_PREVIEW_WIDTH+2*BRUSH_INSET)

BList* BrushStoreWindow::brush_data = new BList();
BrushStoreWindow* BrushStoreWindow::brush_window = NULL;

BrushStoreWindow::BrushStoreWindow()
	:	BWindow(BRect(20,20,220,220),StringServer::ReturnString(BRUSHES_STRING),B_DOCUMENT_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect frame = ((PaintApplication*)be_app)->GlobalSettings()->brush_window_frame;
	((PaintApplication*)be_app)->GlobalSettings()->brush_window_visible = TRUE;
	frame = FitRectToScreen(frame);
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());

	BMenuBar *menu_bar;
	menu_bar = new BMenuBar(BRect(0,0,0,0),"brush window menu-bar");
	BMenu *a_menu = new BMenu(StringServer::ReturnString(BRUSH_STRING));
	menu_bar->AddItem(a_menu);
	a_menu->AddItem(new BMenuItem(StringServer::ReturnString(DELETE_SELECTED_BRUSH_STRING),new BMessage(HS_DELETE_SELECTED_BRUSH)));
	AddChild(menu_bar);

	BRect scroll_bar_frame = BRect(Bounds().right-B_V_SCROLL_BAR_WIDTH+1,menu_bar->Frame().bottom,Bounds().right+1,Bounds().bottom+1-B_H_SCROLL_BAR_HEIGHT);

	scroll_bar = new BScrollBar(scroll_bar_frame,"brush store scroll-bar",NULL,0,0,B_VERTICAL);
	AddChild(scroll_bar);
	scroll_bar->SetRange(0,0);

	store_view = new BrushStoreView(BRect(0,menu_bar->Frame().bottom + 1,scroll_bar->Frame().left-1,Bounds().bottom));
	AddChild(store_view);

	a_menu->FindItem(HS_DELETE_SELECTED_BRUSH)->SetTarget(store_view);

	store_view->MakeFocus(TRUE);
	scroll_bar->SetTarget(store_view);

	// The adding of brushes will be done in a separate thread.
	thread_id adder_thread = spawn_thread(brush_adder,"brush_adder",B_NORMAL_PRIORITY,this);
	resume_thread(adder_thread);

	brush_window = this;

	SetSizeLimits(scroll_bar->Frame().Width()+BRUSH_VAULT_WIDTH,10000,menu_bar->Frame().Height() + 1 + BRUSH_VAULT_HEIGHT,10000);

	window_feel feel = ((PaintApplication*)be_app)->GlobalSettings()->brush_window_feel;
	setFeel(feel);

	// Add a filter that will be used to catch mouse-down-messages in order
	// to activate this window when required
	Lock();
	BMessageFilter *activation_filter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,B_MOUSE_DOWN,window_activation_filter);
	AddCommonFilter(activation_filter);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();

	// TODO: check what this is used for once Haiku implements this
	SetWindowAlignment(B_PIXEL_ALIGNMENT, 0, 0, BRUSH_VAULT_WIDTH,
		int32(B_V_SCROLL_BAR_WIDTH + 1), 0, 0, BRUSH_VAULT_HEIGHT,
		int32(menu_bar->Bounds().Height() + 1));

	FloaterManager::AddFloater(this);
}


BrushStoreWindow::~BrushStoreWindow()
{
	((PaintApplication*)be_app)->GlobalSettings()->brush_window_frame	= Frame();
	((PaintApplication*)be_app)->GlobalSettings()->brush_window_visible = FALSE;
	brush_window = NULL;

	FloaterManager::RemoveFloater(this);
}


void BrushStoreWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

void BrushStoreWindow::writeBrushes(BFile &file)
{
	int32 number_of_brushes = brush_data->CountItems();
	file.Write(&number_of_brushes,sizeof(int32));
	for (int32 i=0;i<brush_data->CountItems();i++) {
		file.Write((brush_data->ItemAt(i)),sizeof(brush_info));
	}
}

void BrushStoreWindow::readBrushes(BFile &file)
{
	int32 number_of_brushes;
	brush_info info;
	if (file.Read(&number_of_brushes,sizeof(int32)) == sizeof(int32)) {
		for (int32 i=0;i<number_of_brushes;i++) {
			if (file.Read(&info,sizeof(brush_info)) == sizeof(brush_info)) {
				brush_data->AddItem(new brush_info(info));
			}
		}
	}
}

void BrushStoreWindow::AddBrush(Brush *brush)
{
	brush_data->AddItem(new brush_info(brush->GetInfo()));

	if (brush_window != NULL) {
		brush_window->Lock();
		if (((BrushStoreWindow*)brush_window)->store_view != NULL)
			((BrushStoreWindow*)brush_window)->store_view->AddBrush(brush);
		brush_window->Unlock();
	}
}

void BrushStoreWindow::DeleteBrush(int32 index)
{
	brush_info *data;
	data = (brush_info*)brush_data->RemoveItem(index);
	delete data;
}

void BrushStoreWindow::showWindow()
{
	if (brush_window == NULL) {
		new BrushStoreWindow();
		brush_window->Show();
	}
	else {
		brush_window->Lock();
		brush_window->SetWorkspaces(B_CURRENT_WORKSPACE);
		if (brush_window->IsHidden()) {
			brush_window->Show();
		}
		if (!brush_window->IsActive()) {
			brush_window->Activate(TRUE);
		}
		brush_window->Unlock();
	}

	BRect new_rect = FitRectToScreen(brush_window->Frame());
	brush_window->MoveTo(new_rect.LeftTop());
}

void BrushStoreWindow::setFeel(window_feel feel)
{
	((PaintApplication*)be_app)->GlobalSettings()->brush_window_feel = feel;
	if (brush_window != NULL) {
		brush_window->Lock();
		brush_window->SetFeel(feel);
		float total_height = brush_window->store_view->Bounds().Height();
		if (feel == B_NORMAL_WINDOW_FEEL) {
			brush_window->SetLook(B_DOCUMENT_WINDOW_LOOK);
			brush_window->scroll_bar->ResizeTo(B_V_SCROLL_BAR_WIDTH,total_height-B_H_SCROLL_BAR_HEIGHT+2);
		}
		else {
			brush_window->SetLook(B_FLOATING_WINDOW_LOOK);
			brush_window->scroll_bar->ResizeTo(B_V_SCROLL_BAR_WIDTH,total_height+2);
		}
		brush_window->Unlock();
	}
}


int32 BrushStoreWindow::brush_adder(void *data)
{
	BrushStoreWindow *this_pointer = (BrushStoreWindow*)data;

	if (this_pointer != NULL) {
		BList temp_list(*brush_data);
		if (temp_list.CountItems()> 0) {
			Brush *a_brush = new Brush(*(brush_info*)(temp_list.ItemAt(0)));

			for (int32 i=0;i<temp_list.CountItems();i++) {
				a_brush->ModifyBrush(*(brush_info*)(temp_list.ItemAt(i)));
				this_pointer->Lock();
				this_pointer->store_view->AddBrush(a_brush);
				this_pointer->Unlock();
			}

			delete a_brush;
		}
		return B_OK;
	}
	return B_ERROR;
}


BrushStoreView::BrushStoreView(BRect frame)
	:	BView(frame,"brush store view",B_FOLLOW_ALL,B_WILL_DRAW|B_FRAME_EVENTS)
{
	brush_images = new BList();
	brush_data = new BList();

	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	selected_brush_index = 0;
	previous_brush_index = 0;
}


BrushStoreView::~BrushStoreView()
{
	// Delete the brush-bitmaps from the list
	BBitmap *a_bitmap;
	while (brush_images->CountItems() > 0) {
		a_bitmap = (BBitmap*)brush_images->RemoveItem((int32)0);
		delete a_bitmap;
	}

	brush_info *info;
	while (brush_data->CountItems() > 0) {
		info = (brush_info*)brush_data->RemoveItem((int32)0);
	}

	delete brush_images;
	delete brush_data;
}

void BrushStoreView::DetachedFromWindow()
{
	BView *view = ScrollBar(B_VERTICAL);
	if (view != NULL) {
		view->RemoveSelf();
		delete view;
	}
}


void BrushStoreView::Draw(BRect area)
{
	SetHighColor(0,0,0,255);
	SetPenSize(1.0);
	for (int32 i=0;i<brush_images->CountItems();i++) {
		if ((area & get_bitmap_frame(i)).IsValid() == TRUE) {
			DrawBitmapAsync((BBitmap*)brush_images->ItemAt(i),get_bitmap_frame(i));
			BRect outer_frame = get_bitmap_frame(i);
			outer_frame.InsetBy(-1,-1);
			StrokeRect(outer_frame);
		}
	}
	if (IsFocus() && (brush_data->CountItems() > 0)) {
		BRect outer_frame = get_bitmap_frame(previous_brush_index);
		outer_frame.InsetBy(-1,-1);
		StrokeRect(outer_frame);

		SetHighColor(0,0,255,255);
		outer_frame = get_bitmap_frame(selected_brush_index);
		outer_frame.InsetBy(-1,-1);
		StrokeRect(outer_frame);
	}
	Sync();
}

void BrushStoreView::FrameResized(float width,float height)
{
	if (((int32)width/BRUSH_VAULT_WIDTH) != in_a_row) {
		Invalidate();
		in_a_row = (int32)width/BRUSH_VAULT_WIDTH;
	}
	// At least one clumn, no matter how small the window is.
	in_a_row = max_c(in_a_row,1);

	int32 rows = (brush_data->CountItems() - 1) / in_a_row + 1;

	BScrollBar *vertical_scrollbar = ScrollBar(B_VERTICAL);

	if (vertical_scrollbar != NULL) {
		if (rows*BRUSH_VAULT_HEIGHT>(height+1)) {
			vertical_scrollbar->SetRange(0,rows*BRUSH_VAULT_HEIGHT-height);
			vertical_scrollbar->SetProportion(height/(float)(rows*BRUSH_VAULT_HEIGHT));
		}
		else
			vertical_scrollbar->SetRange(0,0);
	}
//	float needed_width = in_a_row * BRUSH_VAULT_WIDTH;
//	if (needed_width != width)
//		Window()->ResizeBy(needed_width-width,0);
}

void BrushStoreView::MessageReceived(BMessage *message)
{
	brush_info *info;
	Brush *a_brush;
	int32 size;
	switch (message->what) {
		case HS_BRUSH_DRAGGED:
			if ((message->ReturnAddress() == BMessenger(this)) == FALSE) {
				message->FindData("brush data",B_ANY_TYPE,(const void**)&info,&size);
				if (size == sizeof(brush_info)) {
					a_brush = new Brush(*info);
//					AddBrush(a_brush);
					BrushStoreWindow::AddBrush(a_brush);
					delete a_brush;
				}
			}
			else {
			}
			break;
		case HS_DELETE_SELECTED_BRUSH:
			{
				// In this case we delete the currently selected brush
				// from the brush-list.
				BBitmap *bitmap = (BBitmap*)brush_images->RemoveItem(selected_brush_index);
				delete bitmap;
				brush_info *data = (brush_info*)brush_data->RemoveItem(selected_brush_index);
				delete data;

				BrushStoreWindow::DeleteBrush(selected_brush_index);
				if (brush_data->CountItems() <= selected_brush_index)
					selected_brush_index--;

				ResizeBy(-1,0);
				ResizeBy(1,0);
				Invalidate();
				break;
			}
		default:
			BView::MessageReceived(message);
			break;
	}
}

void BrushStoreView::KeyDown(const char *bytes,int32 numBytes)
{
	if (numBytes == 1) {
		switch (bytes[0]) {
			case B_DELETE:
				{
					// In this case we delete the currently selected brush
					// from the brush-list.
					BBitmap *bitmap = (BBitmap*)brush_images->RemoveItem(selected_brush_index);
					delete bitmap;
					brush_info *data = (brush_info*)brush_data->RemoveItem(selected_brush_index);
					delete data;

					BrushStoreWindow::DeleteBrush(selected_brush_index);
					if (brush_data->CountItems() <= selected_brush_index)
						selected_brush_index--;

					ResizeBy(-1,0);
					ResizeBy(1,0);
					Invalidate();
					break;
				}
			case B_LEFT_ARROW:
				previous_brush_index = selected_brush_index;
				if (selected_brush_index == 0)
					selected_brush_index = brush_data->CountItems() - 1;
				else
					selected_brush_index = (selected_brush_index - 1) % brush_data->CountItems();
				Draw(BRect(BPoint(0,0),BPoint(-1,-1)));
				break;
			case B_RIGHT_ARROW:
				previous_brush_index = selected_brush_index;
				selected_brush_index = (selected_brush_index + 1) % brush_data->CountItems();
				Draw(BRect(BPoint(0,0),BPoint(-1,-1)));
				break;
			case B_UP_ARROW:
//				previous_brush_index = selected_brush_index;
//				selected_brush_index = (selected_brush_index - in_a_row);
//				if (selected_brush_index < 0)
//					selected_brush_index = brush_data->CountItems() + selected_brush_index;
//				 % brush_data->CountItems();
//				Draw(BRect(BPoint(0,0),BPoint(-1,-1)));
				break;
			case B_DOWN_ARROW:
//				previous_brush_index = selected_brush_index;
//				selected_brush_index = (selected_brush_index + in_a_row) % brush_data->CountItems();
//				Draw(BRect(BPoint(0,0),BPoint(-1,-1)));
				break;

			default:
				BView::KeyDown(bytes,numBytes);
				break;
		}
	}
}

void BrushStoreView::MouseDown(BPoint point)
{
	if (!IsFocus())
		MakeFocus(TRUE);

	uint32 buttons;
	GetMouse(&point,&buttons);
	BPoint original_point = point;

	// Loop here until the button is released or user moves mouse enough to
	// start a drag.
//	while (buttons && cont) {
//		GetMouse(&point,&buttons);
//		snooze(20.0 * 1000.0);
//		if (sqrt(pow(original_point.x-point.x,2)+pow(original_point.y-point.y,2)) > 6)
//			cont = FALSE;
//	}
	int32 index = get_point_index(original_point);

	if (index >= 0) {
//		if (sqrt(pow(original_point.x-point.x,2)+pow(original_point.y-point.y,2)) > 6) {
//			// In this case we drag a message.
//			BMessage *a_message = new BMessage(HS_BRUSH_DRAGGED);
//			a_message->AddData("brush data",B_ANY_TYPE,brush_data->ItemAt(index),sizeof(brush_info));
//			DragMessage(a_message,get_bitmap_frame(index));
//			delete a_message;
//		}
//		else {
			// In this case we select the brush to be used with the brush tool.
			// As we only have one brush tool, we should select the brush to be
			// used no matter what button was pressed.
//			uint32 tool_type = ((PaintApplication*)be_app)->getTool(original_button);
//			DrawingTool *a_tool = ((PaintApplication*)be_app)->getDrawingTool(tool_type);
//			const DrawingTool *a_tool = tool_manager->ReturnTool(BRUSH_TOOL);
//			const BrushTool *brush_tool = cast_as(a_tool,const BrushTool);
//			if (brush_tool) {
//				Brush *a_brush = brush_tool->GetBrush();
//				a_brush->ModifyBrush(*((brush_info*)brush_data->ItemAt(index)));
//				BrushEditor::BrushModified();
//				a_brush->CreateDiffBrushes();
//				ToolSetupWindow::changeToolForTheSetupWindow(BRUSH_TOOL);
////				ToolSetupWindow::updateTool(BRUSH_TOOL);
//
//			}
			tool_manager->SetCurrentBrush(((brush_info*)brush_data->ItemAt(index)));
			tool_manager->ChangeTool(BRUSH_TOOL);
//		}
		if (index != selected_brush_index) {
			// If the selected brush changed, we must indicate it
			// by drawing the view.
			previous_brush_index = selected_brush_index;
			selected_brush_index = index;
			Draw(BRect(BPoint(0,0),BPoint(-1,-1)));
		}
	}
}


void BrushStoreView::AddBrush(Brush *brush)
{
	BBitmap *a_bitmap = new BBitmap(BRect(0,0,BRUSH_PREVIEW_WIDTH-1,BRUSH_PREVIEW_HEIGHT-1),B_RGB_32_BIT);

	brush->PreviewBrush(a_bitmap);
	brush_images->AddItem(a_bitmap);
	brush_data->AddItem(new brush_info(brush->GetInfo()));

	ResizeBy(-1,0);
	ResizeBy(1,0);
	Invalidate();
}

BRect BrushStoreView::get_bitmap_frame(int32 index)
{
	// First we get the info, how many brushes can be in a row.
	int32 width = Bounds().IntegerWidth();
	in_a_row = max_c(width/BRUSH_VAULT_WIDTH,1);
	int32 row_number = index/in_a_row;
	int32 column_number = index-row_number*in_a_row;

	BRect frame = BRect(column_number*BRUSH_VAULT_WIDTH+BRUSH_INSET,row_number*BRUSH_VAULT_HEIGHT+BRUSH_INSET,(column_number+1)*BRUSH_VAULT_WIDTH-1-BRUSH_INSET,(row_number+1)*BRUSH_VAULT_HEIGHT-1-BRUSH_INSET);
	return frame;
}


int32 BrushStoreView::get_point_index(BPoint point)
{
	// First we get the info, how many brushes can be in a row.
	int32 width = Bounds().IntegerWidth();
	in_a_row = width/BRUSH_VAULT_WIDTH;

	// Then we check the row-number
	int32 row_number = ((int32)point.y)/BRUSH_VAULT_HEIGHT;

	int32 index = -1;
	if (point.y >= 0) {
		index = row_number*in_a_row;
		if (point.x >=0) {
			index += (int32)point.x / BRUSH_VAULT_WIDTH;
			if (index > brush_images->CountItems()-1) {
				index = -1;
			}
		}
	}
	return index;
}
