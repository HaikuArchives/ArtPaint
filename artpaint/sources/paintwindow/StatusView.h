/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef STATUS_VIEW_H
#define	STATUS_VIEW_H

#include <Box.h>
#include <StringView.h>
#include <View.h>
#include <StatusBar.h>

// this is used to make additional room around views inside statusview
#define HS_STATUS_VIEW_EDGE 4

class ColorContainer;
class SelectedColorsView;
class SelectedToolsView;
class HSPictureButton;
class MagnificationView;

class StatusView : public BView {
		// Here are some string-views.
		BStringView 		*coordinate_view;
		MagnificationView	*mag_state_view;
		BStringView			*message_view;

		// This is the StatusBar that will be used in this status-view.
		BStatusBar	*status_bar;

		// This is the view that has been created by the manipulator
		BView		*manipulator_view;

		// These are the boxes that hold the string-views.
		BBox 	*coordinate_box;
		BBox	*manipulator_box;

		// These are picture-buttons for Cancel- and OK-messages.
		HSPictureButton	*ok_button,*cancel_button;

		// These are views for selected tools and colors
		// and the current color-set.
		ColorContainer		*color_container;
		SelectedColorsView	*selected_colors;


public:
		// Frame only gives right bottom, left is assumed to be at 0
		// and top is moved as much above the bottom as necessary
		StatusView(BRect frame);

		~StatusView();

void	SetCoordinates(BPoint point,BPoint reference,bool use_reference);
void	SetMagnifyingScale(float mag);
void	SetHelpMessage(const char *s) { message_view->SetText(s); }

status_t	DisplayManipulatorView(BView*);
BStatusBar*	DisplayProgressIndicator();
status_t	DisplayToolsAndColors();
status_t	RemoveToolsAndColors();
status_t	DisplayNothing();
};




// this class will display the colors that are currently selected
// they will be linked in a list so that they can all be updated easily
class SelectedColorsView : public BBox {
		// these variables store info about mouse-buttons
//		int32 		button_count;
//		mouse_map 	button_map;
//		BBitmap		*color_pop_up_image;

// a view is added to list in constructor and removed from it in destructor
static	BList	*list_of_views;
		float	foreground_color_percentage;

void	SetHighAndLowColors(const rgb_color&);

bool	IsPointOverForegroundColor(BPoint);
public:
		SelectedColorsView(BRect frame);
		~SelectedColorsView();
void	Draw(BRect);
void	MessageReceived(BMessage *message);
void	MouseDown(BPoint location);
void	MouseMoved(BPoint,uint32, const BMessage*);

// this will be used to send message to all selected colors views
static	void	sendMessageToAll(BMessage *message);
};


//// this class will display the tools that are currently selected
//// they will be linked in a list so that they can all be updated easily
//class SelectedToolsView : public BView {
//		// these variables store info about mouse-buttons
//		int32 button_count;
//		mouse_map button_map;
//
//		// these store the tool-pictures
//		BPicture *left_picture,*right_picture,*middle_picture;
//
//// a view is added to list in constructor and removed from it in destructor
//static	BList *list_of_views;
//
//public:
//		SelectedToolsView(BRect frame);
//		~SelectedToolsView();
//void	Draw(BRect);
//void	MessageReceived(BMessage *message);
//void	MouseDown(BPoint location);
//void	MouseMoved(BPoint,uint32, const BMessage*);
//
//// this will be used to send message to all selected colors views
//static	void	sendMessageToAll(BMessage *message);
//};

#endif
