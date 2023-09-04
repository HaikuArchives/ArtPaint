/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "LayerView.h"

#include "CustomGridLayout.h"
#include "ImageView.h"
#include "LayerWindow.h"
#include "UtilityClasses.h"


#include <Bitmap.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayout.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextControl.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LayerView"


LayerView::LayerView(BBitmap* image, Layer* layer)
	:
	BBox("a layer view", B_WILL_DRAW | B_NAVIGABLE_JUMP | B_FRAME_EVENTS, B_NO_BORDER)
{
	the_layer = layer;

	BMessage a_message;
	a_message.AddInt32("layer_id", the_layer->Id());
	a_message.AddPointer("layer_pointer", (void*)the_layer);
	a_message.what = HS_LAYER_VISIBILITY_CHANGED;
	visibility_box = new BCheckBox("visibility check box", "", new BMessage(a_message));

	thumbnail_view = new ThumbnailView(image);
	thumbnail_view->SetEventMask(0);

	a_message.what = HS_LAYER_NAME_CHANGED;
	layer_name_field = new BTextControl("", "Layer", new BMessage(a_message));

	BGroupLayout* layerLayout = BLayoutBuilder::Group<>(this, B_HORIZONTAL, B_USE_SMALL_SPACING)
		.Add(visibility_box)
		.Add(thumbnail_view)
		.Add(layer_name_field)
		.SetInsets(B_USE_SMALL_INSETS, B_USE_SMALL_INSETS, B_USE_SMALL_INSETS, B_USE_SMALL_INSETS);
}


LayerView::~LayerView()
{
	BWindow* owner_window = Window();
	if (owner_window != NULL) {
		owner_window->Lock();
		RemoveSelf();
		owner_window->Unlock();
	}
}


void
LayerView::AttachedToWindow()
{
	visibility_box->SetTarget(the_layer->GetImageView());
	if ((Parent() != NULL) && (is_active == FALSE))
		SetViewColor(Parent()->ViewColor());

	layer_name_field->SetTarget(the_layer->GetImageView());
	layer_name_field->SetText(the_layer->ReturnLayerName());
}


void
LayerView::Draw(BRect area)
{
	thumbnail_view->Redraw();
	if (is_active == TRUE) {
		BRect a_rect = thumbnail_view->Frame();
		SetHighColor(ui_color(B_CONTROL_HIGHLIGHT_COLOR));
		StrokeRect(a_rect);
	}
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_3_TINT));
	StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
	SetHighColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
	StrokeLine(Bounds().LeftTop(), Bounds().RightTop());
	BView::Draw(area);
}


void
LayerView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BBox::MessageReceived(message);
	}
}


void
LayerView::MouseDown(BPoint location)
{
	BView* image_view = the_layer->GetImageView();
	BWindow* image_window = image_view->Window();
	LayerWindow* layer_window = (LayerWindow*)Window();

	BMessage a_message;
	a_message.AddInt32("layer_id", the_layer->Id());
	a_message.AddPointer("layer_pointer", (void*)the_layer);

	uint32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*) &buttons);
	BRect mini_image_frame = thumbnail_view->Frame();

	if (image_window != NULL) {
		a_message.what = HS_LAYER_ACTIVATED;
		image_window->PostMessage(&a_message, image_view);
		if (layer_window)
			layer_window->SetActiveLayer(the_layer);
	}
	// We start reordering the layers.
	thread_id reorder = spawn_thread(
		LayerView::reorder_thread, "reorder layers", B_NORMAL_PRIORITY, (void*)this);
	resume_thread(reorder);
}


void
LayerView::MouseMoved(BPoint where, uint32 transit, const BMessage*)
{
}


void
LayerView::UpdateImage()
{
	thumbnail_view->Redraw();
}


void
LayerView::Activate(bool active)
{
	is_active = active;
	if (LockLooper() == TRUE) {
		if (active == TRUE)
			SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT));
		else if (Parent() != NULL)
			SetViewColor(Parent()->ViewColor());

		visibility_box->SetViewColor(ViewColor());
		visibility_box->Invalidate();

		Invalidate();
		UnlockLooper();
	} else {
		if (active == TRUE)
			SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT));
		else if (Parent() != NULL)
			SetViewColor(Parent()->ViewColor());

		visibility_box->SetViewColor(ViewColor());
	}
}


void
LayerView::SetVisibility(bool visible)
{
	BWindow* a_window = visibility_box->Window();
	if (a_window != NULL)
		a_window->Lock();

	if (visible == TRUE)
		visibility_box->SetValue(B_CONTROL_ON);
	else
		visibility_box->SetValue(B_CONTROL_OFF);

	if (a_window != NULL)
		a_window->Unlock();
}


void
LayerView::SetName(const char* name)
{
	if (LockLooper() == true) {
		layer_name_field->SetText(name);
		UnlockLooper();
	}
}


int32
LayerView::reorder_thread(void* data)
{
	LayerView* this_pointer = (LayerView*)data;
	return this_pointer->ReorderViews();
}


int32
LayerView::ReorderViews()
{
	uint32 buttons;
	BPoint location;
	BRect frame;

	LayerWindow* the_window = (LayerWindow*)Window();

	int32 positions_moved = 0;

	if (the_window != NULL) {
		the_window->Lock();
		BView* parent_view = the_window->GetListView();
		BView* exchanged_view;
		parent_view->GetMouse(&location, &buttons);
		the_window->Unlock();

		CustomGridLayout* layout = (CustomGridLayout*)the_window->GetListView()->GetLayout();

		if (parent_view != NULL) {
			while (buttons) {
				the_window->Lock();
				BPoint thisPos = layout->GetViewPosition(this);
				frame = this->ConvertToParent(Bounds());
				frame.OffsetBy(0, -(Bounds().Height() + 1));

				if (frame.Contains(location) == TRUE && thisPos.y > 1) {
					exchanged_view = layout->ItemAt(0, thisPos.y - 1)->View();

					if (exchanged_view != NULL) {
						int32 exchangedIndex = layout->IndexOfView(exchanged_view);

						if (exchanged_view != NULL
							&& exchanged_view->Parent() == parent_view
							&& exchangedIndex >= 0
							&& exchanged_view != this) {
							layout->SwapViews(this, exchanged_view);
							positions_moved++;
							BRect pframe = parent_view->ConvertToParent(frame);
							pframe.OffsetBy(0, -pframe.Height() * 0.95);
							if (parent_view->Frame().Contains(pframe) == FALSE)
								parent_view->ScrollBy(0, -pframe.Height() * .95);
						}
					}
				} else {
					frame.OffsetBy(0, (2 * Bounds().Height()) + 2);
					if (frame.Contains(location) == TRUE) {
						exchanged_view = layout->ItemAt(0, thisPos.y + 1)->View();
						if (exchanged_view != NULL) {
							int32 exchangedIndex = layout->IndexOfView(exchanged_view);

							if (exchanged_view != NULL
								&& exchanged_view->Parent() == parent_view
								&& exchangedIndex >= 0
								&& exchanged_view != this) {
								layout->SwapViews(this, exchanged_view);
								positions_moved--;
								BRect pframe = parent_view->ConvertToParent(frame);
								pframe.OffsetBy(0, pframe.Height() * 0.95);
								if (parent_view->Frame().Contains(pframe) == FALSE)
									parent_view->ScrollBy(0, pframe.Height() * .95);
							}
						}
					}
				}

				parent_view->GetMouse(&location, &buttons);
				the_window->Unlock();

				snooze(20 * 1000);
			}
		}
	}
	if (positions_moved != 0) {
		BView* image_view = the_layer->GetImageView();
		BWindow* image_window = image_view->Window();
		BMessage a_message;
		a_message.what = HS_LAYER_POSITION_CHANGED;
		a_message.AddInt32("layer_id", the_layer->Id());
		a_message.AddPointer("layer_pointer", (void*)the_layer);
		a_message.AddInt32("positions_moved", positions_moved);
		image_window->PostMessage(&a_message, image_view);
	}

	return positions_moved;
}


ThumbnailView::ThumbnailView(BBitmap* image)
	:
	BView(BRect(0, 0, 1, 1), "thumbview", B_FOLLOW_NONE, B_WILL_DRAW),
	fThumbnailBitmap(NULL)
{
	BRect frame = image->Bounds();

	SetExplicitMinSize(BSize(frame.Width(), frame.Height()));
	SetExplicitMaxSize(BSize(frame.Width(), frame.Height()));

	frame.InsetBy(1.0, 1.0);
	fThumbnailBitmap = image;
}


ThumbnailView::~ThumbnailView()
{
}


void
ThumbnailView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);

	DrawBitmap(fThumbnailBitmap, BPoint(1.0, 1.0));

	SetHighColor(0, 0, 0, 255);
	StrokeRect(Bounds());
}


void
ThumbnailView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BView::MessageReceived(message);
	}
}


void
ThumbnailView::MouseDown(BPoint location)
{
	Parent()->MouseDown(location);
}
