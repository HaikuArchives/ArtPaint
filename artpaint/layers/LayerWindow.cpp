/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#include "LayerWindow.h"

#include "CustomGridLayout.h"
#include "FloaterManager.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "MessageFilters.h"
#include "Layer.h"
#include "LayerView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "PaintWindow.h"
#include "PixelOperations.h"
#include "UtilityClasses.h"
#include "SettingsServer.h"
#include "UtilityClasses.h"


#include <Bitmap.h>
#include <Button.h>
#include <Catalog.h>
#include <Font.h>
#include <GroupLayout.h>
#include <GridLayout.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <SpaceLayoutItem.h>
#include <String.h>
#include <StringView.h>


#include <new>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LayerWindow"


using ArtPaint::Interface::NumberSliderControl;


// Initialize the static pointer to the layer window.
LayerWindow* LayerWindow::layer_window = NULL;

// Initialize the static pointers to target-window and layer-list, and
// also the composite picture and target-window's title
BWindow* LayerWindow::target_window = NULL;
BList* LayerWindow::target_list = NULL;
const char* LayerWindow::window_title = NULL;
sem_id LayerWindow::layer_window_semaphore = create_sem(1,"layer window semaphore");


LayerWindow::LayerWindow(BRect frame)
	: BWindow(frame, B_TRANSLATE("Layers"),
		B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_NOT_ZOOMABLE)
{
	top_part = new BBox(B_PLAIN_BORDER, NULL);
	title_view = new BStringView("title", "");

	layer_operation_menu = new BMenu(B_TRANSLATE("Layer"));

	layer_operation_menu->AddItem(new BMenuItem(
		B_TRANSLATE("Add"),
		new BMessage(HS_ADD_LAYER_FRONT)));

	layer_operation_menu->AddItem(new BMenuItem(
		B_TRANSLATE("Delete"),
		new BMessage(HS_DELETE_LAYER)));

	layer_operation_menu->AddItem(new BMenuItem(
		B_TRANSLATE("Duplicate"),
		new BMessage(HS_DUPLICATE_LAYER)));

	layer_operation_menu->AddItem(new BMenuItem(
		B_TRANSLATE("Merge down"),
		new BMessage(HS_MERGE_WITH_LOWER_LAYER)));

	layer_operation_menu->SetRadioMode(false);

	BMenuBar* menu = new BMenuBar("menu bar");

	menu->AddItem(layer_operation_menu);

	blend_mode_menu = new BPopUpMenu("blend_mode");

	BMessage* blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_NORMAL);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_NORMAL), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_DISSOLVE);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_DISSOLVE), blend_msg));

	blend_mode_menu->AddSeparatorItem();

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_DARKEN);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_DARKEN), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_MULTIPLY);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_MULTIPLY), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_BURN);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_BURN), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_LINEAR_BURN);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_LINEAR_BURN), blend_msg));

	blend_mode_menu->AddSeparatorItem();

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_LIGHTEN);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_LIGHTEN), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_SCREEN);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_SCREEN), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_DODGE);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_DODGE), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_LINEAR_DODGE);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_LINEAR_DODGE), blend_msg));

	blend_mode_menu->AddSeparatorItem();

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_OVERLAY);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_OVERLAY), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_SOFT_LIGHT);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_SOFT_LIGHT), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_HARD_LIGHT);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_HARD_LIGHT), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_VIVID_LIGHT);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_VIVID_LIGHT), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_LINEAR_LIGHT);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_LINEAR_LIGHT), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_PIN_LIGHT);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_PIN_LIGHT), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_HARD_MIX);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_HARD_MIX), blend_msg));

	blend_mode_menu->AddSeparatorItem();

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_DIFFERENCE);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_DIFFERENCE), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_EXCLUSION);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_EXCLUSION), blend_msg));

	blend_msg = new BMessage(HS_LAYER_BLEND_MODE_CHANGED);
	blend_msg->AddUInt8("blend_mode", BLEND_DIVIDE);
	blend_mode_menu->AddItem(new BMenuItem(mode_to_string(BLEND_DIVIDE), blend_msg));

	blend_mode_menu->ItemAt(0)->SetMarked(TRUE);

	BMenuField* blend_dropdown = new BMenuField("blend_dropdown",
		B_TRANSLATE("Mode:"), blend_mode_menu);

	BMessage* message = new BMessage(HS_LAYER_TRANSPARENCY_CHANGED);
	message->AddInt32("value", 0);

	transparency_slider =
		new NumberSliderControl("Alpha:", "0",
		message, 0, 100, false);

	BFont font;

	BGridLayout* transparencyLayout = BLayoutBuilder::Grid<>(0.0)
		.Add(transparency_slider, 0, 0, 0, 0)
		.Add(transparency_slider->LabelLayoutItem(), 0, 0)
		.Add(transparency_slider->TextViewLayoutItem(), 1, 0)
		.Add(transparency_slider->Slider(), 2, 0, 2)
		.Add(blend_dropdown->CreateLabelLayoutItem(), 0, 1)
		.Add(blend_dropdown->CreateMenuBarLayoutItem(), 1, 1, 3);

	transparencyLayout->SetMaxColumnWidth(1, font.StringWidth("1"));
	transparencyLayout->SetMinColumnWidth(2,
		font.StringWidth("SLIDERSLIDERSLIDER"));

	transparency_slider->Slider()->SetToolTip(B_TRANSLATE("Layer transparency"));

	BGroupLayout* topLayout = BLayoutBuilder::Group<>(top_part, B_VERTICAL)
		.Add(transparencyLayout)
		.SetInsets(B_USE_SMALL_INSETS, B_USE_SMALL_INSETS,
			B_USE_SMALL_INSETS, B_USE_SMALL_INSETS);

	list_view = new LayerListView();
	list_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BScrollView* scroll_view = new BScrollView("scroller", list_view,
		B_FRAME_EVENTS, false, true, B_NO_BORDER);
	scroll_view->ScrollBar(B_VERTICAL)->SetSteps(8.0, 32.0);

	BGroupLayout* mainLayout = BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menu)
		.Add(top_part)
		.Add(scroll_view)
		.SetInsets(0, 0, -1, -1);

	layer_count = 0;
	layer_window = this;

	window_feel feel = B_NORMAL_WINDOW_FEEL;
	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		settings.FindInt32(skLayerWindowFeel, (int32*)&feel);
		server->SetValue(SettingsServer::Application, skLayerWindowVisible,
			true);
	}
	setFeel(feel);
	float min_width = font.StringWidth("W") * 20;

	scroll_view->SetExplicitMinSize(BSize(min_width -
		scroll_view->ScrollBar(B_VERTICAL)->Bounds().Width(),
		LAYER_VIEW_HEIGHT));

	if (Lock()) {
		BMessageFilter *activation_filter = new BMessageFilter(B_ANY_DELIVERY,
			B_ANY_SOURCE, B_MOUSE_DOWN, window_activation_filter);
		AddCommonFilter(activation_filter);
		AddCommonFilter(new BMessageFilter(B_KEY_DOWN, AppKeyFilterFunction));
		SetSizeLimits(min_width, 1000,
			LAYER_VIEW_HEIGHT * 2.5, 1000);
		Unlock();
	}
	Show();

	FloaterManager::AddFloater(this);

	active_layer = NULL;
}


LayerWindow::~LayerWindow()
{
	acquire_sem(layer_window_semaphore);
	while (layer_window->list_view->CountChildren() != 0)
		layer_window->list_view->RemoveChild(layer_window->list_view->ChildAt(0));
	layer_window = NULL;
	release_sem(layer_window_semaphore);

	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skLayerWindowFrame,
			Frame());
		server->SetValue(SettingsServer::Application, skLayerWindowVisible,
			false);
	}

	FloaterManager::RemoveFloater(this);
}


void
LayerWindow::MessageReceived(BMessage *message)
{
	// a standard switch to handle the messages
	switch (message->what) {
		case HS_ADD_LAYER_FRONT:
		case HS_DUPLICATE_LAYER:
		case HS_DELETE_LAYER:
		case HS_MERGE_WITH_LOWER_LAYER:
			if (active_layer != NULL) {
				BMessage layer_op_message;
				layer_op_message.what = message->what;
				layer_op_message.AddInt32("layer_id", active_layer->Id());
				layer_op_message.AddPointer("layer_pointer",(void*)active_layer);

				BView *image_view = (BView*)active_layer->GetImageView();
				BWindow *image_window = image_view->Window();

				if (image_window && active_layer->IsActive())
					image_window->PostMessage(&layer_op_message, image_view);
			} break;
		case HS_LAYER_TRANSPARENCY_CHANGED:
			if (active_layer != NULL && active_layer->IsActive()) {
				int32 value = transparency_slider->Value();
				active_layer->SetTransparency((float)value / 100.0f);

				BView *image_view = (BView*)active_layer->GetImageView();
				BWindow *image_window = image_view->Window();

				if (image_window && active_layer->IsActive())
					image_window->PostMessage(message, image_view);
			} break;
		case HS_LAYER_BLEND_MODE_CHANGED:
			if (active_layer != NULL && active_layer->IsActive()) {
				uint8 mode;
				if(message->FindUInt8("blend_mode", &mode) == B_OK) {
					active_layer->SetBlendMode(mode);

					BView *image_view = (BView*)active_layer->GetImageView();
					BWindow *image_window = image_view->Window();

					if (image_window && active_layer->IsActive())
						image_window->PostMessage(message, image_view);
				}
			} break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
LayerWindow::QuitRequested()
{
	return true;
}


void
LayerWindow::ActiveWindowChanged(BWindow *active_window,
	BList *list, BBitmap *composite)
{
	acquire_sem(layer_window_semaphore);
	target_window = active_window;
	target_list = list;

	Layer *a_layer = NULL;
	if (list != NULL)
		a_layer = (Layer*)list->ItemAt(0);

	if (a_layer != NULL)
		window_title = a_layer->ReturnProjectName();
	else
		window_title = NULL;

//	if (target_window != NULL)
//		window_title = target_window->Title();
//	else
//		window_title = NULL;

	if (layer_window != NULL)
		layer_window->Update();

	release_sem(layer_window_semaphore);
}


void
LayerWindow::showLayerWindow()
{
	acquire_sem(layer_window_semaphore);
	if (layer_window == NULL) {
		BRect frame(300, 300, 400, 400);
		if (SettingsServer* server = SettingsServer::Instance()) {
			BMessage settings;
			server->GetApplicationSettings(&settings);
			settings.FindRect(skLayerWindowFrame, &frame);
		}
		new LayerWindow(FitRectToScreen(frame));

		layer_window->Update();
	}
	else {
		layer_window->SetWorkspaces(B_CURRENT_WORKSPACE);
		layer_window->Show();
		layer_window->Activate();
		BRect frame = layer_window->Frame();
		frame = FitRectToScreen(frame);
		layer_window->MoveTo(frame.LeftTop());
	}
	release_sem(layer_window_semaphore);
}


void
LayerWindow::setFeel(window_feel feel)
{
	if (SettingsServer* server = SettingsServer::Instance()) {
		server->SetValue(SettingsServer::Application, skLayerWindowFeel,
			int32(feel));
	}

	if (layer_window) {
		layer_window->Lock();
		layer_window->SetFeel(feel);
		if (feel == B_NORMAL_WINDOW_FEEL)
			layer_window->SetLook(B_TITLED_WINDOW_LOOK);
		else
			layer_window->SetLook(B_FLOATING_WINDOW_LOOK);

		layer_window->Unlock();
	}
}


void
LayerWindow::FrameResized(float width, float height)
{
}


void
LayerWindow::Update()
{
	// Only update if the order or the amount of the layers has changed or
	// the layers have completely changed.
	bool must_update = FALSE;
	if (target_list != NULL) {
		layer_window->Lock();
		if (layer_count != target_list->CountItems())
			must_update = TRUE;
		else {
			for (int32 i = 0;i < list_view->CountChildren(); i++) {
				Layer *layer = (Layer*)((LayerView*)list_view->ChildAt(i))->ReturnLayer();
				if (layer != (Layer*)target_list->ItemAt(i))
					must_update = TRUE;
				if (layer->IsActive())
					SetActiveLayer(layer);
			}
		}
		if ((window_title == NULL) || (title_view->Text() == NULL) || (strcmp(window_title,title_view->Text()) != 0))
			must_update = TRUE;

		layer_window->Unlock();
	}
	else
		must_update = TRUE;

	if (must_update) {
		layer_window->Lock();

		BGridLayout* layout = (BGridLayout*)layer_window->list_view->GetLayout();

		while (layout->CountItems() > 0)
			layout->RemoveItem((int32) 0);

		// locking target_window here causes deadlocks so we do not lock it at the moment
		// concurrency problems with the closing of target_window should be solved somehow though

		int32 number_of_layers = 0;
		// then we add the layers of current paint window
		if ((target_window != NULL)) {
			if (target_list != NULL) {
				// Reorder the layers' views so that the topmost layer's view is at the top.
				int numItems = target_list->CountItems();
				for (int32 i = numItems - 1;i >= 0; i--) {
					Layer* added_layer = (Layer*)target_list->ItemAt(i);
					LayerView *added_view = (LayerView*)(added_layer)->GetView();
					layout->AddView(added_view, 0, numItems - i);
					if (added_layer->IsActive())
						SetActiveLayer(added_layer);
				}
				number_of_layers = target_list->CountItems();
				layout->AddItem(BSpaceLayoutItem::CreateGlue(), 0,
					number_of_layers + 1);
			}
		}

		layer_count = number_of_layers;

		BFont font;
		float width = font.StringWidth("AVERYLONGFILENAMESERIOUSLY");
		BString name(window_title);
		font.TruncateString(&name, B_TRUNCATE_MIDDLE, width);
		layer_window->title_view->SetText(name);

		layer_window->Unlock();

		// jiggle the window to update the scroll bar
		layer_window->ResizeBy(1, 0);
		layer_window->ResizeBy(-1, 0);
	}
}


void
LayerWindow::SetActiveLayer(Layer* layer)
{
	active_layer = layer;
	transparency_slider->SetValue(active_layer->GetTransparency() * 100);
	BMenuItem* blend_mode_item = blend_mode_menu->FindItem(
		mode_to_string((BlendModes)(active_layer->GetBlendMode())));

	if (blend_mode_item)
		blend_mode_item->SetMarked(true);

	BView *image_view = (BView*)active_layer->GetImageView();
	PaintWindow *image_window = (PaintWindow*)(image_view->Window());
	BMenuBar* main_menubar = image_window->ReturnMenuBar();

	layer_operation_menu->FindItem(HS_DELETE_LAYER)->SetEnabled(TRUE);
	main_menubar->FindItem(HS_DELETE_LAYER)->SetEnabled(TRUE);

	if (layer->ReturnLowerLayer() == NULL) {
		layer_operation_menu->FindItem(HS_MERGE_WITH_LOWER_LAYER)->SetEnabled(FALSE);
		main_menubar->FindItem(HS_MERGE_WITH_LOWER_LAYER)->SetEnabled(FALSE);
		if (layer->ReturnUpperLayer() == NULL) {
			layer_operation_menu->FindItem(HS_DELETE_LAYER)->SetEnabled(FALSE);
			main_menubar->FindItem(HS_DELETE_LAYER)->SetEnabled(FALSE);
		}
	} else {
		layer_operation_menu->FindItem(HS_MERGE_WITH_LOWER_LAYER)->SetEnabled(TRUE);
		main_menubar->FindItem(HS_MERGE_WITH_LOWER_LAYER)->SetEnabled(TRUE);
	}

}


LayerListView::LayerListView()
	: BView("list of layers", B_WILL_DRAW | B_FRAME_EVENTS)
{
	CustomGridLayout *mainLayout = new CustomGridLayout(B_USE_DEFAULT_SPACING, 0.0);
	SetLayout(mainLayout);
}


LayerListView::~LayerListView()
{
}


void
LayerListView::DetachedFromWindow()
{
	BView *view = ScrollBar(B_VERTICAL);

	if (view != NULL) {
		view->RemoveSelf();
		delete view;
	}
}


void
LayerListView::FrameResized(float, float height)
{
}
