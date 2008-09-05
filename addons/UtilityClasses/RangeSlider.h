/*

	Filename:	RangeSlider.h
	Contents:	Declaration for a slider that can be used to adjust a range.
	Author:		Heikki Suhonen

*/



#ifndef RANGE_SLIDER_H
#define	RANGE_SLIDER_H

#include <Slider.h>


class RangeSlider : public BSlider {
int32	Value() const { return 0; }	// so that this cannot be accessed from outside
void	SetValue(int32) {} // does nothing
static	int32	track_entry(void*);
		int32	track_mouse();

		int32	lower_value;
		int32	higher_value;

		int32 	min_value;
		int32	max_value;

		rgb_color	lower_thumb_color;
		rgb_color	higher_thumb_color;

		int32	HigherValueX();
		int32	LowerValueX();


public:
		RangeSlider(BRect frame, const char *name, const char *label, BMessage *message, int32 minValue, int32 maxValue);

void	DrawBar();
void	DrawThumb();
void	MouseDown(BPoint);

int32	HigherValue() { return higher_value; }
int32	LowerValue() { return lower_value; }

void	SetHigherValue(int32);
void	SetLowerValue(int32);

void	SetHigherThumbColor(rgb_color);
void	SetLowerThumbColor(rgb_color);
};

#endif
