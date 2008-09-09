/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <File.h>
#include <new.h>
#include <stdio.h>

#include "Layer.h"
#include "LayerView.h"
#include "MessageConstants.h"
#include "PaintApplication.h"
#include "ImageView.h"
#include "Selection.h"
#include "UtilityClasses.h"
#include "ProjectFileFunctions.h"
#include "Image.h"

Layer::Layer(BRect frame,int32 id,BView *image_v,int32 type, BBitmap *a_bitmap)
{
	// NULL the_bitmap
	the_bitmap = NULL;
	the_image = NULL;

	// First record the layer type
	layer_type = type;

	// Record the information from view.
	image_view = image_v;
	layer_id = id;
	is_active = FALSE;

	sprintf(layer_name, "Layer %ld", layer_id);

	// ensure that the frame is in correct position
	frame.OffsetTo(0,0);

	// the layer is of course visible
	visibility = TRUE;


	if (layer_type == HS_NORMAL_LAYER) {
		// we have to create a normal layer, therefore we create
		// the bitmap at first
		// if a bitmap was loaded we have to create bitmap that is at least that big
		uint32 *bitmap_bits;
		int32 bits_length;

		if ((a_bitmap != NULL) && (a_bitmap->IsValid())) {
			// change the frame size
			frame.right = (max_c(frame.right,a_bitmap->Bounds().right));
			frame.bottom = (max_c(frame.bottom,a_bitmap->Bounds().bottom));
		}
		the_bitmap = new BBitmap(frame,B_RGB_32_BIT);
		if (the_bitmap->IsValid() == FALSE) {
			throw bad_alloc();
		}

		// Fill the bitmap with wanted initial color.
		bitmap_bits = (uint32*)the_bitmap->Bits();
		bits_length = the_bitmap->BitsLength()/4;

		union {
			char bytes[4];
			uint32 word;
		} color;
		color.bytes[0] = 0xFF;
		color.bytes[1] = 0xFF;
		color.bytes[2] = 0xFF;
		color.bytes[3] = 0x00;
		for (int32 i=0;i<bits_length;i++) {
			*bitmap_bits++ = color.word;
		}

		if ((a_bitmap != NULL) && (a_bitmap->IsValid() == TRUE)) {
			uint32 *target_bits = (uint32*)the_bitmap->Bits();
			uint32 *source_bits = (uint32*)a_bitmap->Bits();
			int32 target_bpr = the_bitmap->BytesPerRow()/4;
			int32 source_bpr = a_bitmap->BytesPerRow()/4;
			int32 width = (int32)min_c(the_bitmap->Bounds().Width(),a_bitmap->Bounds().Width());
			int32 height = (int32)min_c(the_bitmap->Bounds().Height(),a_bitmap->Bounds().Height());

			for (int32 y=0;y<=height;y++) {
				for (int32 x=0;x<=width;x++) {
					*(target_bits + x + y*target_bpr) = *(source_bits + x + y*source_bpr);
				}
			}
		}

		// create the miniature image for this layer and a semaphore for it
		miniature_image = new BBitmap(BRect(0,0,HS_MINIATURE_IMAGE_WIDTH - 1,HS_MINIATURE_IMAGE_HEIGHT - 1),B_RGB_32_BIT);
		mini_image_semaphore = create_sem(1,"mini image semaphore");
		mini_image_threads_waiting = 0;
	}

	layer_view = new LayerView(miniature_image,this);


	SetTransparency(1.0);
}


void Layer::AddToImage(Image *im)
{
	the_image = im;
}

void Layer::SetTransparency(float coeff)
{
	transparency_coefficient = coeff;
	for (int i=0;i<256;i++) {
		float a = (float)i/255.0*transparency_coefficient;
		float_alpha_table[i] = a;

		a *= 32768;
		fixed_alpha_table[i] = (uint32)a;
	}
}


Layer::~Layer()
{
	delete layer_view;

	// free the bitmap and mask and other allocated things if required
	if (the_bitmap != NULL) {
		delete the_bitmap;
		delete miniature_image;
	}
}

void Layer::Clear(rgb_color color)
{
	// This will copy the color (including alpha) to every pixel in this layer.
	// If the selection is not empty, the color will be copied only to the selected
	// points
	Selection *selection = ((ImageView*)image_view)->GetSelection();

	// we will copy the color to this in correct order
	uint32 color_bits = RGBColorToBGRA(color);


	uint32 *bits = (uint32*)the_bitmap->Bits();
	int32 bitslength = the_bitmap->BitsLength()/4;
	if (selection->IsEmpty()) {
		for (int32 i=0;i<bitslength;i++)
			*bits++ = color_bits;
	}
	else {
		BRect bounds = selection->GetBoundingRect();
		int32 left = (int32)bounds.left;
		int32 top = (int32)bounds.top;
		int32 right = (int32)bounds.right;
		int32 bottom = (int32)bounds.bottom;
		int32 bpr = the_bitmap->BytesPerRow()/4;
		for (int32 y=top;y<=bottom;y++) {
			for (int32 x=left;x<=right;x++) {
				if (selection->ContainsPoint(x,y))
					*(bits + x + y*bpr) = color_bits;
			}
		}
	}
}

int32 Layer::CreateMiniatureImage(void *data)
{
	// first take a this pointer
	Layer *this_layer = (Layer*)data;

	// then call the function that creates the actual miniature image
	// and return it's value
	if (this_layer != NULL)
		return this_layer->calc_mini_image();
	else
		return B_ERROR;
}


int32 Layer::calc_mini_image()
{
	// This function might crash if it is executing while the layer is
	// destroyed. This is because the bitmap and miniature image are destroyed.
	// We need something to stop this thread when the layer is being destroyed.

	union {
		uint8 bytes[4];
		uint32 word;
	} white,color;

	white.bytes[0] = 0xFF;
	white.bytes[1] = 0xFF;
	white.bytes[2] = 0xFF;
	white.bytes[3] = 0x00;

	// increase the number of waiting threads
	atomic_add(&mini_image_threads_waiting,1);
	// aquire the semaphore that is required to access the miniature_image
	acquire_sem(mini_image_semaphore);
	// decrease the number of waiting threads
	atomic_add(&mini_image_threads_waiting,-1);

	int32 miniature_width = (int32)(HS_MINIATURE_IMAGE_WIDTH * (min_c(the_bitmap->Bounds().Width()/the_bitmap->Bounds().Height(),1)));
	int32 miniature_height = (int32)(HS_MINIATURE_IMAGE_HEIGHT * (min_c(the_bitmap->Bounds().Height()/the_bitmap->Bounds().Width(),1)));

	// Here we copy the contents of the_bitmap to miniature image.
	// by using a DDA-scaling algorithm first take the dx and dy variables
	float dx = 	(the_bitmap->Bounds().Width() + 1)/(float)miniature_width;
	float dy = 	(the_bitmap->Bounds().Height() + 1)/(float)miniature_height;
	int32 x=0,y=0;

	int32 x_offset_left = (int32)floor((float)(HS_MINIATURE_IMAGE_WIDTH-miniature_width)/2.0);
	int32 x_offset_right = (int32)ceil((float)(HS_MINIATURE_IMAGE_WIDTH-miniature_width)/2.0);

	int32 y_offset = (HS_MINIATURE_IMAGE_HEIGHT-miniature_height)/2;

	// The bitmap might be changed and deleted while we are accessing it.
	int32	b_bpr = the_bitmap->BytesPerRow()/4;
	uint32 *big_image;
	uint32 *small_image = (uint32*)miniature_image->Bits();
	big_image = (uint32*)the_bitmap->Bits();
	// Clear the parts that we do not set.
	for (int32 i=0;i<HS_MINIATURE_IMAGE_WIDTH*y_offset;i++)
		*small_image++ = white.word;

	while ((y < miniature_height) && (mini_image_threads_waiting == 0)) {
		for (int32 i=0;i<x_offset_left;i++)
			*small_image++ = white.word;

		while ((x < miniature_width) && (mini_image_threads_waiting == 0)) {
			color.word = *(big_image + ((int32)(y*dy))*b_bpr + (int32)(x*dx));
			color.bytes[0] = (uint8)(color.bytes[0] * float_alpha_table[color.bytes[3]] + 255 * (1.0 - float_alpha_table[color.bytes[3]]));
			color.bytes[1] = (uint8)(color.bytes[1] * float_alpha_table[color.bytes[3]] + 255 * (1.0 - float_alpha_table[color.bytes[3]]));
			color.bytes[2] = (uint8)(color.bytes[2] * float_alpha_table[color.bytes[3]] + 255 * (1.0 - float_alpha_table[color.bytes[3]]));
			*small_image++ = color.word;
			x++;
		}
		y++;

		for (int32 i=0;i<x_offset_right;i++)
			*small_image++ = white.word;

		x = 0;
	}

	// Clear the rest of the image
	while (small_image != ((uint32*)miniature_image->Bits() + miniature_image->BitsLength()/4))
		*small_image++ = white.word;

	if (mini_image_threads_waiting == 0) {
		snooze(50 * 1000);
		if (mini_image_threads_waiting == 0) {
			if (layer_view->LockLooper()) {
				layer_view->UpdateImage();
				BView *bmap_view;
				if ((bmap_view = layer_view->Window()->FindView("bitmap_view")) != NULL) {
					bmap_view->Draw(bmap_view->Bounds());
				}
				layer_view->UnlockLooper();
			}
		}
	}
	release_sem(mini_image_semaphore);

	return B_NO_ERROR;
}

void Layer::ChangeBitmap(BBitmap *new_bitmap)
{
	BBitmap *old_bitmap;
	old_bitmap = the_bitmap;

	the_bitmap = new_bitmap;
	delete old_bitmap;
}


void Layer::ActivateLayer(bool active)
{
	is_active = active;
	layer_view->Activate(active);
}


void Layer::SetVisibility(bool visible)
{
	visibility = visible;
	layer_view->SetVisibility(visible);
}



void Layer::Merge(Layer *top_layer)
{
	int32 top_bpr = top_layer->Bitmap()->BytesPerRow()/4;
	int32 bottom_bpr = Bitmap()->BytesPerRow()/4;
	uint32 *top_bits = (uint32*)top_layer->Bitmap()->Bits();
	uint32 *bottom_bits = (uint32*)Bitmap()->Bits();

	int32 height = (int32)min_c(top_layer->Bitmap()->Bounds().Height()+1,Bitmap()->Bounds().Height()+1);
	int32 width = (int32)min_c(top_layer->Bitmap()->Bounds().Width()+1,Bitmap()->Bounds().Width()+1);

	union {
		uint8 bytes[4];
		uint32 word;
	} top,bottom,target;

	float alpha;
	float beta;

	float top_coefficient;
	float bottom_coefficient;
	float new_alpha;

	float *alphas = float_alpha_table;
	float *betas = top_layer->float_alpha_table;

	for (register int32 y=0;y<height;++y) {
		for (register int32 x=0;x<width;++x) {
			top.word = *(top_bits + x + y*top_bpr);
			bottom.word = *(bottom_bits + x + y*bottom_bpr);
			alpha = alphas[bottom.bytes[3]];
			beta = betas[top.bytes[3]];

			new_alpha = (alpha + beta - alpha*beta);
			if (new_alpha > 0) {
				bottom_coefficient = (alpha - alpha*beta)/new_alpha;
				top_coefficient = beta/new_alpha;
			}
			else {
				// If both layers are fully transparent, the new color will
				// be fully transparent average of the two colors.
				bottom_coefficient = 0.5;
				top_coefficient = 0.5;
			}

			target.bytes[0] = (uint8)max_c(0,min_c(255,bottom_coefficient*bottom.bytes[0]+top_coefficient*top.bytes[0]));
			target.bytes[1] = (uint8)max_c(0,min_c(255,bottom_coefficient*bottom.bytes[1]+top_coefficient*top.bytes[1]));
			target.bytes[2] = (uint8)max_c(0,min_c(255,bottom_coefficient*bottom.bytes[2]+top_coefficient*top.bytes[2]));
			target.bytes[3] = (uint8)(255*new_alpha);
			*(bottom_bits + x + y*bottom_bpr) = target.word;
		}
	}

	// Change the transparency to 1.0
	SetTransparency(1.0);
}


Layer* Layer::readLayer(BFile &file,ImageView *image_v,int32 new_id,bool is_little_endian,int32 compression_method)
{
	// This is the new way of reading the layers.
	int32 marker;
	if (file.Read(&marker,sizeof(int32)) != sizeof(int32))
		return NULL;

	if (is_little_endian)
		marker = B_LENDIAN_TO_HOST_INT32(marker);
	else
		marker = B_BENDIAN_TO_HOST_INT32(marker);

	if (marker != PROJECT_FILE_LAYER_START_MARKER)
		return NULL;

	int32 width;
	int32 height;
	int32 layer_type;
	int32 layer_visibility;
	int64 length;
	if (file.Read(&width,sizeof(int32)) != sizeof(int32))
		return NULL;
	if (file.Read(&height,sizeof(int32)) != sizeof(int32))
		return NULL;
	if (file.Read(&layer_type,sizeof(int32)) != sizeof(int32))
		return NULL;
	if (file.Read(&layer_visibility,sizeof(int32)) != sizeof(int32))
		return NULL;
	if (file.Read(&length,sizeof(int64)) != sizeof(int64))
		return NULL;

	if (is_little_endian) {
		width = B_LENDIAN_TO_HOST_INT32(width);
		height = B_LENDIAN_TO_HOST_INT32(height);
		layer_type = B_LENDIAN_TO_HOST_INT32(layer_type);
		length = B_LENDIAN_TO_HOST_INT64(length);
	}
	else {
		width = B_BENDIAN_TO_HOST_INT32(width);
		height = B_BENDIAN_TO_HOST_INT32(height);
		layer_type = B_BENDIAN_TO_HOST_INT32(layer_type);
		length = B_BENDIAN_TO_HOST_INT64(length);
	}

	Layer *layer = new Layer(BRect(0,0,width-1,height-1),new_id,image_v,layer_type);
	layer->SetVisibility((uint32(layer_visibility) == 0xFFFFFFFF));
	int8 *bits = (int8*)layer->Bitmap()->Bits();
	if (file.Read(bits,length) != length) {
		delete layer;
		return NULL;
	}

	// Read the end-marker.
	if (file.Read(&marker,sizeof(int32)) != sizeof(int32)) {
		delete layer;
		return NULL;
	}
	if (is_little_endian)
		marker = B_LENDIAN_TO_HOST_INT32(marker);
	else
		marker = B_BENDIAN_TO_HOST_INT32(marker);

	if (marker != PROJECT_FILE_LAYER_END_MARKER) {
		delete layer;
		return NULL;
	}

	// Here try to read the extra-data block.
	if (file.Read(&marker,sizeof(int32)) == sizeof(int32)) {
		if (is_little_endian)
			marker = B_LENDIAN_TO_HOST_INT32(marker);
		else
			marker = B_BENDIAN_TO_HOST_INT32(marker);

		if (marker == PROJECT_FILE_LAYER_EXTRA_DATA_START_MARKER) {
			// Read the length of this section
			int32 length;
			if (file.Read(&length,sizeof(int32)) != sizeof(int32)) {
				delete layer;
				return NULL;
			}

			if (is_little_endian)
				length = B_LENDIAN_TO_HOST_INT32(length);
			else
				length = B_BENDIAN_TO_HOST_INT32(length);

			// Read the transparency coefficient
			float coeff;
			if (file.Read(&coeff,sizeof(float)) != sizeof(float)) {
				delete layer;
				return NULL;
			}
			if (is_little_endian)
				coeff = B_LENDIAN_TO_HOST_FLOAT(coeff);
			else
				coeff = B_BENDIAN_TO_HOST_FLOAT(coeff);

			layer->SetTransparency(coeff);
			length -= sizeof(float);

			// Skip the extra data that we do not recognize.
			file.Seek(length,SEEK_CUR);

			// Here we should get the end-marker for layer's extra data
			if (file.Read(&marker,sizeof(int32)) != sizeof(int32)) {
				delete layer;
				return NULL;
			}
			if (is_little_endian)
				marker = B_LENDIAN_TO_HOST_INT32(marker);
			else
				marker = B_BENDIAN_TO_HOST_INT32(marker);

			if (marker != PROJECT_FILE_LAYER_EXTRA_DATA_END_MARKER) {
				delete layer;
				return NULL;
			}

		}
		else {
			// Somehow -sizeof(int32) does not seem to work????
			file.Seek(-4,SEEK_CUR);
		}
	}

	// Before returning calculate the layer's miniature image.
	layer->calc_mini_image();

	return layer;
}


Layer* Layer::readLayerOldStyle(BFile &file,ImageView *image_v,int32 new_id)
{
//	// Layer has stored the following things:
//	//	1.	Layer frame (i.e. the frame of bitmap)
//	//	2.	Layer id
//	//	3.	Layer type
//	//	4.	Layer visibility
//	//	5.	Bitmap data
//	//
	BRect layer_frame;
	uint32 type;
	int32 id;	// This is not actually used.
	bool visi;

	if (file.Read(&layer_frame,sizeof(BRect)) != sizeof(BRect)) {
		return NULL;
	}

	// The layer id is written to the file, so it must be read also, but it will not
	// be used. Instead we use the id that is provided as a parameter
	if (file.Read(&id,sizeof(uint32)) != sizeof(uint32)) {
		return NULL;
	}
	else {
		id = B_BENDIAN_TO_HOST_INT32(id);
	}
	if ((file.Read(&type,sizeof(uint32)) != sizeof(uint32))) {
		return NULL;
	}
	if (file.Read(&visi,sizeof(bool)) != sizeof(bool)) {
		return NULL;
	}

	// Old files project-files are all big-endian so we convert data here.
	layer_frame.left = B_BENDIAN_TO_HOST_FLOAT(layer_frame.left);
	layer_frame.right = B_BENDIAN_TO_HOST_FLOAT(layer_frame.right);
	layer_frame.top = B_BENDIAN_TO_HOST_FLOAT(layer_frame.top);
	layer_frame.bottom = B_BENDIAN_TO_HOST_FLOAT(layer_frame.bottom);

	type = B_BENDIAN_TO_HOST_INT32(type);
	if (type != HS_NORMAL_LAYER) {
		return NULL;
	}

	// Create the layer
	Layer *layer = new Layer(layer_frame,new_id,image_v,type);
	layer->SetVisibility(visi);

	int8 *bits = (int8*)layer->Bitmap()->Bits();
	// align the file pointer to four-byte boundary.
	int32 alignment_offset;
	alignment_offset = (4 - (file.Position() % 4)) % 4;

	if (file.Read(bits,alignment_offset) != alignment_offset) {
		delete layer;
		return NULL;
	}
	bits += alignment_offset;


	if (file.Read(bits,layer->Bitmap()->BitsLength()-alignment_offset) != (layer->Bitmap()->BitsLength()-alignment_offset)) {
		delete layer;
		return NULL;
	}
	// Before returning calculate the layer's miniature image.
	layer->calc_mini_image();
	return layer;
}

int64 Layer::writeLayer(BFile &file,int32 compression_method)
{
	int64 written_bytes = 0;
	int32 marker = PROJECT_FILE_LAYER_START_MARKER;
	int32 visi;
	if (visibility == TRUE)
		visi = 0xFFFFFFFF;
	else
		visi = 0x00000000;

	written_bytes += file.Write(&marker,sizeof(int32));
	int32 width = the_bitmap->Bounds().IntegerWidth()+1;
	int32 height = the_bitmap->Bounds().IntegerHeight()+1;

	written_bytes += file.Write(&width,sizeof(int32));
	written_bytes += file.Write(&height,sizeof(int32));
	written_bytes += file.Write(&layer_type,sizeof(int32));
	written_bytes += file.Write(&visi,sizeof(int32));

	int64 data_length = the_bitmap->BitsLength();
	written_bytes += file.Write(&data_length,sizeof(int64));

	written_bytes += file.Write(the_bitmap->Bits(),data_length);

	marker = PROJECT_FILE_LAYER_END_MARKER;
	written_bytes += file.Write(&marker,sizeof(int32));

	// Here we write the layer extra-data to the file.
	marker = PROJECT_FILE_LAYER_EXTRA_DATA_START_MARKER;
	written_bytes += file.Write(&marker,sizeof(int32));

	marker = sizeof(float);
	written_bytes += file.Write(&marker,sizeof(int32));
	written_bytes += file.Write(&transparency_coefficient,sizeof(float));

	marker = PROJECT_FILE_LAYER_EXTRA_DATA_END_MARKER;
	written_bytes += file.Write(&marker,sizeof(int32));

	return written_bytes;
}


Layer* Layer::ReturnUpperLayer()
{
	if (the_image != NULL)
		return the_image->ReturnUpperLayer(this);
	else
		return NULL;
}


Layer* Layer::ReturnLowerLayer()
{
	if (the_image != NULL)
		return the_image->ReturnLowerLayer(this);
	else
		return NULL;
}



const char* Layer::ReturnProjectName()
{
	return ((ImageView*)image_view)->ReturnProjectName();
}
