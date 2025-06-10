/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef BRUSH_STORE_WINDOW_H
#define BRUSH_STORE_WINDOW_H

#include <View.h>
#include <Window.h>

class Brush;
class BFile;


#define HS_DELETE_SELECTED_BRUSH	'Dslb'

class BrushStoreView : public BView {
			int32 		in_a_row;

			BList*		brush_images;
			BList*		brush_data;
			int32		selected_brush_index;
			int32		previous_brush_index;

			BRect		get_bitmap_frame(int32);
			int32		get_point_index(BPoint);

public:
						BrushStoreView();
						~BrushStoreView();

			void		DetachedFromWindow();
			void		Draw(BRect);
			void		FrameResized(float, float);
			void		MessageReceived(BMessage*);
			void		KeyDown(const char* bytes, int32);
			void		MouseDown(BPoint);

			bool		AddBrush(Brush*, bool notify = true);
};


class BrushStoreWindow : public BWindow {

			BScrollBar*	scroll_bar;
			BrushStoreView*	store_view;

	static	BList*		brush_data;

	static	BrushStoreWindow*	brush_window;

	static	int32		brush_adder(void*);

public:
						BrushStoreWindow();
						~BrushStoreWindow();

			void		MessageReceived(BMessage*);

	static	void		writeBrushes(BFile&);
	static	void		readBrushes(BFile&);
	static	void		AddBrush(Brush* brush);
	static	void		DeleteBrush(int32);

	static	void		showWindow();

	static	void		setFeel(window_feel);
};


#endif // BRUSH_STORE_WINDOW_H
