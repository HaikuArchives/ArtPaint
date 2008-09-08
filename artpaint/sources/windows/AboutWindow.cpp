/*

	Filename:	AboutWindow.cpp
	Contents:	AboutWindow-class definitions
	Author:		Heikki Suhonen

*/

#include <stdio.h>

#include "AboutWindow.h"
#include "StringServer.h"
#include "MessageFilters.h"
#include "PaintApplication.h"

BWindow* AboutWindow::the_window = NULL;

AboutWindow::AboutWindow(BRect frame)
	:	BWindow(frame,"about_window",B_TITLED_WINDOW_LOOK,B_NORMAL_WINDOW_FEEL,B_NOT_RESIZABLE|B_NOT_ZOOMABLE)
{
	char string[256];
	sprintf(string,"%s",StringServer::ReturnString(ABOUT_ARTPAINT_STRING));
	SetTitle(string);

	ResizeTo(300,200);

	TextScrollerView *scroller = new TextScrollerView(Bounds());

	rgb_color yellow = { 255,255,0,255 };
	rgb_color white = { 255,255,255,255 };
	rgb_color red = { 255,0,0,255 };

	BFont title_font;
	title_font.SetSize(title_font.Size()*1.616);

	BFont subtitle_font;

	BFont text_font;

	BFont italic_font;
	italic_font.SetFace(B_ITALIC_FACE);

	scroller->AddLine("ArtPaint v. 2.0",yellow,title_font);
	sprintf(string,StringServer::ReturnString(RELEASE_DATE_STRING),"March 2003");
	scroller->AddLine(string,white,italic_font);
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_1_TEXT_STRING),white,text_font);
	scroller->AddEmptyLine();

	scroller->AddLine(StringServer::ReturnString(ABOUT_3_TEXT_STRING),white,text_font);
	scroller->AddLine("http://www.beunited.org/index.php?page=developer",red,text_font);
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_4_TEXT_STRING),white,text_font);
	scroller->AddLine("contact@beunited.org",red,text_font);
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_5_TEXT_STRING),yellow,subtitle_font);
	scroller->AddLine("Heikki Suhonen",white,italic_font);
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_6_TEXT_STRING),yellow,subtitle_font);
	scroller->AddLine("Heikki Suhonen",white,italic_font);
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_7_TEXT_STRING),yellow,subtitle_font);
	scroller->AddLine("Rainer Riedl",white,italic_font);
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_10_TEXT_STRING),yellow,subtitle_font);
	scroller->AddLine("Jean-Rémi Taponier",white,italic_font);
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_8_TEXT_STRING),yellow,subtitle_font);
	scroller->AddLine("Esa Kallioniemi",white,italic_font);
	scroller->AddLine("Rainer Riedl",white,italic_font);
	scroller->AddLine("Be Inc. and Be Europe",white,italic_font);
	scroller->AddLine("All Registered Users",white,italic_font);
	scroller->AddLine("BeDevTalk List",white,italic_font);
	scroller->AddLine("Dominic Giampaolo",white,italic_font);
	scroller->AddEmptyLine();
	scroller->AddLine("...and You",white,italic_font);

	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddEmptyLine();
	scroller->AddLine(StringServer::ReturnString(ABOUT_9_TEXT_STRING),yellow,subtitle_font);
	scroller->AddLine("William Barret",white,italic_font);
	scroller->AddLine("Paul Haeberli",white,italic_font);
	scroller->AddLine("Aaron Herzmann",white,italic_font);
	scroller->AddLine("Peter Litwinowicz",white,italic_font);
	scroller->AddLine("Eric Mortensen",white,italic_font);
	scroller->AddLine("Ken Perlin",white,italic_font);
	scroller->AddLine("Steve Strassmann",white,italic_font);
	scroller->AddLine("Xiaolin Wu",white,italic_font);

	AddChild(scroller);
	the_window = this;

	Show();
}

AboutWindow::~AboutWindow()
{
	the_window = NULL;
}

void AboutWindow::showWindow()
{
	if (the_window != NULL) {
		the_window->Lock();
		if (the_window->IsHidden()) {
			the_window->Show();
		}
		else {
			the_window->Activate();
		}
		the_window->Unlock();
	}
	else {
		new AboutWindow(BRect(200,200,200,200));
	}
}





TextScrollerView::TextScrollerView(BRect frame)
	:	BView(frame,"text_scroller_view",B_FOLLOW_NONE,B_WILL_DRAW)
{
	frame.OffsetTo(0,0);

	text_view = new BTextView(frame,"text_view",frame,B_FOLLOW_NONE,B_WILL_DRAW);
	text_view->MakeEditable(false);
	text_view->MakeSelectable(false);
	text_view->SetStylable(true);
	text_view->SetViewColor(0,0,0,255);
	text_view->SetAlignment(B_ALIGN_CENTER);

	AddChild(text_view);
	SetViewColor(B_TRANSPARENT_32_BIT);

	tracking_mouse = false;
	display_easter_egg = false;

	BMessageFilter *parent_filter = new BMessageFilter(B_ANY_DELIVERY,B_ANY_SOURCE,B_MOUSE_DOWN,message_to_parent);
	text_view->AddFilter(parent_filter);
}


TextScrollerView::~TextScrollerView()
{
	if (continue_updating) {
		continue_updating = false;
		tracking_mouse = false;
		suspend_thread(updater_thread);
		snooze(1000);
		int32 return_value;
		wait_for_thread(updater_thread,&return_value);
	}
}


void TextScrollerView::AttachedToWindow()
{
	updater_thread = spawn_thread(&updater_entry,"TextScrollerView updater_thread",B_NORMAL_PRIORITY,this);
	continue_updating = true;
	resume_thread(updater_thread);
}

void TextScrollerView::Draw(BRect)
{
}


void TextScrollerView::MouseDown(BPoint point)
{
	tracking_mouse = true;
	uint32 buttons;
	GetMouse(&point,&buttons);
	previous_point = point;
	while (buttons) {
		float distance = (point.y - previous_point.y) / 10.0;
		if (distance > 0)
			distance = min_c(distance,5.0);
		else
			distance = max_c(distance,-5.0);

		LockLooper();
		text_view->ScrollBy(0,-distance);
		GetMouse(&point,&buttons);
		UnlockLooper();
		snooze(20 * 1000);
	}
	tracking_mouse = false;
}

void TextScrollerView::AddLine(const char *text,rgb_color &c,BFont &f)
{
	text_run_array run_array;
	run_array.count = 1;
	run_array.runs[0].offset = 0;
	run_array.runs[0].color = c;
	run_array.runs[0].font = f;

	if (LockLooper()) {
		text_view->Insert(text,&run_array);
		text_view->Insert("\n");
		UnlockLooper();
	}
	else {
		text_view->Insert(text,&run_array);
		text_view->Insert("\n");
	}
}

void TextScrollerView::AddEmptyLine()
{
	if (LockLooper()) {
		text_view->Insert("\n");
		UnlockLooper();
	}
	else {
		text_view->Insert("\n");
	}
}


int32 TextScrollerView::updater_entry(void *data)
{
	TextScrollerView *view = (TextScrollerView*)data;

	return view->updater_function();
}



int32 TextScrollerView::updater_function()
{
	float height;
	if (LockLooper()) {
		height = Bounds().Height();
		text_view->ScrollTo(0,-height);
		text_view->Sync();
		UnlockLooper();
	}
	else {
		height = Bounds().Height();
	}
	float total_height = text_view->TextHeight(0,text_view->CountLines());

	while (continue_updating) {
		if (LockLooper()) {
			if ((text_view->Bounds().top <= total_height) && (text_view->Bounds().top >= -height)) {
				text_view->ScrollBy(0,1);
			}
			else {
				text_view->ScrollTo(0,-height);
			}
			text_view->Sync();
			UnlockLooper();
		}
		snooze(60 * 1000);
		while ((tracking_mouse) && (continue_updating)) {
			snooze(250 * 1000);	// sleep 1/4 seconds
		}
	}
}
