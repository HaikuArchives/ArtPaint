/* 

	Filename:	GlobalSetupWindow.cpp
	Contents:	Definitions for GlobalSetupWindow-class	
	Author:		Heikki Suhonen
	
*/

#include <Button.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <stdio.h>
#include <string.h>
#include <TabView.h>

#include "GlobalSetupWindow.h"
#include "MessageConstants.h"
#include "RGBControl.h"
#include "PaintApplication.h"
#include "Settings.h"
#include "ColorPalette.h"
#include "LayerWindow.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
#include "BrushStoreWindow.h"
#include "ManipulatorWindow.h"
#include "UndoQueue.h"
#include "Controls.h"
#include "StringServer.h"
#include "Cursors.h"
#include "Flags.h"
#include "UtilityClasses.h"

GlobalSetupWindow* GlobalSetupWindow::setup_window = NULL;

GlobalSetupWindow::GlobalSetupWindow(BRect frame)
		:	BWindow(frame,StringServer::ReturnString(GLOBAL_SETTINGS_STRING),B_TITLED_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_ZOOMABLE|B_NOT_RESIZABLE)
{
	frame.OffsetTo(0,0);
	BBox *bg = new BBox(frame);
	bg->SetResizingMode(B_FOLLOW_ALL_SIDES);
	bg->SetBorder(B_NO_BORDER);
	AddChild(bg);
	BMessage *button_message = new BMessage(HS_CLOSE_GLOBAL_SETTINGS_WINDOW);
	button_message->AddBool("apply",TRUE);
	BButton *a_button = new BButton(BRect(0,0,0,0),"ok button",StringServer::ReturnString(APPLY_CHANGES_STRING),button_message);
	bg->AddChild(a_button);
	a_button->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	a_button->ResizeToPreferred();
	a_button->MoveTo(bg->Bounds().Width()-a_button->Bounds().Width()-8,bg->Bounds().bottom - a_button->Bounds().Height() - 5);
	SetDefaultButton(a_button);

	button_message = new BMessage(HS_CLOSE_GLOBAL_SETTINGS_WINDOW);
	button_message->AddBool("apply",FALSE);
	BButton *discard_button = new BButton(BRect(0,0,0,0),"cancel button",StringServer::ReturnString(DISCARD_CHANGES_STRING),button_message);
	bg->AddChild(discard_button);
	discard_button->SetResizingMode(B_FOLLOW_RIGHT|B_FOLLOW_BOTTOM);
	discard_button->ResizeToPreferred();
	discard_button->MoveTo(a_button->Frame().left-discard_button->Bounds().Width()-4,bg->Bounds().bottom - discard_button->Bounds().Height() - 5);
//	a_button->MoveBy(discard_button->Bounds().Width() + 10,0);
	
	frame.bottom -= (a_button->Bounds().Height() + 10) ;

	feel_view = NULL;
	undo_view = NULL;
	language_view = NULL;
	general_view = NULL;


	// Here add the tab-view	
	tab_view = new BTabView(frame,"setup_tab_view");

	frame.InsetBy(5,5);
	frame.bottom -= tab_view->TabHeight();

	// Add a tab for window-feel checkboxes		
	BTab *tab = new BTab();	
	feel_view = new WindowFeelView(frame);
	tab_view->AddTab(feel_view,tab);
	tab->SetLabel(StringServer::ReturnString(WINDOW_FLOATING_STRING));


	// Add a tab for the undo.
	tab = new BTab();
	undo_view = new UndoControlView(frame);
	tab_view->AddTab(undo_view,tab);
	tab->SetLabel(StringServer::ReturnString(UNDO_SETTINGS_STRING));



	// Add a tab for the language.
	tab = new BTab();
	language_view = new LanguageControlView(frame);
	tab_view->AddTab(language_view,tab);
	tab->SetLabel(StringServer::ReturnString(LANGUAGE_STRING));


	tab = new BTab();
	general_view = new GeneralControlView(frame);
	tab_view->AddTab(general_view,tab);
	tab->SetLabel(StringServer::ReturnString(MISCELLANEOUS_STRING));



	int32 tab_count = 4;

	bg->AddChild(tab_view);
	tab_view->SetTabWidth(B_WIDTH_FROM_LABEL);
	tab_view->ResizeToPreferred();
	tab_view->ResizeTo(tab_view->TabFrame(tab_count-1).right+tab_view->TabFrame(0).left,tab_view->Frame().Height());
	tab_view->SetResizingMode(B_FOLLOW_NONE);
	ResizeTo(tab_view->Frame().Width(),Frame().Height());
	bg->ResizeTo(Frame().Width(),bg->Frame().Height());
	tab_view->SetResizingMode(B_FOLLOW_ALL_SIDES);
	ResizeBy(100,0);

	tab_view->Select(((PaintApplication*)be_app)->Settings()->settings_window_tab_number);
	Lock();
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,AppKeyFilterFunction));
	Unlock();
}


GlobalSetupWindow::~GlobalSetupWindow()
{
	// We should also store settings in PaintApplication::settings
	((PaintApplication*)be_app)->Settings()->global_setup_window_frame = Frame();
	((PaintApplication*)be_app)->Settings()->settings_window_tab_number = tab_view->Selection();
	
	setup_window = NULL;
}


void GlobalSetupWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case HS_CLOSE_GLOBAL_SETTINGS_WINDOW:
			if (msg->FindBool("apply") == TRUE) {
				if (feel_view != NULL)
					feel_view->ApplyChanges();
			
				if (undo_view != NULL)
					undo_view->ApplyChanges();

				if (language_view != NULL)
					language_view->ApplyChanges();

				if (general_view != NULL)
					general_view->ApplyChanges();
			}
			Close();
			break;
		default:
			BWindow::MessageReceived(msg);
			break;
	}
}

void GlobalSetupWindow::showGlobalSetupWindow()
{
	BRect frame = ((PaintApplication*)be_app)->Settings()->global_setup_window_frame;
	
	// If there is no setup-window we should open one.
	if (setup_window == NULL) {
		setup_window = new GlobalSetupWindow(frame);
	}

	if (setup_window->IsHidden())
		setup_window->Show();
	if (!setup_window->IsActive())
		setup_window->Activate(TRUE);
}


void GlobalSetupWindow::closeGlobalSetupWindow()
{
	setup_window->Close();
	delete setup_window;
	setup_window = NULL;
}



WindowFeelView::WindowFeelView(BRect frame)
	:	BView(frame,"window feel view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW)
{
	BRect rect = Bounds();
	rect.InsetBy(1,1);
	BBox *box = new BBox(rect);
	box->SetResizingMode(B_FOLLOW_ALL_SIDES);
	box->SetLabel(StringServer::ReturnString(KEEP_IN_FRONT_STRING));
	AddChild(box);

	font_height fHeight;
	box->GetFontHeight(&fHeight);
	
	float label_height = fHeight.ascent + fHeight.descent;

	tool_window_feel = ((PaintApplication*)be_app)->Settings()->tool_select_window_feel;
	tool_setup_window_feel = ((PaintApplication*)be_app)->Settings()->tool_setup_window_feel;
	palette_window_feel = ((PaintApplication*)be_app)->Settings()->palette_window_feel;
	brush_window_feel = ((PaintApplication*)be_app)->Settings()->brush_window_feel;
	layer_window_feel = ((PaintApplication*)be_app)->Settings()->layer_window_feel;
	add_on_window_feel = ((PaintApplication*)be_app)->Settings()->add_on_window_feel;
	
	BView *p = Parent();
	if (p != NULL) {
		SetViewColor(p->ViewColor());
	}

	BCheckBox *cb;
	BRect cb_frame = BRect(4,4+label_height,4,4+label_height);
	BMessage *cb_message;
	float original_top = cb_frame.top;
	float halfway_width = box->Bounds().Width()/2;
	float bottom = box->Bounds().bottom;
	int32 resizing_mode = B_FOLLOW_TOP | B_FOLLOW_LEFT;
	
	cb_message = new BMessage(HS_WINDOW_FEEL_CHANGED);
	cb_message->AddString("name","color");
	cb = new BCheckBox(cb_frame,"feel box",StringServer::ReturnString(COLOR_WINDOW_STRING),cb_message);	
	box->AddChild(cb);
	cb->ResizeToPreferred();
	cb->SetResizingMode(resizing_mode);
	if (((PaintApplication*)be_app)->Settings()->palette_window_feel == B_FLOATING_APP_WINDOW_FEEL)
		cb->SetValue(B_CONTROL_ON);
	else
		cb->SetValue(B_CONTROL_OFF);

	cb_frame = cb->Frame();
	

	float button_height = cb->Bounds().Height();
	
	if ((cb_frame.bottom + button_height + EXTRA_EDGE) > bottom) {
		cb_frame.OffsetTo(halfway_width,original_top);
		resizing_mode = B_FOLLOW_TOP | B_FOLLOW_RIGHT;
	}
	else {
		cb_frame.OffsetBy(0,button_height + 4);
	}

	cb_message = new BMessage(HS_WINDOW_FEEL_CHANGED);
	cb_message->AddString("name","tool");
	cb = new BCheckBox(cb_frame,"feel box",StringServer::ReturnString(TOOL_SELECTION_WINDOW_STRING),cb_message);	
	box->AddChild(cb);
	cb->ResizeToPreferred();
	cb->SetResizingMode(resizing_mode);
	if (((PaintApplication*)be_app)->Settings()->tool_select_window_feel == B_FLOATING_APP_WINDOW_FEEL)
		cb->SetValue(B_CONTROL_ON);
	else
		cb->SetValue(B_CONTROL_OFF);

	if ((cb_frame.bottom + button_height + EXTRA_EDGE) > bottom) {
		cb_frame.OffsetTo(halfway_width,original_top);
		resizing_mode = B_FOLLOW_TOP | B_FOLLOW_RIGHT;
	}
	else {
		cb_frame.OffsetBy(0,button_height + 4);
	}

	cb_message = new BMessage(HS_WINDOW_FEEL_CHANGED);
	cb_message->AddString("name","tool setup");
	cb = new BCheckBox(cb_frame,"feel box",StringServer::ReturnString(TOOL_SETUP_WINDOW_STRING),cb_message);	
	box->AddChild(cb);
	cb->ResizeToPreferred();
	cb->SetResizingMode(resizing_mode);
	if (((PaintApplication*)be_app)->Settings()->tool_setup_window_feel == B_FLOATING_APP_WINDOW_FEEL)
		cb->SetValue(B_CONTROL_ON);
	else
		cb->SetValue(B_CONTROL_OFF);
	
	if ((cb_frame.bottom + button_height + EXTRA_EDGE) > bottom) {
		cb_frame.OffsetTo(halfway_width,original_top);
		resizing_mode = B_FOLLOW_TOP | B_FOLLOW_RIGHT;
	}
	else {
		cb_frame.OffsetBy(0,button_height + 4);
	}

	cb_message = new BMessage(HS_WINDOW_FEEL_CHANGED);
	cb_message->AddString("name","layer");
	cb = new BCheckBox(cb_frame,"feel box",StringServer::ReturnString(LAYER_WINDOW_STRING),cb_message);	
	box->AddChild(cb);
	cb->ResizeToPreferred();
	cb->SetResizingMode(resizing_mode);
	if (((PaintApplication*)be_app)->Settings()->layer_window_feel == B_FLOATING_APP_WINDOW_FEEL)
		cb->SetValue(B_CONTROL_ON);
	else
		cb->SetValue(B_CONTROL_OFF);

	if ((cb_frame.bottom + button_height + EXTRA_EDGE) > bottom) {
		cb_frame.OffsetTo(halfway_width,original_top);
		resizing_mode = B_FOLLOW_TOP | B_FOLLOW_RIGHT;
	}
	else {
		cb_frame.OffsetBy(0,button_height + 4);
	}

	cb_message = new BMessage(HS_WINDOW_FEEL_CHANGED);
	cb_message->AddString("name","brush");
	cb = new BCheckBox(cb_frame,"feel box",StringServer::ReturnString(BRUSH_WINDOW_STRING),cb_message);	
	box->AddChild(cb);
	cb->ResizeToPreferred();
	cb->SetResizingMode(resizing_mode);
	if (((PaintApplication*)be_app)->Settings()->brush_window_feel == B_FLOATING_APP_WINDOW_FEEL)
		cb->SetValue(B_CONTROL_ON);
	else
		cb->SetValue(B_CONTROL_OFF);

	if ((cb_frame.bottom + button_height + EXTRA_EDGE) > bottom) {
		cb_frame.OffsetTo(halfway_width,original_top);
		resizing_mode = B_FOLLOW_TOP | B_FOLLOW_RIGHT;
	}
	else {
		cb_frame.OffsetBy(0,button_height + 4);
	}
	
	cb_message = new BMessage(HS_WINDOW_FEEL_CHANGED);
	cb_message->AddString("name","add on");
	cb = new BCheckBox(cb_frame,"feel box",StringServer::ReturnString(EFFECTS_WINDOW_STRING),cb_message);	
	box->AddChild(cb);
	cb->ResizeToPreferred();
	cb->SetResizingMode(resizing_mode);
	if (((PaintApplication*)be_app)->Settings()->add_on_window_feel == B_FLOATING_SUBSET_WINDOW_FEEL)
		cb->SetValue(B_CONTROL_ON);
	else
		cb->SetValue(B_CONTROL_OFF);
}

void WindowFeelView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
		
	BView *box = ChildAt(0);
	if (box != NULL) {
		for (int32 i=0;i<box->CountChildren();i++) {
			BControl *control = dynamic_cast<BControl*>(box->ChildAt(i));
			control->SetTarget(this);
		}
	}
}

void WindowFeelView::MessageReceived(BMessage *msg)
{
	BControl *source = NULL;
	switch (msg->what) {
		case HS_WINDOW_FEEL_CHANGED:
			msg->FindPointer("source",(void**)&source);
			if (source != NULL) {
				if (strcmp("color",msg->FindString("name")) == 0) {
					if (source->Value() == B_CONTROL_OFF) {
						palette_window_feel = B_NORMAL_WINDOW_FEEL;
					}
					else
						palette_window_feel = B_FLOATING_APP_WINDOW_FEEL;
				}
				else if (strcmp("tool",msg->FindString("name")) == 0) {
					if (source->Value() == B_CONTROL_OFF) {
						tool_window_feel = B_NORMAL_WINDOW_FEEL;
					}
					else
						tool_window_feel = B_FLOATING_APP_WINDOW_FEEL;
				} 
				else if (strcmp("tool setup",msg->FindString("name")) == 0) {
					if (source->Value() == B_CONTROL_OFF) {
						tool_setup_window_feel = B_NORMAL_WINDOW_FEEL;
					}
					else
						tool_setup_window_feel = B_FLOATING_APP_WINDOW_FEEL;
				} 
				else if (strcmp("layer",msg->FindString("name")) == 0) {
					if (source->Value() == B_CONTROL_OFF) {
						layer_window_feel = B_NORMAL_WINDOW_FEEL;
					}
					else
						layer_window_feel = B_FLOATING_APP_WINDOW_FEEL;
				} 
				else if (strcmp("brush",msg->FindString("name")) == 0) {
					if (source->Value() == B_CONTROL_OFF) {
						brush_window_feel = B_NORMAL_WINDOW_FEEL;
					}
					else
						brush_window_feel = B_FLOATING_APP_WINDOW_FEEL;
				} 
				else if (strcmp("add on",msg->FindString("name")) == 0) {
					if (source->Value() == B_CONTROL_OFF) {
						add_on_window_feel = B_NORMAL_WINDOW_FEEL;
					}
					else
						add_on_window_feel = B_FLOATING_SUBSET_WINDOW_FEEL;
				} 
			}		
			break;
			
		default:
			BView::MessageReceived(msg);
			break;
	}
}



void WindowFeelView::ApplyChanges()
{
	// The functions that are called here will store the settings to the
	// global settings structure.
	ColorPaletteWindow::setFeel(palette_window_feel);
	LayerWindow::setFeel(layer_window_feel);
	ToolSelectionWindow::setFeel(tool_window_feel);
	ToolSetupWindow::setFeel(tool_setup_window_feel);
	BrushStoreWindow::setFeel(brush_window_feel);
	ManipulatorWindow::setFeel(add_on_window_feel);	
}




UndoControlView::UndoControlView(BRect frame)
	: BView(frame,"undo control view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW)
{
	BRect button_frame(10,10,10,10);
	unlimited_depth_button = new BRadioButton(button_frame,"unlimited_depth_button",StringServer::ReturnString(UNLIMITED_STRING),new BMessage(UNLIMITED_UNDO_SET));	
	unlimited_depth_button->ResizeToPreferred();
	AddChild(unlimited_depth_button);
	
	button_frame = unlimited_depth_button->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+10);
	
	adjustable_depth_button = new BRadioButton(button_frame,"adjustable depth button",StringServer::ReturnString(ADJUSTABLE_STRING),new BMessage(ADJUSTABLE_UNDO_SET));
	adjustable_depth_button->ResizeToPreferred();
	AddChild(adjustable_depth_button);
	
	button_frame = adjustable_depth_button->Frame();
	button_frame.OffsetBy(button_frame.Width(),0);
	
	undo_depth_control = new NumberControl(button_frame,"undo_depth_control","100","20",new BMessage(UNDO_DEPTH_ADJUSTED));
	undo_depth_control->ResizeToPreferred();
	undo_depth_control->SetDivider(0);
	AddChild(undo_depth_control);
	undo_depth_control->SetEnabled(FALSE);
	undo_depth_control->SetValue(0);
		
	button_frame = adjustable_depth_button->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+10);
	
	zero_depth_button = new BRadioButton(button_frame,"zero_depth_button",StringServer::ReturnString(OFF_STRING),new BMessage(UNDO_OFF));
	zero_depth_button->ResizeToPreferred();
	AddChild(zero_depth_button);



	if (UndoQueue::ReturnDepth() == INFINITE_QUEUE_DEPTH) 
		unlimited_depth_button->SetValue(B_CONTROL_ON);
		
	if (UndoQueue::ReturnDepth() == 0)
		zero_depth_button->SetValue(B_CONTROL_ON);

	if (UndoQueue::ReturnDepth() > 0) {	
		adjustable_depth_button->SetValue(B_CONTROL_ON);
		undo_depth_control->SetValue(UndoQueue::ReturnDepth());
		undo_depth_control->SetEnabled(TRUE);
	}	

	undo_depth = UndoQueue::ReturnDepth();
}


void UndoControlView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
		
	unlimited_depth_button->SetTarget(this);
	adjustable_depth_button->SetTarget(this);
	zero_depth_button->SetTarget(this);
	undo_depth_control->SetTarget(this);
}

void UndoControlView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case UNLIMITED_UNDO_SET:
			undo_depth_control->SetEnabled(FALSE);
			undo_depth = INFINITE_QUEUE_DEPTH;
			break;
		
		case ADJUSTABLE_UNDO_SET:
			undo_depth_control->SetEnabled(TRUE);
			undo_depth = undo_depth_control->Value(); 
			break;
		
		case UNDO_OFF:
			undo_depth_control->SetEnabled(FALSE);
			undo_depth = 0;
			break;
			
		case UNDO_DEPTH_ADJUSTED:
			undo_depth_control->SetValue(max_c(min_c(undo_depth_control->Value(),100),0));						
			undo_depth = undo_depth_control->Value();
			break;
		
		default:
			BView::MessageReceived(message);
			break;
	}
}

void UndoControlView::ApplyChanges()
{
	UndoQueue::SetQueueDepth(undo_depth);
}



LanguageControlView::LanguageControlView(BRect frame)
	: BView(frame,"language_control_view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW)
{
	BRect button_frame(10,10,10,10);
	language_button_1 = new BRadioButton(button_frame,"language_button_1","English",new BMessage(ENGLISH_LANGUAGE_SET));	
	language_button_1->ResizeToPreferred();
	AddChild(language_button_1);
	
	button_frame = language_button_1->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+10);
	
	language_button_2 = new BRadioButton(button_frame,"language_button_2","Deutsch",new BMessage(GERMAN_LANGUAGE_SET));
	language_button_2->ResizeToPreferred();
	AddChild(language_button_2);
	
	button_frame = language_button_2->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+10);

	language_button_3 = new BRadioButton(button_frame,"language_button_3","Française",new BMessage(FRENCH_LANGUAGE_SET));
	language_button_3->ResizeToPreferred();
	AddChild(language_button_3);
	

	// Add the flags here
	BBitmap *flag_image;
	flag_image = new BBitmap(BRect(0,0,flagWidth-1,flagHeight-1),B_CMAP8);
	flag_image->SetBits(flagBritishBits,flag_image->BitsLength(),0,flagColorSpace);
	BitmapView *flag1 = new BitmapView(flag_image,language_button_1->Frame().RightBottom()+BPoint(4,-language_button_1->Frame().Height()/2-flagHeight/2));
	AddChild(flag1);
	
	flag_image = new BBitmap(BRect(0,0,flagWidth-1,flagHeight-1),B_CMAP8);
	flag_image->SetBits(flagGermanBits,flag_image->BitsLength(),0,flagColorSpace);
	BitmapView *flag2 = new BitmapView(flag_image,language_button_2->Frame().RightBottom()+BPoint(4,-language_button_2->Frame().Height()/2-flagHeight/2));
	AddChild(flag2);


	flag_image = new BBitmap(BRect(0,0,flagWidth-1,flagHeight-1),B_CMAP8);
	flag_image->SetBits(flagFrenchBits,flag_image->BitsLength(),0,flagColorSpace);
	BitmapView *flag3 = new BitmapView(flag_image,language_button_3->Frame().RightBottom()+BPoint(4,-language_button_3->Frame().Height()/2-flagHeight/2));
	AddChild(flag3);



	float flag_left_point = 0;
	flag_left_point = max_c(flag1->Frame().left,flag2->Frame().left);
	
	
	flag1->MoveTo(flag_left_point,flag1->Frame().top);
	flag2->MoveTo(flag_left_point,flag2->Frame().top);
	

	original_language = language = ((PaintApplication*)be_app)->Settings()->language;
	if (language == ENGLISH_LANGUAGE) 
		language_button_1->SetValue(B_CONTROL_ON);
		
	else if (language == GERMAN_LANGUAGE)
		language_button_2->SetValue(B_CONTROL_ON);
		
	else if (language == FRENCH_LANGUAGE)
		language_button_3->SetValue(B_CONTROL_ON);


	message_view = new BStringView(BRect(Bounds().left,Bounds().bottom-20,Bounds().right,Bounds().bottom),"message_view","");
	AddChild(message_view);
}


void LanguageControlView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
		
	language_button_1->SetTarget(this);
	language_button_2->SetTarget(this);
	language_button_3->SetTarget(this);
}

void LanguageControlView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case ENGLISH_LANGUAGE_SET:
			language = ENGLISH_LANGUAGE;
			break;
		
		case FINNISH_LANGUAGE_SET:
			language = FINNISH_LANGUAGE;
			break;
		
		case GERMAN_LANGUAGE_SET:
			language = GERMAN_LANGUAGE;
			break;

		case FRENCH_LANGUAGE_SET:
			language = FRENCH_LANGUAGE;
			
		default:
			BView::MessageReceived(message);
			break;
	}

	if (language != original_language) {
		message_view->SetText(StringServer::ReturnString(CHANGES_TAKE_EFFECT_STRING));
	}
	else {
		message_view->SetText("");
	}
}

void LanguageControlView::ApplyChanges()
{
	((PaintApplication*)be_app)->Settings()->language = language;
}



GeneralControlView::GeneralControlView(BRect frame)
	: BView(frame,"language_control_view",B_FOLLOW_ALL_SIDES,B_WILL_DRAW)
{
	BBox *box;
	BMessage *message;
	BRect button_frame(10,10,10,10);
	message = new BMessage(CURSOR_MODE_CHANGED);
	message->AddInt32("cursor_mode",CROSS_HAIR_CURSOR_MODE);
	cursor_button_1 = new BRadioButton(button_frame,"cursor_button_1",StringServer::ReturnString(CROSS_HAIR_CURSOR_STRING),message);	
	cursor_button_1->ResizeToPreferred();
	
	button_frame = cursor_button_1->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+4);
	
	message = new BMessage(CURSOR_MODE_CHANGED);
	message->AddInt32("cursor_mode",TOOL_CURSOR_MODE);
	cursor_button_2 = new BRadioButton(button_frame,"cursor_button_2",StringServer::ReturnString(TOOL_CURSOR_STRING),message);
	cursor_button_2->ResizeToPreferred();
	
	button_frame = cursor_button_2->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+10);
	
	cursor_mode = ((PaintApplication*)be_app)->Settings()->cursor_mode;
	if (cursor_mode == CROSS_HAIR_CURSOR_MODE) 
		cursor_button_1->SetValue(B_CONTROL_ON);
		
	else if (cursor_mode == TOOL_CURSOR_MODE)
		cursor_button_2->SetValue(B_CONTROL_ON);


	box = new BBox(BRect(5,5,5,5));
	float width = max_c(cursor_button_1->Frame().Width(),cursor_button_2->Frame().Width());
	width += cursor_button_1->Frame().left*2;
	box->ResizeTo(width,cursor_button_2->Frame().bottom+4);
	box->SetLabel(StringServer::ReturnString(CURSOR_STRING));
	AddChild(box);
	box->AddChild(cursor_button_1);
	box->AddChild(cursor_button_2);
	
	button_frame = box->Frame();
	button_frame.OffsetBy(0,button_frame.Height()+4);
	
	quit_confirm_mode = ((PaintApplication*)be_app)->Settings()->quit_confirm_mode;
	quit_confirm_box = new BCheckBox(button_frame,"quit_confirm_box",StringServer::ReturnString(CONFIRM_QUIT_STRING),new BMessage(CONFIRM_MODE_CHANGED));
	quit_confirm_box->ResizeToPreferred();
	AddChild(quit_confirm_box);
	quit_confirm_box->SetValue(quit_confirm_mode);
}


void GeneralControlView::AttachedToWindow()
{
	if (Parent() != NULL)
		SetViewColor(Parent()->ViewColor());
		
	cursor_button_1->SetTarget(this);
	cursor_button_2->SetTarget(this);
	quit_confirm_box->SetTarget(this);
}

void GeneralControlView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case CURSOR_MODE_CHANGED:
			cursor_mode = message->FindInt32("cursor_mode");
			break;
		case CONFIRM_MODE_CHANGED:
			quit_confirm_mode = quit_confirm_box->Value();
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}

void GeneralControlView::ApplyChanges()
{
	((PaintApplication*)be_app)->Settings()->cursor_mode = cursor_mode;
	((PaintApplication*)be_app)->Settings()->quit_confirm_mode = quit_confirm_mode;
}

