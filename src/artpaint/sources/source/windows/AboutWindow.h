/* 

	Filename:	AboutWindow.h
	Contents:	AboutWindow-class declaration			
	Author:		Heikki Suhonen
	
*/

#ifndef	ABOUT_WINDOW_H
#define	ABOUT_WINDOW_H

#include <Bitmap.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>


class AboutWindow : public BWindow {
static	BWindow	*the_window;
		AboutWindow(BRect frame);
		~AboutWindow();
public:
static	void	showWindow();		
};




class TextScrollerView : public BView {
	BTextView	*text_view;

		thread_id	updater_thread;
static	int32		updater_entry(void*);
		int32		updater_function();

		bool	continue_updating;
		bool	tracking_mouse;
		bool	display_easter_egg;

		BPoint	previous_point;		

public:
		TextScrollerView(BRect);
		~TextScrollerView();
		
void	AttachedToWindow();		
void	Draw(BRect);
void 	MouseDown(BPoint);

void	AddLine(const char*,rgb_color&,BFont&);		
void	AddEmptyLine();

BTextView*	ReturnTextView() { return text_view; }
};

#endif