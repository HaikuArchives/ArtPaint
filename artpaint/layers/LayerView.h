/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef LAYER_VIEW
#define LAYER_VIEW

#include <Box.h>
#include <MenuField.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include "Layer.h"


#define	LAYER_VIEW_HEIGHT	(HS_MINIATURE_IMAGE_HEIGHT+10)


class ThumbnailView : public BView {
public:
				ThumbnailView(BBitmap* image);
	virtual		~ThumbnailView();

	virtual void 	Draw(BRect updateRect);
	virtual void	MessageReceived(BMessage* message);
	void		MouseDown(BPoint);

	BBitmap*	ThumbnailBitmap() { return fThumbnailBitmap; }

	void		Redraw() { Draw(Bounds()); }

private:
	BBitmap*	fThumbnailBitmap;
};


class LayerView : public BBox {
BBitmap	*the_image;		// Not to be deleted.
Layer	*the_layer;		// Not to be deleted.
bool	is_active;

BCheckBox*		visibility_box;
BTextControl*	layer_name_field;
ThumbnailView* 	thumbnail_view;


// The returned value will indicate how many positions we moved
// up (if positive) or down (if negative)
static	int32		reorder_thread(void *data);
		int32		ReorderViews();

public:
			LayerView(BBitmap *image,Layer *layer);
			~LayerView();

void		AttachedToWindow();
void		MessageReceived(BMessage*);
void		MouseDown(BPoint);
void		MouseMoved(BPoint where,uint32 transit,const BMessage*);
void		Draw(BRect);


void		UpdateImage();
void		Activate(bool);
void		SetVisibility(bool);

Layer*		ReturnLayer() { return the_layer; }
};


#endif
