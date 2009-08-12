/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "LayerView.h"

#include "ImageView.h"
#include "StringServer.h"
#include "UtilityClasses.h"


#include <Bitmap.h>
#include <CheckBox.h>
#include <InterfaceDefs.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <Window.h>


LayerView::LayerView(BBitmap *image,Layer *layer)
	:	BBox(BRect(0,0,120,LAYER_VIEW_HEIGHT-1),"a layer view",B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW | B_NAVIGABLE_JUMP | B_FRAME_EVENTS, B_NO_BORDER)
{
	the_image = image;
	the_layer = layer;

	BMessage a_message;
	a_message.AddInt32("layer_id",the_layer->Id());
	a_message.AddPointer("layer_pointer",(void*)the_layer);
	a_message.what = HS_LAYER_VISIBILITY_CHANGED;
	visibility_box = new BCheckBox(BRect(80,5,140,30),"visibility check box",StringServer::ReturnString(VISIBLE_STRING),new BMessage(a_message));

	BRect rect = visibility_box->Frame();
	rect.OffsetBy(0,rect.Height()+4);

//	The target should be set later, when we know that image-view is alrady in its window.
	ResizeBy(visibility_box->Frame().right - 120+5,0);
	AddChild(visibility_box);


	layer_operation_pop_up_menu = new BPopUpMenu("layer operation menu");


	a_message.what = HS_MERGE_WITH_UPPER_LAYER;
	layer_operation_pop_up_menu->AddItem(new BMenuItem(StringServer::ReturnString(MERGE_WITH_FRONT_LAYER_STRING),new BMessage(a_message)));

	a_message.what = HS_MERGE_WITH_LOWER_LAYER;
	layer_operation_pop_up_menu->AddItem(new BMenuItem(StringServer::ReturnString(MERGE_WITH_BACK_LAYER_STRING),new BMessage(a_message)));

	layer_operation_pop_up_menu->AddSeparatorItem();

	a_message.what = HS_ADD_LAYER_FRONT;
	layer_operation_pop_up_menu->AddItem(new BMenuItem(StringServer::ReturnString(ADD_LAYER_IN_FRONT_STRING),new BMessage(a_message)));

	a_message.what = HS_ADD_LAYER_BEHIND;
	layer_operation_pop_up_menu->AddItem(new BMenuItem(StringServer::ReturnString(ADD_LAYER_BEHIND_STRING),new BMessage(a_message)));

	layer_operation_pop_up_menu->AddSeparatorItem();

	a_message.what = HS_DUPLICATE_LAYER;
	layer_operation_pop_up_menu->AddItem(new BMenuItem(StringServer::ReturnString(DUPLICATE_LAYER_STRING),new BMessage(a_message)));

	layer_operation_pop_up_menu->AddSeparatorItem();

	a_message.what = HS_DELETE_LAYER;
	layer_operation_pop_up_menu->AddItem(new BMenuItem(StringServer::ReturnString(DELETE_LAYER_STRING),new BMessage(a_message)));

	layer_operation_pop_up_menu->SetRadioMode(false);

	popUpMenuField = new BMenuField(BRect(0,0,0,0),"popUpMenuField","",layer_operation_pop_up_menu);
	popUpMenuField->ResizeToPreferred();
	popUpMenuField->MoveTo(visibility_box->Frame().left,visibility_box->Frame().bottom+4);
	popUpMenuField->MenuItem()->SetLabel("");
	AddChild(popUpMenuField);
}


LayerView::~LayerView()
{
	BWindow *owner_window = Window();
	if (owner_window != NULL) {
		owner_window->Lock();
		RemoveSelf();
		owner_window->Unlock();
	}
}


void LayerView::AttachedToWindow()
{
	visibility_box->SetTarget(the_layer->GetImageView());
	if ((Parent() != NULL) && (is_active == FALSE))
		SetViewColor(Parent()->ViewColor());

//	layer_name_field->SetTarget(this);
}

void LayerView::Draw(BRect area)
{
	DrawBitmap(the_image,BPoint(5,5));
	if (is_active == TRUE) {
		BRect a_rect = the_image->Bounds();
		a_rect.InsetBy(-1,-1);
		a_rect.OffsetTo(4,4);
		SetHighColor(0,0,255,255);
		StrokeRect(a_rect);
	}
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_3_TINT));
	StrokeLine(Bounds().LeftBottom(),Bounds().RightBottom());
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_LIGHTEN_2_TINT));
	StrokeLine(Bounds().LeftTop(),Bounds().RightTop());
	BView::Draw(area);
}

void LayerView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case HS_LAYER_NAME_CHANGED:
			the_layer->SetName(layer_name_field->Text());
			break;

		default:
			BBox::MessageReceived(message);
			break;
	}
}

void LayerView::MouseDown(BPoint location)
{
	BView *image_view = the_layer->GetImageView();
	BWindow *image_window = image_view->Window();
	BMessage a_message;
	a_message.AddInt32("layer_id",the_layer->Id());
	a_message.AddPointer("layer_pointer",(void*)the_layer);

	uint32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons",(int32*)&buttons);
	BRect mini_image_frame = BRect(5,5,HS_MINIATURE_IMAGE_WIDTH+5,HS_MINIATURE_IMAGE_HEIGHT+5);

	if (image_window != NULL) {
		a_message.what = HS_LAYER_ACTIVATED;
		image_window->PostMessage(&a_message,image_view);
	}
	// Here initiate a drag-session. If mouse was over the image, whole layer should
	// be dragged. Otherwise we drag this view in it's parent to reorder the layers.
	if (mini_image_frame.Contains(location)) {
		// This should start a real drag'n'drop session
		a_message.what = HS_LAYER_DRAGGED;
		a_message.AddPointer("layer_bitmap",(void*)the_layer->Bitmap());
		BBitmap *layer_mini_image = new BBitmap(the_layer->GetMiniatureImage());
		DragMessage(&a_message,layer_mini_image,B_OP_ALPHA,location-BPoint(5,5));
	}
	else {
		// We start reordering the layers.
		thread_id reorder = spawn_thread(LayerView::reorder_thread,"reorder layers",B_NORMAL_PRIORITY,(void*)this);
		resume_thread(reorder);
	}
}



void LayerView::MouseMoved(BPoint where,uint32 transit,const BMessage*)
{
	if (transit == B_EXITED_VIEW) {
		if (popUpMenuField->Frame().Contains(where)) {
			int32 number_of_null = 0;
			if (the_layer->ReturnUpperLayer() == NULL) {
				layer_operation_pop_up_menu->FindItem(HS_MERGE_WITH_UPPER_LAYER)->SetEnabled(FALSE);
				number_of_null++;
			}
			else {
				layer_operation_pop_up_menu->FindItem(HS_MERGE_WITH_UPPER_LAYER)->SetEnabled(TRUE);
			}

			if (the_layer->ReturnLowerLayer() == NULL) {
				layer_operation_pop_up_menu->FindItem(HS_MERGE_WITH_LOWER_LAYER)->SetEnabled(FALSE);
				number_of_null++;
			}
			else {
				layer_operation_pop_up_menu->FindItem(HS_MERGE_WITH_LOWER_LAYER)->SetEnabled(TRUE);
			}

			if (number_of_null == 2) {
				layer_operation_pop_up_menu->FindItem(HS_DELETE_LAYER)->SetEnabled(FALSE);
			}
			else {
				layer_operation_pop_up_menu->FindItem(HS_DELETE_LAYER)->SetEnabled(TRUE);
			}

			BView *image_view = the_layer->GetImageView();
			BWindow *image_window = image_view->Window();

			popUpMenuField->Menu()->SetTargetForItems(BMessenger(image_view,image_window));
		}
	}
}


void LayerView::UpdateImage()
{
	DrawBitmap(the_image,BPoint(5,5));
}


void LayerView::Activate(bool active)
{
	is_active = active;
	if (LockLooper() == TRUE) {
		if (active == TRUE) {
			SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_1_TINT));
		}
		else if (Parent() != NULL) {
			SetViewColor(Parent()->ViewColor());
		}

		visibility_box->SetViewColor(ViewColor());
		visibility_box->Invalidate();

		popUpMenuField->SetViewColor(ViewColor());
		popUpMenuField->Invalidate();

		Invalidate();
		UnlockLooper();

	}
	else {
		if (active == TRUE) {
			SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR),B_DARKEN_1_TINT));
		}
		else if (Parent() != NULL)
			SetViewColor(Parent()->ViewColor());

		visibility_box->SetViewColor(ViewColor());

		popUpMenuField->SetViewColor(ViewColor());
	}
}

void LayerView::SetVisibility(bool visible)
{
	BWindow *a_window = visibility_box->Window();
	if (a_window != NULL)
		a_window->Lock();

	if (visible == TRUE)
		visibility_box->SetValue(B_CONTROL_ON);
	else
		visibility_box->SetValue(B_CONTROL_OFF);

	if (a_window != NULL)
		a_window->Unlock();
}



int32 LayerView::reorder_thread(void *data)
{
	LayerView *this_pointer = (LayerView*)data;
	return this_pointer->ReorderViews();
}


int32 LayerView::ReorderViews()
{
	uint32 buttons;
	BPoint location;
	BRect frame;

	BWindow *the_window = Window();

	int32 positions_moved = 0;
	if (the_window != NULL) {
		the_window->Lock();
		BView *parent_view = Parent();
		BView *exchanged_view;
		GetMouse(&location,&buttons);
		the_window->Unlock();

		if (parent_view != NULL) {
			while (buttons) {
				the_window->Lock();
				location = ConvertToParent(location);
				frame = ConvertToParent(Bounds());
				frame.OffsetBy(0,-LAYER_VIEW_HEIGHT);
				if (frame.Contains(location) == TRUE) {
					exchanged_view = the_window->FindView(the_window->ConvertFromScreen(parent_view->ConvertToScreen(location)));
					if (exchanged_view != NULL) {
						if (exchanged_view->Parent() == parent_view) {
							exchanged_view->MoveBy(0,LAYER_VIEW_HEIGHT);
							MoveBy(0,-LAYER_VIEW_HEIGHT);
							positions_moved++;
							if (parent_view->Bounds().Contains(Frame().LeftTop()) == FALSE) {
								parent_view->ScrollBy(0,Frame().top);
							}
						}
					}
				}
				else {
					frame.OffsetBy(0,2*LAYER_VIEW_HEIGHT);
					if (frame.Contains(location) == TRUE) {
						exchanged_view = the_window->FindView(the_window->ConvertFromScreen(parent_view->ConvertToScreen(location)));
						if (exchanged_view != NULL) {
							if (exchanged_view->Parent() == parent_view) {
								exchanged_view->MoveBy(0,-LAYER_VIEW_HEIGHT);
								MoveBy(0,LAYER_VIEW_HEIGHT);
								positions_moved--;
								if (parent_view->Bounds().Contains(Frame().LeftBottom()) == FALSE) {
									parent_view->ScrollBy(0,Frame().bottom-parent_view->Bounds().bottom);
								}
							}
						}
					}
				}
				GetMouse(&location,&buttons);
				the_window->Unlock();

				snooze(20 * 1000);
			}
		}
	}
	if (positions_moved != 0) {
		BView *image_view = the_layer->GetImageView();
		BWindow *image_window = image_view->Window();
		BMessage a_message;
		a_message.what = HS_LAYER_POSITION_CHANGED;
		a_message.AddInt32("layer_id",the_layer->Id());
		a_message.AddPointer("layer_pointer",(void*)the_layer);
		a_message.AddInt32("positions_moved",positions_moved);
		image_window->PostMessage(&a_message,image_view);
	}

	return positions_moved;
}
