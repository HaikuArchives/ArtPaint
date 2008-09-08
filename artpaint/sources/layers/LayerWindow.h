/*

	Filename:	LayerWindow.h
	Contents:	LayerWindow class declaration + some of it's child views
	Author:		Heikki Suhonen

*/

#ifndef	LAYER_WINDOW_H
#define	LAYER_WINDOW_H

#include <Window.h>
#include <StringView.h>

class BitmapView;
class LayerListView;

// this class displays all the layers of one image at a time in a window
// the same window is used to display layers for all images
class LayerWindow : public BWindow {
				LayerListView			*list_view;		// Layer representations will be added
												// as children to this view.
				int32					layer_count;

//				BScrollView		*scroll_view;
				BScrollBar		*scroll_bar;
				BitmapView		*bitmap_view;
				BStringView		*title_view;


				// this is the paint-window from which we display the image
static			BWindow			*target_window;

static			BList			*target_list;	// In this list are the layer-item views
												// of all layers. They will be add as children
												// to list_view.

static			BBitmap			*composite_image;
static	const	char			*window_title;
//static			bool			updates_permitted;

static			sem_id			layer_window_semaphore;

static			LayerWindow		*layer_window;

				LayerWindow(BRect frame);
				~LayerWindow();

void			Update();
public:
		void	MessageReceived(BMessage *message);
		bool	QuitRequested();


static	void	ActiveWindowChanged(BWindow *active_window,BList *list=NULL,BBitmap *composite=NULL);
static	void	showLayerWindow();
static	void	setFeel(window_feel);
};



class LayerListView : public BView {

public:
		LayerListView();
		~LayerListView();

void		DetachedFromWindow();
void	FrameResized(float,float);
};


#endif
