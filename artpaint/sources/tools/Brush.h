/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */


/*
	Brush-class represents a brush that can be drawn to image by copying it's
	bits or by using it's bits as a weighting function when drawing with a solid
	color. The latter is the main use for brushes. There are three kinds
	of brushes:

		1.	Binary brushes
		2.	Alpha brushes
		3.	32-bit RGBA brushes

	The class offers facilities for creating and editing the brushes.
*/

#ifndef BRUSH_H
#define BRUSH_H

#include <SupportDefs.h>


#define	BRUSH_PREVIEW_WIDTH		64
#define	BRUSH_PREVIEW_HEIGHT	64

// This constant is used in messages where brushes are dragged.
// The brush dragging message contains data of sizeof(brush_info)
// that is named "brush data", it is B_ANY_TYPE type.
#define	HS_BRUSH_DRAGGED		'Brdr'

enum brush_shapes {
	HS_RECTANGULAR_BRUSH,
	HS_ELLIPTICAL_BRUSH,
	HS_IRREGULAR_BRUSH
};

//enum brush_types {
//	HS_BINARY_BRUSH,
//	HS_ALPHA_BRUSH,
//	HS_RGBA_BRUSH
//};

//struct brush_edge {
//	float	fade_length;	// This determines how many pixels the fade takes
//	float	fade_type;		// This determines the type of fade (exponential,linear or somewhere in between).
//};


struct brush_info {
	int32	shape;
	float	width;
	float 	height;
	float	angle;
	float	fade_length;
};

// This next struct is used to form a linked list for brush's
// spans. One element contains the row-number and the beginning
// and ending clumns of the span.
struct span {
	int32	row;
	int32	span_start;
	int32	span_end;
	span	*next;

	span(int32 r,int32 x1, int32 x2) { row = r; span_start = x1; span_end = x2; next = NULL; }
};

class Brush {
	int32			shape_;
	float 			width_;
	float			height_;
	float			angle_;
	float			fade_length_;

	float			maximum_width;
	float			maximum_height;

	float 			actual_width;
	float 			actual_height;

	void			make_rectangular_brush();
	void			make_elliptical_brush();

	uint32**		reserve_brush();
	uint32**		make_diff_brush(uint32**,int32,int32);
	span*			make_span_list(uint32**);

	void			delete_all_data();

	void			print_brush(uint32**);


	uint32 			**brush;
	uint32			**diff10;
	uint32			**diff11;
	uint32			**diff1_1;
	uint32			**diff01;
	uint32			**diff0_1;
	uint32			**diff_10;
	uint32			**diff_11;
	uint32			**diff_1_1;

	span			*brush_span;
	span			*diff10_span;
	span			*diff11_span;
	span			*diff1_1_span;
	span			*diff01_span;
	span			*diff0_1_span;
	span			*diff_10_span;
	span			*diff_11_span;
	span			*diff_1_1_span;


public:
		Brush(brush_info &info,bool create_diff_brushes=TRUE);
		~Brush();

void	ModifyBrush(brush_info &info);
void	CreateDiffBrushes();

float	PreviewBrush(BBitmap*);


uint32**	GetData(span**,int32,int32);
float		Width() { return width_; }
float		Height() { return height_; }
brush_info	GetInfo();
};

#endif
