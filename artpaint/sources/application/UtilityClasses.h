/*

	Filename:	UtilityClasses.h
	Contents:	Declarations for small classes that do not need their own file
	Author:		Heikki Suhonen

*/

#ifndef UTILITY_CLASSES_H
#define UTILITY_CLASSES_H

#include <Box.h>
#include <FilePanel.h>
#include <View.h>
#include <Window.h>


// this class creates an object that can display help-text in a B_BORDERED_WINDOW
class HelpWindow : public BWindow {

public:
		HelpWindow(BPoint location, const char *text);
		HelpWindow(BPoint location, char **text_lines, int32 line_count);
};



// this class creates a view that draws a bitmap in it

class BitmapView : public BView {
		BBitmap		*the_bitmap;

public:
		BitmapView(BBitmap *bitmap, BPoint location);
		BitmapView(BBitmap *bitmap, BRect frame);
void	Draw(BRect updateRect);

inline	void	ChangeBitmap(BBitmap *bitmap) { the_bitmap = bitmap; }
};


class BitmapViewBox : public BBox {
	BitmapView	*bmap_view;

public:
		BitmapViewBox(BBitmap *bitmap,BRect frame,char *label);

void	UpdateBitmap();
};

class PointStack {
	BPoint	*stack;
	BPoint	*top;
	int 	size;
public:
		PointStack(int sz)	{ top = stack = new BPoint[size = sz]; }
		~PointStack()		{ delete[] stack; }

BPoint 	Pop()				{ return (top != stack ? *--top : *stack); }
void	Push(BPoint p)		{ *top++ = p; }
bool	IsEmpty()			{ return top <= stack; }
};

// This function just returns a copy of the parameter-bitmap. If the parameter
// is true, the returned bitmap also contains any possible parameter views.
BBitmap*	CopyBitmap(BBitmap*,bool=FALSE);

BRect		FitRectToScreen(BRect);



inline uint32 RGBColorToBGRA(rgb_color c)
{
	union {
		char bytes[4];
		uint32 word;
	} color;

	color.bytes[0] = c.blue;
	color.bytes[1] = c.green;
	color.bytes[2] = c.red;
	color.bytes[3] = c.alpha;
//	return ((c.blue << 24) & 0xFF000000) | ((c.green << 16) & 0x00FF0000) | ((c.red << 8) & 0x0000FF00) | (c.alpha & 0xFF);
	return color.word;
}



#if __POWERPC__
inline rgb_color BGRAColorToRGB(uint32 bgra_color)
{
	rgb_color c;
	c.red = (bgra_color >> 8) & 0xFF;
	c.green = (bgra_color >> 16) & 0xFF;
	c.blue = (bgra_color >> 24) & 0xFF;
	c.alpha = (bgra_color) & 0xFF;

	return c;
}
#else
inline rgb_color BGRAColorToRGB(uint32 bgra_color)
{
	rgb_color c;
	c.red = (bgra_color >> 16) & 0xFF;
	c.green = (bgra_color >> 8) & 0xFF;
	c.blue = (bgra_color >> 0) & 0xFF;
	c.alpha = (bgra_color >> 24) & 0xFF;

	return c;
}
#endif


// This function makes a rect out of two points after sorting the points
BRect make_rect_from_points(BPoint&,BPoint&);

// The next function translates the strings in filepanels (like 'Open' etc.)
status_t set_filepanel_strings(BFilePanel*);

// This function reads a bitmap from apps resources.
//BBitmap* read_bitmap_from_resources(int32,int32,color_space,int32);

#endif
