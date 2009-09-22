/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef IMAGE_H
#define IMAGE_H


#include <InterfaceDefs.h>
#include <List.h>
#include <Region.h>


class BBitmap;
class Selection;


/*
	The Image-class handles the image. It has the following duties:

		1.	Stores the layers
		2.	Renders a complete picture out of the layers
		3.	Allows undo-mechanism to update the layers
		4.	Calculates thumbnail-images when necessary
		5.	Makes a dithered image if necessary.
*/

class BFile;
class Layer;
class UndoQueue;
class UndoEvent;
class ImageView;


struct color_entry {
	float	probability;
	int8	value;
};

class Image {
		BList	*layer_list;
		Layer	**layer_id_list;

		// This is the index of active layer in layer_list.
		int32	current_layer_index;
		int32	next_layer_id;

		BBitmap	*rendered_image;
		BBitmap	*thumbnail_image;

		BBitmap	*dithered_image;
		BList	*dithered_users;

		bool	dithered_up_to_date;

		float	image_width,image_height;	// these are the real width and height of canvas in pixels



		ImageView	*image_view;
		UndoQueue	*undo_queue;



		void		CalculateThumbnails();
static	int32	calculate_thumbnail_image(void*);


		uint32	full_fixed_alpha;

//		uint8		index_map[32768];
static	rgb_color	*color_list;
static	color_entry	*color_candidates;
static	int32		color_candidate_users;

		int32		number_of_cpus;

static	int32		enter_dither(void*);
		int32		DoDither(BRect);


//static	int32		enter_copy_to_dither(void*);
//		int32		CopyToDitherImage(BRect);

static	int32		candidate_creator(void*);


static	int32		enter_render(void*);
		int32		DoRender(BRect);


static	int32		enter_render_preview(void*);
		int32		DoRenderPreview(BRect,int32);


public:
			Image(ImageView*,float,float,UndoQueue*);
			~Image();

void		Render();
void		Render(BRect);
void		RenderPreview(BRect,int32);
void		RenderPreview(BRegion&,int32);
void		MultiplyRenderedImagePixels(int32);

bool		SetImageSize();

Layer*		AddLayer(BBitmap*,Layer*,bool add_to_front,float layer_transparency_coefficient=1.0);
bool		ChangeActiveLayer(Layer*,int32);
bool		ChangeLayerPosition(Layer*,int32,int32);
bool		ClearCurrentLayer(rgb_color&);
bool		ClearLayers(rgb_color&);
bool		DuplicateLayer(Layer*,int32);
bool		MergeLayers(Layer*,int32,bool merge_with_upper);
bool		RemoveLayer(Layer*,int32);
bool		ToggleLayerVisibility(Layer*,int32);


Layer*		ReturnUpperLayer(Layer*);
Layer*		ReturnLowerLayer(Layer*);

status_t	InsertLayer(BBitmap *layer_bitmap = NULL);

BList*		LayerList() { return layer_list; }

void		UpdateImageStructure(UndoEvent*);

void		RegisterLayersWithUndo();

BBitmap*	ReturnThumbnailImage();
BBitmap*	ReturnRenderedImage();
BBitmap*	ReturnActiveBitmap();
Layer*		ReturnActiveLayer() { return (Layer*)layer_list->ItemAt(current_layer_index); }

float		Width() { return image_width; }
float		Height() { return image_height; }

bool		ContainsLayer(Layer*);

status_t	ReadLayers(BFile&);
int64		WriteLayers(BFile&);
status_t	ReadLayersOldStyle(BFile&,int32);

status_t	RegisterDitheredUser(void*);
status_t	UnregisterDitheredUser(void*);
BBitmap*	ReturnDitheredImage() { return dithered_image; }
bool		IsDitheredUpToDate() { return dithered_up_to_date; }
};
#endif
