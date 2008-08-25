/* 

	Filename:	Layer.h
	Contents:	Layer-class declaration and constants	
	Author:		Heikki Suhonen
	
*/

#ifndef LAYER_H
#define LAYER_H

#include <InterfaceDefs.h>
#include <OS.h>
#include <string.h>
#include <SupportDefs.h>

enum layer_type {
	HS_NORMAL_LAYER		=	'NrLa'
};


// These define the maximum dimensions for miniature-images.
#define	HS_MINIATURE_IMAGE_WIDTH	65
#define	HS_MINIATURE_IMAGE_HEIGHT	65

// These are the messages that can be sent from a layer-view to image-view.
#define	HS_LAYER_ACTIVATED			'LaAc'
#define	HS_LAYER_VISIBILITY_CHANGED	'LviC'
#define	HS_LAYER_POSITION_CHANGED	'LpoC'

// these constants are used in controlling the layers
#define HS_ADD_LAYER_FRONT			'AdLf'
#define HS_ADD_LAYER_BEHIND			'AdLb'
#define	HS_DELETE_LAYER				'DelL'
#define	HS_MERGE_WITH_UPPER_LAYER	'MrWu'
#define	HS_MERGE_WITH_LOWER_LAYER	'MrWl'
#define	HS_FADE_WITH_UPPER_LAYER	'FaWu'
#define	HS_FADE_WITH_LOWER_LAYER	'FaWl'
#define	HS_DUPLICATE_LAYER			'DupL'
#define	HS_LAYER_NAME_CHANGED		'LanC'
#define	HS_LAYER_DRAGGED			'Drla'

class ImageView;
class LayerView;
class Image;

// The class layer inherits from BListItem so that it can be used
// in a list-view also. This should change, no point in inheriting
// from a list view just because of that. And the listview represenntation
// is not very good anyway.
class Layer {
private:	
	// this bitmap holds the actual image-data of this layer
	BBitmap	*the_bitmap;
//	BView	*bitmap_view;
	// this bitmap holds the miniature image of layers visible area
	BBitmap	*miniature_image;

	// This id identifies the layer within the image-view that it belongs to.
	// It is set in the constructor.
	int32	layer_id;	

	// this semaphore is used to guard the access to miniature image
	// and the int32 tells how many threads are waiting to do this job
	// the last one waiting should get to do the job
	sem_id	mini_image_semaphore;
	int32	mini_image_threads_waiting;

	// this tells whether the layer is visible at all
	bool	visibility;
	bool	is_active;
	
	// this is the type of the layer, HS_NORMAL_LAYER or HS_CONTROL_LAYER
	int32	layer_type;
	
	// this stores the layer name
	char	layer_name[64];

	// This is the view that owns this layer. All the controls in layer_view
	// should send a message to this view. It will then change the properties
	// for this layer.
	BView	*image_view;


	Image	*the_image;
	
	// This is the view tha contains layer's miniature representation
	// and some controls for controlling layers properties.
	LayerView	*layer_view;
		
	uint32		fixed_alpha_table[256];
	float		float_alpha_table[256];
		
	float		transparency_coefficient;

// this function calculates the miniature_image
int32	calc_mini_image();


//// these functions perform the basic manipulation operations on individual layers
//void	Crop(BRect visible_area);

// this function calculates the bounds_rectangle for actual image date
BRect	ImageBounds();
	
public:
	Layer(BRect frame,int32 id,BView *image_v,int32 type=HS_NORMAL_LAYER,BBitmap *a_bitmap=NULL);
	~Layer();

BBitmap*	Bitmap()	{ return the_bitmap; }
bool		IsVisible()	{ return visibility; }
void		SetVisibility(bool);

void		AddToImage(Image*);


// this function clears the layer to given background-color
void	Clear(rgb_color color);


void			ToggleVisibility() { visibility = !visibility; }

BBitmap*		GetMiniatureImage() { return miniature_image; }
void			ChangeBitmap(BBitmap *new_bitmap);
LayerView*		GetView() { return layer_view; }
BView*			GetImageView() { return image_view; }

void			ActivateLayer(bool);
bool			IsActive() { return is_active; }
int32			Id() { return layer_id; }

void			Merge(Layer *top_layer);

// This static function puts the correct layer to create a miniature
// image of itself. It creates a new thread.
static	int32	CreateMiniatureImage(void *data);


// This static function reads a layer from the parameter file
// and leaves the file-pointer after the layer. If it does not
// succeed it returns NULL else it returns a valid Layer-object.
static	Layer*		readLayer(BFile &file,ImageView *image_v,int32 new_id,bool is_little_endian,int32 compression_method);
static	Layer*		readLayerOldStyle(BFile &file,ImageView *image_v,int32 new_id);
		int64		writeLayer(BFile &file,int32 compression_method);

		void		SetName(const char *c) {strncpy(layer_name,c,64);}

const	uint32*		ReturnFixedAlphaTable() { return fixed_alpha_table; }


		void		SetTransparency(float);
		float		GetTransparency() { return transparency_coefficient; }
		
		Layer*		ReturnUpperLayer();
		Layer*		ReturnLowerLayer();

const	char*		ReturnProjectName();
const	char*		ReturnLayerName() { return layer_name; }
};

#endif