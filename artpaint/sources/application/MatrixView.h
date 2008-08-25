/* 

	Filename:	MatrixView.h
	Contents:	MatrixView-class declaration		
	Author:		Heikki Suhonen
	
*/



#ifndef MATRIX_VIEW_H
#define	MATRIX_VIEW_H

#include <View.h>

/*
	MatrixView is a class that holds subviews of equal size
	and orders them in a matrix. When the view is resized, the 
	subviews are automatically reordered.
*/


class MatrixView : public BView {
		const	int32 	cell_height;
		const	int32 	cell_width;
		const	int32	cell_spacing;
public:
			MatrixView(int32 cell_h,int32 cell_w,int32 spacing);

void		AttachedToWindow();
void		FrameResized(float,float);
void		GetPreferredSize(float*,float*);

status_t	AddSubView(BView*);
};


#endif