/* 

	Filename:	DatatypeSetupWindow.cpp
	Contents:	Definitions for DatatypeSetupWindow	
	Author:		Heikki Suhonen
	
*/

#include <InterfaceDefs.h>
#include <stdio.h>
#include <StringView.h>

#include "DatatypeSetupWindow.h"
#include "PaintApplication.h"
#include "StringServer.h"


BWindow* DatatypeSetupWindow::setup_window = NULL;

DatatypeSetupWindow::DatatypeSetupWindow()
		: BWindow(BRect(100,120,120,120),"Datatype setup",B_FLOATING_WINDOW_LOOK,B_FLOATING_APP_WINDOW_FEEL,B_NOT_RESIZABLE)
{
	container = new BView(Bounds(),"container",B_FOLLOW_ALL_SIDES,B_WILL_DRAW);
	container->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	container->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));	
	AddChild(container);
	
	setup_window = this;
	Lock();
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();
}


DatatypeSetupWindow::~DatatypeSetupWindow()
{
	setup_window = NULL;	
}

void DatatypeSetupWindow::ChangeHandler(translator_id handler)
{
	const int32 window_min_width = 250;
	const int32 window_min_height = 200;

	// Only do something if the setup-window is already open.
	if (setup_window != NULL) {
		setup_window->Lock();
		// Remove the old view and create a new one
		BView *config_view = NULL;
		BRect config_rect;
		
		BTranslatorRoster *roster = BTranslatorRoster::Default();
				
		if ((config_view = (setup_window->FindView("container")->ChildAt(0))) != NULL) {
			config_view->RemoveSelf();
			delete config_view;	
			config_view = NULL;
		}
	
		if ((roster->MakeConfigurationView(handler,NULL,&config_view,&config_rect) == B_NO_ERROR) && (config_view != NULL)) {
			if (config_view != NULL) {
				config_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
				setup_window->ResizeTo(max_c(config_rect.Width()+8,window_min_width),max_c(config_rect.Height()+8,window_min_height));
				config_view->ResizeTo(max_c(config_rect.Width(),window_min_width-8),max_c(config_rect.Height(),window_min_height-8));
				config_view->MoveTo(4,4);								
				setup_window->FindView("container")->AddChild(config_view);
			}			
		}
		else {
			BStringView *no_options_view = new BStringView(BRect(0,0,0,0),"no_options_view",StringServer::ReturnString(NO_OPTIONS_STRING));
			no_options_view->ResizeTo(window_min_width-8,window_min_height-8);
			setup_window->ResizeTo(window_min_width,window_min_height);
			setup_window->FindView("container")->AddChild(no_options_view);			
			no_options_view->MoveTo(4,4);
		}

		const char	*handler_name;
		const char	*handler_info;
		int32	handler_version;

		if (roster->GetTranslatorInfo(handler,&handler_name,&handler_info,&handler_version) == B_NO_ERROR) {
			char window_name[256];
			sprintf(window_name,"Datatype Setup: %s",handler_name);
			setup_window->SetTitle(window_name);
		}
		else {
			setup_window->SetTitle("Datatype Setup: no translator");	
		}
		
		setup_window->Unlock();		
	}
}


void DatatypeSetupWindow::showWindow(translator_id handler)
{
	if (setup_window == NULL) {
		setup_window = new DatatypeSetupWindow();
		setup_window->Show();
	}
	else {
		if (setup_window->IsHidden()) {
			setup_window->Show();
		}
		if (!setup_window->IsActive()) {
			setup_window->Activate(TRUE);
		}
	}	
	
	// Also change the handler
	ChangeHandler(handler);
}
