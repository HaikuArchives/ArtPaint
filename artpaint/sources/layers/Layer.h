/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef LAYER_H
#define LAYER_H

#include <GraphicsDefs.h>
#include <OS.h>
#include <String.h>
#include <SupportDefs.h>


class BBitmap;
class BFile;
class BRect;
class BView;
class Image;
class ImageView;
class LayerView;


enum layer_type {
	HS_NORMAL_LAYER		=	'NrLa'
};


// These define the maximum dimensions for miniature-images.
#define	HS_MINIATURE_IMAGE_WIDTH		  65
#define	HS_MINIATURE_IMAGE_HEIGHT		  65


// These are the messages that can be sent from a layer-view to image-view.
#define	HS_LAYER_ACTIVATED				'LaAc'
#define	HS_LAYER_VISIBILITY_CHANGED		'LviC'
#define	HS_LAYER_POSITION_CHANGED		'LpoC'


// these constants are used in controlling the layers
#define HS_ADD_LAYER_FRONT				'AdLf'
#define HS_ADD_LAYER_BEHIND				'AdLb'
#define	HS_DELETE_LAYER					'DelL'
#define	HS_MERGE_WITH_UPPER_LAYER		'MrWu'
#define	HS_MERGE_WITH_LOWER_LAYER		'MrWl'
#define	HS_FADE_WITH_UPPER_LAYER		'FaWu'
#define	HS_FADE_WITH_LOWER_LAYER		'FaWl'
#define	HS_DUPLICATE_LAYER				'DupL'
#define	HS_LAYER_NAME_CHANGED			'LanC'
#define	HS_LAYER_DRAGGED				'Drla'


class Layer {
public:
								Layer(BRect frame, int32 id, BView* imageView,
									int32 type = HS_NORMAL_LAYER,
									BBitmap* bitmap = NULL);
								~Layer();

			BBitmap*			Bitmap() const { return the_bitmap; }

			bool				IsVisible() const { return visibility; }
			void				SetVisibility(bool visible);
			void				ToggleVisibility() { visibility = !visibility; }

			void				AddToImage(Image* image);

			// this function clears the layer to given background-color
			void				Clear(rgb_color color);

			BBitmap*			GetMiniatureImage() const {
									return miniature_image;
								}
			void				ChangeBitmap(BBitmap* new_bitmap);
			LayerView*			GetView() const {
									return layer_view;
								}
			BView*				GetImageView() const {
									return image_view;
								}

			void				ActivateLayer(bool activate);
			bool				IsActive() const { return is_active; }

			int32				Id() const { return layer_id; }

			void				Merge(Layer* top_layer);

			// This static function puts the correct layer to create a miniature
			// image of itself. It creates a new thread.
	static	int32				CreateMiniatureImage(void* data);


			// This static function reads a layer from the parameter file
			// and leaves the file-pointer after the layer. If it does not
			// succeed it returns NULL else it returns a valid Layer-object.
	static	Layer*				readLayer(BFile& file, ImageView* imageView,
									int32 newId, bool littleEndian,
									int32 compressionMethod);
	static	Layer*				readLayerOldStyle(BFile& file,
									ImageView* imageView, int32 newId);
			int64				writeLayer(BFile& file, int32 compressionMethod);

			void				SetName(const char* c) {
									strncpy(layer_name, c, 64);
								}
			const char*			ReturnLayerName() const { return layer_name; }

			const uint32*		ReturnFixedAlphaTable() const {
									return fixed_alpha_table;
								}

			void				SetTransparency(float coefficient);
			float				GetTransparency() const {
									return transparency_coefficient;
								}

			Layer*				ReturnUpperLayer();
			Layer*				ReturnLowerLayer();

			const char*			ReturnProjectName() const;

private:
			// this bitmap holds the actual image-data of this layer
			BBitmap*			the_bitmap;

			// this bitmap holds the miniature image of layers visible area
			BBitmap*			miniature_image;

			// This id identifies the layer within the image-view that it
			// belongs to. It is set in the constructor.
			int32				layer_id;

			// this semaphore is used to guard the access to miniature image
			// and the int32 tells how many threads are waiting to do this job
			// the last one waiting should get to do the job
			sem_id				mini_image_semaphore;
			int32				mini_image_threads_waiting;

			// this tells whether the layer is visible at all
			bool				visibility;
			bool				is_active;

			// this is the type of the layer, HS_NORMAL_LAYER or HS_CONTROL_LAYER
			int32				layer_type;

			// this stores the layer name
			char				layer_name[64];

			// This is the view that owns this layer. All the controls in
			// layer_view should send a message to this view. It will then
			// change the properties for this layer.
			BView*				image_view;

			Image*				the_image;

			// This is the view tha contains layer's miniature representation
			// and some controls for controlling layers properties.
			LayerView*			layer_view;

			uint32				fixed_alpha_table[256];
			float				float_alpha_table[256];

			float				transparency_coefficient;

			// this function calculates the miniature_image
			int32				calc_mini_image();

			// a function to calculate the bounds_rectangle for actual image
			BRect				ImageBounds();
};

#endif
