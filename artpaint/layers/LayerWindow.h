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
#include "PixelOperations.h"


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
				BMenu*			blend_mode_menu;

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
static	void	SetTransparency(int32 value);
static	void	SetBlendMode(uint8 mode);

		NumberSliderControl* GetTransparencySlider() { return transparency_slider; }
		BMenu*	GetBlendModeMenu() { return blend_mode_menu; }
};



class LayerListView : public BView {

public:
		LayerListView();
		~LayerListView();

void		DetachedFromWindow();
void	FrameResized(float,float);
};


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PixelOperations"


inline const char* mode_to_string(BlendModes mode)
{
	switch(mode) {
		case BLEND_NORMAL:
			return B_TRANSLATE("Normal");
		case BLEND_MULTIPLY:
			return B_TRANSLATE("Multiply");
		case BLEND_DIVIDE:
			return B_TRANSLATE("Divide");
		case BLEND_SCREEN:
			return B_TRANSLATE("Screen");
		case BLEND_OVERLAY:
			return B_TRANSLATE("Overlay");
		case BLEND_DARKEN:
			return B_TRANSLATE("Darken");
		case BLEND_LIGHTEN:
			return B_TRANSLATE("Lighten");
		case BLEND_DODGE:
			return B_TRANSLATE("Color dodge");
		case BLEND_BURN:
			return B_TRANSLATE("Color burn");
		case BLEND_LINEAR_DODGE:
			return B_TRANSLATE("Linear dodge");
		case BLEND_LINEAR_BURN:
			return B_TRANSLATE("Linear burn");
		case BLEND_HARD_LIGHT:
			return B_TRANSLATE("Hard light");
		case BLEND_SOFT_LIGHT:
			return B_TRANSLATE("Soft light");
		case BLEND_VIVID_LIGHT:
			return B_TRANSLATE("Vivid light");
		case BLEND_LINEAR_LIGHT:
			return B_TRANSLATE("Linear light");
		case BLEND_PIN_LIGHT:
			return B_TRANSLATE("Pin light");
		case BLEND_HARD_MIX:
			return B_TRANSLATE("Hard mix");
		case BLEND_DIFFERENCE:
			return B_TRANSLATE("Difference");
		case BLEND_EXCLUSION:
			return B_TRANSLATE("Exclusion");
		case BLEND_DISSOLVE:
			return B_TRANSLATE("Dissolve");
		case BLEND_HUE:
			return B_TRANSLATE("Hue");
		case BLEND_SATURATION:
			return B_TRANSLATE("Saturation");
		case BLEND_LIGHTNESS:
			return B_TRANSLATE("Lightness");
		case BLEND_COLOR:
			return B_TRANSLATE("Color");
	}

	return "";
}


#endif
