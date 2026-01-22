/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "BrushStoreWindow.h"

#include "Brush.h"
#include "BrushEditor.h"
#include "DrawingTools.h"
#include "FloaterManager.h"
#include "MessageFilters.h"
#include "SettingsServer.h"
#include "ToolManager.h"
#include "ToolSetupWindow.h"
#include "UtilityClasses.h"


#include <Bitmap.h>
#include <Catalog.h>
#include <File.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <ScrollBar.h>
#include <ScrollView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Windows"


#define BRUSH_INSET 3
#define BRUSH_VAULT_WIDTH (BRUSH_PREVIEW_WIDTH + 2 * BRUSH_INSET)
#define BRUSH_VAULT_HEIGHT (BRUSH_PREVIEW_WIDTH + 2 * BRUSH_INSET)


BList* BrushStoreWindow::brush_data = new BList();
BrushStoreWindow* BrushStoreWindow::brush_window = NULL;


BrushStoreWindow::BrushStoreWindow()
	:
	BWindow(BRect(20, 20, 220, 220), B_TRANSLATE("Brushes"), B_TITLED_WINDOW_LOOK,
		B_NORMAL_WINDOW_FEEL, B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{
	BRect frame(20, 20, 220, 220);
	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		settings.FindRect(skBrushWindowFrame, &frame);
		settings.FindInt32(skBrushWindowFeel, (int32*)&feel);
		server->SetValue(SettingsServer::Application, skBrushWindowVisible, true);
	}
	frame = FitRectToScreen(frame);
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(), frame.Height());

	BMenuBar* menu_bar;
	menu_bar = new BMenuBar("brush window menu-bar");
	BMenu* a_menu = new BMenu(B_TRANSLATE("Brush"));
	menu_bar->AddItem(a_menu);
	a_menu->AddItem(new BMenuItem(
		B_TRANSLATE("Delete selected brush"), new BMessage(HS_DELETE_SELECTED_BRUSH)));

	store_view = new BrushStoreView();
	BScrollView* scroll_view
		= new BScrollView("scroller", store_view, B_FRAME_EVENTS, false, true, B_PLAIN_BORDER);

	BView* brush_editor = BrushEditor::CreateBrushEditor(ToolManager::Instance().GetCurrentBrush());

	BGroupLayout* mainLayout = BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menu_bar, 0.0)
		.AddGroup(B_VERTICAL)
			.Add(brush_editor)
			.Add(scroll_view)
		.End()
		.SetInsets(-1, 0, -1, -1);

	a_menu->FindItem(HS_DELETE_SELECTED_BRUSH)->SetTarget(store_view);

	store_view->MakeFocus(TRUE);

	// The adding of brushes will be done in a separate thread.
	thread_id adder_thread = spawn_thread(brush_adder, "brush_adder", B_NORMAL_PRIORITY, this);
	resume_thread(adder_thread);

	brush_window = this;

	setFeel(feel);

	float scroll_width = scroll_view->ScrollBar(B_VERTICAL)->Bounds().Width();
	scroll_view->SetExplicitMinSize(BSize(BRUSH_VAULT_WIDTH * 3 + scroll_width + 2, B_SIZE_UNSET));

	BFont font;
	font_height fHeight;
	font.GetHeight(&fHeight);
	float brush_editor_height = (fHeight.ascent + fHeight.descent + fHeight.leading) * 15;
	SetSizeLimits(BRUSH_VAULT_WIDTH * 3 + scroll_width + 2, 1000,
		brush_editor_height + BRUSH_VAULT_HEIGHT * 2.5 + 2, 1000);

	// Add a filter that will be used to catch mouse-down-messages in order
	// to activate this window when required
	Lock();
	BMessageFilter* activation_filter
		= new BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, B_MOUSE_DOWN, window_activation_filter);
	AddCommonFilter(activation_filter);
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
	Unlock();

	FloaterManager::AddFloater(this);
}


BrushStoreWindow::~BrushStoreWindow()
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skBrushWindowFrame, Frame());
		server->SetValue(SettingsServer::Application, skBrushWindowVisible, false);
	}

	FloaterManager::RemoveFloater(this);
	brush_window = NULL;
}


void
BrushStoreWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BWindow::MessageReceived(message);
	}
}


void
BrushStoreWindow::writeBrushes(BFile& file)
{
	int32 number_of_brushes = brush_data->CountItems();
	file.Write(&number_of_brushes, sizeof(int32));
	for (int32 i = 0; i < brush_data->CountItems(); i++)
		file.Write((brush_data->ItemAt(i)), sizeof(brush_info));
}


void
BrushStoreWindow::readBrushes(BFile& file)
{
	int32 number_of_brushes;
	brush_info info;
	if (file.Read(&number_of_brushes, sizeof(int32)) == sizeof(int32)) {
		for (int32 i = 0; i < number_of_brushes; i++) {
			if (file.Read(&info, sizeof(brush_info)) == sizeof(brush_info))
				brush_data->AddItem(new brush_info(info));
		}
	}
}


void
BrushStoreWindow::AddBrush(Brush* brush)
{
	bool added = false;

	if (brush_window != NULL) {
		brush_window->Lock();
		if (((BrushStoreWindow*)brush_window)->store_view != NULL)
			added = ((BrushStoreWindow*)brush_window)->store_view->AddBrush(brush);
		brush_window->Unlock();
	}

	if (added == true)
		brush_data->AddItem(new brush_info(brush->GetInfo()));
}


void
BrushStoreWindow::DeleteBrush(int32 index)
{
	brush_info* data;
	data = (brush_info*)brush_data->RemoveItem(index);
	delete data;
}


void
BrushStoreWindow::showWindow()
{
	if (brush_window == NULL) {
		new BrushStoreWindow();
		brush_window->Show();
	} else {
		brush_window->Lock();
		brush_window->SetWorkspaces(B_CURRENT_WORKSPACE);
		if (brush_window->IsHidden())
			brush_window->Show();

		if (!brush_window->IsActive())
			brush_window->Activate(TRUE);

		brush_window->Unlock();
	}

	BRect new_rect = FitRectToScreen(brush_window->Frame());
	brush_window->MoveTo(new_rect.LeftTop());
}


void
BrushStoreWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance())
		server->SetValue(SettingsServer::Application, skBrushWindowFeel, int32(feel));

	if (brush_window != NULL) {
		brush_window->Lock();
		brush_window->SetFeel(feel);
		if (feel == B_NORMAL_WINDOW_FEEL)
			brush_window->SetLook(B_TITLED_WINDOW_LOOK);
		else
			brush_window->SetLook(B_FLOATING_WINDOW_LOOK);

		brush_window->Unlock();
	}
}


int32
BrushStoreWindow::brush_adder(void* data)
{
	BrushStoreWindow* this_pointer = (BrushStoreWindow*)data;

	if (this_pointer != NULL) {
		this_pointer->Lock();
		this_pointer->BeginViewTransaction();

		BList temp_list(*brush_data);
		if (temp_list.CountItems() > 0) {
			Brush* a_brush = new Brush(*(brush_info*)(temp_list.ItemAt(0)));

			for (int32 i = 0; i < temp_list.CountItems(); i++) {
				a_brush->ModifyBrush(*(brush_info*)(temp_list.ItemAt(i)), false);
				this_pointer->store_view->AddBrush(a_brush, false);
			}

			delete a_brush;
		}
		this_pointer->EndViewTransaction();
		this_pointer->Unlock();
		return B_OK;
	}
	return B_ERROR;
}


BrushStoreView::BrushStoreView()
	:
	BView("brush store view", B_WILL_DRAW | B_FRAME_EVENTS)
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
	BBitmap* a_bitmap;
	while (brush_images->CountItems() > 0) {
		a_bitmap = (BBitmap*)brush_images->RemoveItem((int32)0);
		delete a_bitmap;
	}

	brush_info* info;
	while (brush_data->CountItems() > 0) {
		info = (brush_info*)brush_data->RemoveItem((int32)0);
	}

	delete brush_images;
	delete brush_data;
}


void
BrushStoreView::DetachedFromWindow()
{
	BView* view = ScrollBar(B_VERTICAL);
	if (view != NULL) {
		view->RemoveSelf();
		delete view;
	}
}


void
BrushStoreView::Draw(BRect area)
{
	SetHighColor(0, 0, 0, 255);
	SetPenSize(1.0);
	for (int32 i = 0; i < brush_images->CountItems(); i++) {
		if ((area & get_bitmap_frame(i)).IsValid() == TRUE) {
			SetHighColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			BRect outer_frame = get_bitmap_frame(i);
			DrawBitmap((BBitmap*)brush_images->ItemAt(i), get_bitmap_frame(i));
			StrokeRect(outer_frame.InsetByCopy(-1, -1));
			StrokeRect(outer_frame.InsetByCopy(-2, -2));
		}
	}
	if (IsFocus() && (brush_data->CountItems() > 0)) {
		SetHighColor(ui_color(B_NAVIGATION_BASE_COLOR));
		BRect outer_frame = get_bitmap_frame(selected_brush_index);
		SetPenSize(1.5);
		StrokeRect(outer_frame.InsetByCopy(-1, -1));
	}
	Sync();
}


void
BrushStoreView::FrameResized(float width, float height)
{
	if (((int32)width / BRUSH_VAULT_WIDTH) != in_a_row) {
		Invalidate();
		in_a_row = (int32)width / BRUSH_VAULT_WIDTH;
	}
	// At least one clumn, no matter how small the window is.
	in_a_row = max_c(in_a_row, 1);

	int32 rows = (brush_data->CountItems() - 1) / in_a_row + 1;

	BScrollBar* vertical_scrollbar = ScrollBar(B_VERTICAL);

	if (vertical_scrollbar != NULL) {
		if (rows * BRUSH_VAULT_HEIGHT > (height + 1)) {
			vertical_scrollbar->SetRange(0, rows * BRUSH_VAULT_HEIGHT - height);
			vertical_scrollbar->SetProportion(height / (float)(rows * BRUSH_VAULT_HEIGHT));
		} else
			vertical_scrollbar->SetRange(0, 0);
	}
}


void
BrushStoreView::MessageReceived(BMessage* message)
{
	brush_info* info;
	Brush* a_brush;
	ssize_t size;
	switch (message->what) {
		case HS_BRUSH_DRAGGED:
		{
			if ((message->ReturnAddress() == BMessenger(this)) == FALSE) {
				message->FindData("brush data", B_ANY_TYPE, (const void**)&info, &size);
				if (size == sizeof(brush_info)) {
					a_brush = new Brush(*info);
					BrushStoreWindow::AddBrush(a_brush);
					delete a_brush;
				}
			}
		} break;
		case HS_DELETE_SELECTED_BRUSH:
		{
			// In this case we delete the currently selected brush
			// from the brush-list.
			BBitmap* bitmap = (BBitmap*)brush_images->RemoveItem(selected_brush_index);
			delete bitmap;
			brush_info* data = (brush_info*)brush_data->RemoveItem(selected_brush_index);
			delete data;

			BrushStoreWindow::DeleteBrush(selected_brush_index);
			if (brush_data->CountItems() <= selected_brush_index)
				selected_brush_index--;

			ResizeBy(-1, 0);
			ResizeBy(1, 0);
			Invalidate();
			if (selected_brush_index >= 0)
				ToolManager::Instance().SetCurrentBrush(
					((brush_info*)brush_data->ItemAt(selected_brush_index)));

			Draw(Bounds());
		} break;
		default:
			BView::MessageReceived(message);
	}
}


void
BrushStoreView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch (bytes[0]) {
			case B_DELETE:
			{
				// In this case we delete the currently selected brush
				// from the brush-list.
				BBitmap* bitmap = (BBitmap*)brush_images->RemoveItem(selected_brush_index);
				delete bitmap;
				brush_info* data = (brush_info*)brush_data->RemoveItem(selected_brush_index);
				delete data;

				BrushStoreWindow::DeleteBrush(selected_brush_index);
				if (brush_data->CountItems() <= selected_brush_index)
					selected_brush_index--;

				ResizeBy(-1, 0);
				ResizeBy(1, 0);
				Invalidate();
				ToolManager::Instance().SetCurrentBrush(
					((brush_info*)brush_data->ItemAt(selected_brush_index)));
				Draw(Bounds());
			} break;
			case B_LEFT_ARROW:
			{
				previous_brush_index = selected_brush_index;
				if (selected_brush_index == 0)
					selected_brush_index = brush_data->CountItems() - 1;
				else
					selected_brush_index = (selected_brush_index - 1) % brush_data->CountItems();
				ToolManager::Instance().SetCurrentBrush(
					((brush_info*)brush_data->ItemAt(selected_brush_index)));
				Draw(Bounds());
			} break;
			case B_RIGHT_ARROW:
			{
				previous_brush_index = selected_brush_index;
				selected_brush_index = (selected_brush_index + 1) % brush_data->CountItems();
				ToolManager::Instance().SetCurrentBrush(
					((brush_info*)brush_data->ItemAt(selected_brush_index)));
				Draw(Bounds());
			} break;
			case B_UP_ARROW:
				break;
			case B_DOWN_ARROW:
				break;

			default:
				BView::KeyDown(bytes, numBytes);
		}
	}
}


void
BrushStoreView::MouseDown(BPoint point)
{
	if (!IsFocus())
		MakeFocus(TRUE);

	uint32 buttons;
	GetMouse(&point, &buttons);
	BPoint original_point = point;

	int32 index = get_point_index(original_point);

	if (index >= 0) {
		ToolManager::Instance().SetCurrentBrush(((brush_info*)brush_data->ItemAt(index)));
		if (index != selected_brush_index) {
			// If the selected brush changed, we must indicate it
			// by drawing the view.
			previous_brush_index = selected_brush_index;
			selected_brush_index = index;
			Draw(Bounds());
		}
	}
}


bool
BrushStoreView::AddBrush(Brush* brush, bool notify)
{
	bool added = false;

	BBitmap* a_bitmap
		= new BBitmap(BRect(0, 0, BRUSH_PREVIEW_WIDTH, BRUSH_PREVIEW_HEIGHT), B_RGB_32_BIT, true);

	brush_info new_brush_info = brush->GetInfo();

	int32 index = -1;

	for (int i = 0; i < brush_data->CountItems(); ++i) {
		brush_info item = *(brush_info*)brush_data->ItemAt(i);
		if (Brush::compare_brushes(item, new_brush_info) == true) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		index = brush_data->CountItems();
		previous_brush_index = selected_brush_index;
		selected_brush_index = index;
		brush->PreviewBrush(a_bitmap);
		brush_images->AddItem(a_bitmap);
		brush_data->AddItem(new brush_info(new_brush_info));
		added = true;
	} else {
		delete a_bitmap;
		previous_brush_index = selected_brush_index;
		selected_brush_index = index;
		Draw(Bounds());
	}

	ToolManager::Instance().SetCurrentBrush(((brush_info*)brush_data->ItemAt(index)), notify);

	ResizeBy(-1, 0);
	ResizeBy(1, 0);
	Invalidate();

	return added;
}


BRect
BrushStoreView::get_bitmap_frame(int32 index)
{
	// First we get the info, how many brushes can be in a row.
	int32 width = Bounds().IntegerWidth();
	in_a_row = max_c(width / BRUSH_VAULT_WIDTH, 1);
	int32 row_number = index / in_a_row;
	int32 column_number = index - row_number * in_a_row;

	BRect frame = BRect(column_number * BRUSH_VAULT_WIDTH + BRUSH_INSET,
		row_number * BRUSH_VAULT_HEIGHT + BRUSH_INSET,
		(column_number + 1) * BRUSH_VAULT_WIDTH - BRUSH_INSET,
		(row_number + 1) * BRUSH_VAULT_HEIGHT - BRUSH_INSET);
	return frame;
}


int32
BrushStoreView::get_point_index(BPoint point)
{
	// First we get the info, how many brushes can be in a row.
	int32 width = Bounds().IntegerWidth();
	in_a_row = width / BRUSH_VAULT_WIDTH;

	// Then we check the row-number
	int32 row_number = ((int32)point.y) / BRUSH_VAULT_HEIGHT;

	int32 index = -1;
	if (point.y >= 0) {
		index = row_number * in_a_row;
		if (point.x >= 0) {
			index += (int32)point.x / BRUSH_VAULT_WIDTH;
			if (index > brush_images->CountItems() - 1) {
				index = -1;
			}
		}
	}
	return index;
}
