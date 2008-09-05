/* 

	Filename:	LayerWindow.cpp
	Contents:	LayerWindow-class definition	
	Author:		Heikki Suhonen
	
*/

#include <InterfaceDefs.h>
#include <ScrollBar.h>
#include <stdio.h>
#include <StringView.h>

#include "FloaterManager.h"
#include "LayerWindow.h"
#include "PaintApplication.h"
#include "MessageConstants.h"
#include "Layer.h"
#include "UtilityClasses.h"
#include "LayerView.h"
#include "Settings.h"
#include "StringServer.h"


// Initialize the static pointer to the layer window.
LayerWindow* LayerWindow::layer_window = NULL;

// Initialize the static pointers to target-window and layer-list, and
// also the composite picture and target-window's title
BWindow* LayerWindow::target_window = NULL;
BList* LayerWindow::target_list = NULL;
BBitmap* LayerWindow::composite_image = NULL;
const char* LayerWindow::window_title = NULL;
sem_id LayerWindow::layer_window_semaphore = create_sem(1,"layer window semaphore");

LayerWindow::LayerWindow(BRect frame)
	:	BWindow(frame,StringServer::ReturnString(LAYERS_STRING),B_FLOATING_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_H_RESIZABLE | B_NOT_ZOOMABLE | B_WILL_ACCEPT_FIRST_CLICK)
{ 
	BBox *top_part = new BBox(BRect(-1,0,Bounds().Width()+2,HS_MINIATURE_IMAGE_HEIGHT+3),NULL,B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	AddChild(top_part);
	bitmap_view = new BitmapView(NULL,BRect(6,2,HS_MINIATURE_IMAGE_WIDTH-1+6,HS_MINIATURE_IMAGE_HEIGHT-1+2));
	top_part->AddChild(bitmap_view);
	title_view = new BStringView(BRect(HS_MINIATURE_IMAGE_WIDTH+10,2,top_part->Bounds().Width()-2,HS_MINIATURE_IMAGE_HEIGHT-2),"image title","");
	top_part->AddChild(title_view);
	
	
	list_view = new LayerListView();
	list_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	list_view->MoveTo(top_part->Frame().LeftBottom()+BPoint(0,1));	
	list_view->ResizeTo(frame.Width()-B_V_SCROLL_BAR_WIDTH,frame.Height()-list_view->Frame().top);
	AddChild(list_view);
	
	scroll_bar = new BScrollBar(BRect(0,0,B_V_SCROLL_BAR_WIDTH-1,100),"layer_window_scroll_bar",list_view,0,0,B_VERTICAL);	
	scroll_bar->MoveTo(list_view->Frame().RightTop()+BPoint(2,0));
	scroll_bar->ResizeTo(B_V_SCROLL_BAR_WIDTH,100);
	AddChild(scroll_bar);

	layer_window = this;
	layer_count = 0;

	window_feel feel = ((PaintApplication*)be_app)->Settings()->layer_window_feel;
	setFeel(feel);
	((PaintApplication*)be_app)->Settings()->layer_window_visible = TRUE;
	ResizeBy(1,0);
	ResizeBy(-1,0);

	Show();
	FloaterManager::AddFloater(this);
}


LayerWindow::~LayerWindow()
{
	acquire_sem(layer_window_semaphore);
	// Here we must remove any existing layer views.
	while (layer_window->list_view->CountChildren() != 0) {
		layer_window->list_view->RemoveChild(layer_window->list_view->ChildAt(0));
	}
	
	layer_window = NULL;
	release_sem(layer_window_semaphore);
	
	// Then we must record our frame to the app's preferences.
	((PaintApplication*)be_app)->Settings()->layer_window_frame = Frame();
	((PaintApplication*)be_app)->Settings()->layer_window_visible = FALSE;

	FloaterManager::RemoveFloater(this);
}


void LayerWindow::MessageReceived(BMessage *message)
{
	// a standard switch to handle the messages
	switch (message->what) {
	default:
		BWindow::MessageReceived(message);
		break;
	}
}

bool LayerWindow::QuitRequested()
{
	return TRUE;
}

void LayerWindow::ActiveWindowChanged(BWindow *active_window,BList *list,BBitmap *composite)
{
	acquire_sem(layer_window_semaphore);
	target_window = active_window;
	target_list = list;
	composite_image = composite;

	Layer *a_layer = NULL;
	if (list != NULL) {
		a_layer = (Layer*)list->ItemAt(0);
	}
	
	if (a_layer != NULL)
		window_title = a_layer->ReturnProjectName();
	else
		window_title = NULL;
		
//	if (target_window != NULL) 
//		window_title = target_window->Title();
//	else
//		window_title = NULL;
			
	if (layer_window != NULL) {
		layer_window->Update();
	}
	release_sem(layer_window_semaphore);
}

void LayerWindow::showLayerWindow()
{
	acquire_sem(layer_window_semaphore);
	if (layer_window == NULL) {
		BRect frame = ((PaintApplication*)be_app)->Settings()->layer_window_frame;
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


void LayerWindow::setFeel(window_feel feel)
{
	((PaintApplication*)be_app)->Settings()->layer_window_feel = feel;
	
	if (layer_window != NULL) {
		layer_window->Lock();
		layer_window->SetFeel(feel);
		if (feel == B_NORMAL_WINDOW_FEEL) {
			layer_window->SetLook(B_DOCUMENT_WINDOW_LOOK);
			layer_window->scroll_bar->ResizeTo(layer_window->scroll_bar->Bounds().Width(),layer_window->list_view->Bounds().Height()-B_H_SCROLL_BAR_HEIGHT+1);			
		}
		else {
			layer_window->SetLook(B_FLOATING_WINDOW_LOOK);
			layer_window->scroll_bar->ResizeTo(layer_window->scroll_bar->Bounds().Width(),layer_window->list_view->Bounds().Height()+1);			
		}
		layer_window->Unlock();
	}
}


void LayerWindow::Update()
{
	// Only update if the order or the amount of the layers has changed or
	// the layers have completely changed.
	bool must_update = FALSE;
	if (target_list != NULL) {
		layer_window->Lock();
		if (layer_count != target_list->CountItems())
			must_update = TRUE;
		else {	
			for (int32 i=0;i<list_view->CountChildren();i++) {
				Layer *layer = (Layer*)((LayerView*)list_view->ChildAt(i))->ReturnLayer();
				if (layer != (Layer*)target_list->ItemAt(i))
					must_update = TRUE;	
			
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
			
		// Here we must first remove any existing layer views.
		while (layer_window->list_view->CountChildren() > 0) {
			layer_window->list_view->RemoveChild(layer_window->list_view->ChildAt(0));
		}
		
		// locking target_window here causes deadlocks so we do not lock it at the moment
		// concurrency problems with the closing of target_window should be solved somehow though 
		
		// Move the scroll-bar up
		float scroll_bar_old_value = layer_window->list_view->ScrollBar(B_VERTICAL)->Value();
		layer_window->list_view->ScrollBar(B_VERTICAL)->SetValue(0);
		
		int32 number_of_layers = 0;			
		// then we add the layers of current paint window
		if ((target_window != NULL)) {
			if (target_list != NULL) {
				// Reorder the layers' views so that the topmost layer's view is at the top. 
				for (int32 i=0;i<target_list->CountItems();i++) {
					BView *added_view = (BView*)((Layer*)target_list->ItemAt(i))->GetView();
					layer_window->list_view->AddChild(added_view);
					layer_window->list_view->ChildAt(i)->MoveTo(1,(target_list->CountItems() - i-1)*LAYER_VIEW_HEIGHT+1);
				}
				number_of_layers = target_list->CountItems();
			}
		}
		
		layer_count = number_of_layers;
		
		BView *first_layer = layer_window->list_view->ChildAt(0);
		if (first_layer != NULL) {
			ResizeBy(first_layer->Bounds().Width() - layer_window->list_view->Bounds().Width(),0);
		}
	
	
	//	layer_window->SetSizeLimits(10,1000,140,layer_window->scroll_view->Frame().top + number_of_layers * LAYER_VIEW_HEIGHT);
		
		layer_window->list_view->ScrollBar(B_VERTICAL)->SetRange(0,number_of_layers * LAYER_VIEW_HEIGHT-layer_window->list_view->Bounds().Height());
		layer_window->list_view->ScrollBar(B_VERTICAL)->SetProportion(layer_window->list_view->Bounds().Height()/(float)(number_of_layers * LAYER_VIEW_HEIGHT));
		layer_window->list_view->ScrollBar(B_VERTICAL)->SetValue(scroll_bar_old_value);
		
		layer_window->title_view->SetText(window_title);			
		layer_window->bitmap_view->ChangeBitmap(composite_image);
		layer_window->bitmap_view->Invalidate();
		
		layer_window->Unlock();	
	}
}




LayerListView::LayerListView()
	: BView(BRect(0,0,120,140),"list of layers",B_FOLLOW_ALL,B_WILL_DRAW|B_FRAME_EVENTS)
{
}	


LayerListView::~LayerListView()
{

}


void LayerListView::DetachedFromWindow()
{
	BView *view = ScrollBar(B_VERTICAL);

	if (view != NULL) {
		view->RemoveSelf();
		delete view;
	}
}


void LayerListView::FrameResized(float, float height)
{
	// We only care about the height.
	BScrollBar *v_scroll_bar = ScrollBar(B_VERTICAL);
	if (CountChildren() > 0) {	
		if (v_scroll_bar != NULL) {
			v_scroll_bar->SetRange(0,max_c(0,CountChildren()*LAYER_VIEW_HEIGHT-height));
			v_scroll_bar->SetProportion(height/(float)(CountChildren()*LAYER_VIEW_HEIGHT));
		}
	
		if (ChildAt(0)->Frame().bottom < height) {
			if (ChildAt(CountChildren()-1)->Frame().top < 0) {
				ScrollBy(0,min_c(height-ChildAt(0)->Frame().bottom,height-ChildAt(CountChildren()-1)->Frame().top));
			}	
		}
	}
	else {
		v_scroll_bar->SetRange(0,0);
	}
}
 