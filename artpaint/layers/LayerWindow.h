/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	LAYER_WINDOW_H
#define	LAYER_WINDOW_H

#include <GridLayout.h>
#include <StringView.h>
#include <Window.h>

#include "Box.h"
#include "Layer.h"
#include "Menu.h"


class BitmapView;
class LayerListView;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


// this class displays all the layers of one image at a time in a window
// the same window is used to display layers for all images
class LayerWindow : public BWindow {
				LayerListView*			list_view;		// Layer representations will be added
												// as children to this view.
				int32					layer_count;

				Layer*			active_layer;

//				BScrollView*	scroll_view;
				BScrollBar*		scroll_bar;
				BStringView*	title_view;
				NumberSliderControl*
								transparency_slider;
				BBox*			top_part;
				BMenu*			layer_operation_menu;

				// this is the paint-window from which we display the image
static			BWindow*		target_window;

static			BList*			target_list;	// In this list are the layer-item views
												// of all layers. They will be add as children
												// to list_view.

static	const	char*			window_title;
//static			bool			updates_permitted;

static			sem_id			layer_window_semaphore;

static			LayerWindow*	layer_window;

				LayerWindow(BRect frame);
				~LayerWindow();

void			Update();
public:
		void	MessageReceived(BMessage *message);
		bool	QuitRequested();
		void	SetActiveLayer(Layer* layer);

static	void	ActiveWindowChanged(BWindow *active_window,BList *list=NULL,BBitmap *composite=NULL);
static	void	showLayerWindow();
static	void	setFeel(window_feel);
		void 	FrameResized(float, float);
		LayerListView*	GetListView() { return list_view; }
};



class LayerListView : public BView {

public:
		LayerListView();
		~LayerListView();

void		DetachedFromWindow();
void	FrameResized(float,float);
};


#endif
