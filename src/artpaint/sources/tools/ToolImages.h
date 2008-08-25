/* 

	Filename:	ToolImages.h
	Contents:	Class declaration for tool-images and constants
	Author:		Heikki Suhonen
	
*/

#ifndef TOOL_IMAGES_H
#define TOOL_IMAGES_H

#include <Picture.h>
#include <SupportDefs.h>

// these define the sizes of various picture-buttons
#define	BIG_TOOL_PICTURE_SIZE		32
#define	SMALL_TOOL_PICTURE_SIZE		16 

class ToolImages {
	BPicture *off_big,*on_big,*off_small,*on_small;
	int32 tool_type;
	ToolImages *next_tool;

// this points to the first tool in the list	
static	ToolImages 	*first_tool;		

static	status_t	ReadImages(BResources*,int32,int32);

public:
		ToolImages(int32 type, BPicture *picture_off_big,BPicture *picture_on_big,BPicture *picture_off_small,BPicture *picture_on_small);

static	BPicture*	getPicture(int32 type,int32 picture_size,int32 picture_number);

// this function will create all the tool-images
static	void	createToolImages();
};



BPicture* bitmap_to_picture(BBitmap*);

#endif