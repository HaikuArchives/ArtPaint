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
#define HS_LAYER_TRANSPARENCY_CHANGED	'LtXc'
#define HS_LAYER_BLEND_MODE_CHANGED		'LbmC'


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
								Layer(BRect frame, int32 id,
									ImageView* imageView,
									layer_type type = HS_NORMAL_LAYER,
									BBitmap* bitmap = NULL,
									BRect* offset = NULL);
								~Layer();

			BBitmap*			Bitmap() const { return fLayerData; }

			bool				IsVisible() const { return fLayerVisible; }
			void				SetVisibility(bool visible);
			void				ToggleVisibility() { fLayerVisible = !fLayerVisible; }

			void				AddToImage(Image* image);

			// this function clears the layer to given background-color
			void				Clear(rgb_color color);

			BBitmap*			GetMiniatureImage() const {
									return fLayerPreview;
								}
			void				ChangeBitmap(BBitmap* new_bitmap);
			LayerView*			GetView() const {
									return fLayerView;
								}
			ImageView*			GetImageView() const {
									return fImageView;
								}

			void				ActivateLayer(bool activate);
			bool				IsActive() const { return fLayerActive; }

			int32				Id() const { return fLayerId; }

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

			void				SetName(const char* name);
			const char*			ReturnLayerName() const { return fLayerName.String(); }

			const uint32*		ReturnFixedAlphaTable() const {
									return fixed_alpha_table;
								}

			void				SetTransparency(float coefficient,
									bool update_old = TRUE);
			float				GetTransparency() const {
									return transparency_coefficient;
								}
			float				GetOldTransparency() const {
									return old_transparency_coefficient;
								}

			Layer*				ReturnUpperLayer();
			Layer*				ReturnLowerLayer();

			const char*			ReturnProjectName() const;

			void				SetBlendMode(uint8 newBlendMode)
									{ fBlendMode = newBlendMode; }
			uint8				GetBlendMode() { return fBlendMode; }
private:
			// this bitmap holds the actual image-data of this layer
			BBitmap*			fLayerData;

			// this bitmap holds the miniature image of layers visible area
			BBitmap*			fLayerPreview;

			// This id identifies the layer within the image-view that it
			// belongs to. It is set in the constructor.
			int32				fLayerId;

			// this semaphore is used to guard the access to miniature image
			// and the int32 tells how many threads are waiting to do this job
			// the last one waiting should get to do the job
			sem_id				fLayerPreviewSem;
			int32				fLayerPreviewThreads;

			// this tells whether the layer is visible at all
			bool				fLayerVisible;
			bool				fLayerActive;

			// this is the type of the layer, HS_NORMAL_LAYER or HS_CONTROL_LAYER
			layer_type			fLayerType;

			// this stores the layer name
			BString				fLayerName;

			// This is the view that owns this layer. All the controls in
			// fLayerView should send a message to this view. It will then
			// change the properties for this layer.
			Image*				fImage;
			ImageView*			fImageView;

			// This is the view tha contains layer's miniature representation
			// and some controls for controlling layers properties.
			LayerView*			fLayerView;

			uint32				fixed_alpha_table[256];
			float				float_alpha_table[256];

			float				transparency_coefficient;
			float				old_transparency_coefficient;

			// this function calculates the fLayerPreview
			int32				calc_mini_image();

			uint8				fBlendMode;
};

#endif
