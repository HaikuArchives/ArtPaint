/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef STATUS_VIEW_H
#define	STATUS_VIEW_H

#include <Bitmap.h>
#include <Box.h>
#include <StringView.h>
#include <View.h>


#include "Brush.h"


class BCardLayout;
class BGridLayout;
class BGroupLayout;
class BStatusBar;
class ColorContainer;
class SelectedColorsView;
class CurrentBrushView;
class HSPictureButton;
class MagnificationView;


class StatusView : public BView {
public:
								StatusView();
	virtual						~StatusView();

			void				SetMagnifyingScale(float mag);
			void				SetHelpMessage(const char* s);
			void				SetCoordinates(BPoint point, BPoint reference,
									bool use_reference);

			BStatusBar*			DisplayProgressIndicator();
			status_t			DisplayToolsAndColors();

private:
			BStringView*		coordinate_view;
			MagnificationView*	mag_state_view;
			BStringView*		fHelpView;

			// This is the StatusBar that will be used in this status-view.
			BStatusBar*			status_bar;

			// These are the boxes that hold the string-views.
			BBox*				coordinate_box;

			// These are views for selected tools and colors
			// and the current color-set.
			ColorContainer*		color_container;
			SelectedColorsView*	selected_colors;
			CurrentBrushView*	current_brush;

			BCardLayout*		fCardLayout;
			BGridLayout*		fStatusView;
};


// this class will display the colors that are currently selected
// they will be linked in a list so that they can all be updated easily
class SelectedColorsView : public BBox {
public:
								SelectedColorsView(BRect frame);
	virtual						~SelectedColorsView();

	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint location);
	virtual	void				MouseMoved(BPoint where, uint32 tranist,
									const BMessage* message);

			// this will be used to send message to all selected colors views
	static	void				SendMessageToAll(uint32 what);
	static	void				sendMessageToAll(BMessage* message);

private:
			bool	IsPointOverForegroundColor(BPoint where);
			void	SetHighAndLowColors(const rgb_color& color);

private:
			float				foreground_color_percentage;
	static	BList				list_of_views;
};


class CurrentBrushView : public BView {
public:
								CurrentBrushView(BRect frame);
	virtual						~CurrentBrushView();

	virtual void				Draw(BRect updateRect);
	virtual void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint location);

			void				SetBrush(Brush* new_brush);

	static	void				SendMessageToAll(uint32 what);
	static	void				sendMessageToAll(BMessage* message);
private:
			Brush*				fBrush;
			BBitmap*			fBrushPreview;
	static	BList				list_of_views;
};

#endif	// STATUS_VIEW_H
