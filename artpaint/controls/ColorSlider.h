/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#ifndef COLOR_SLIDER_H
#define COLOR_SLIDER_H


#include <Bitmap.h>
#include <Slider.h>


class ColorSlider: public BSlider
{
public:
								ColorSlider(BRect frame, const char* name,
									const char* label, BMessage* message,
									int32 minValue, int32 maxValue,
									thumb_style thumbType = B_BLOCK_THUMB,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_NAVIGABLE | B_WILL_DRAW
										| B_FRAME_EVENTS);

								ColorSlider(BRect frame, const char* name,
									const char* label, BMessage* message,
									int32 minValue, int32 maxValue,
									orientation posture,
									thumb_style thumbType = B_BLOCK_THUMB,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_NAVIGABLE | B_WILL_DRAW
										| B_FRAME_EVENTS);

								ColorSlider(const char* name, const char* label,
									BMessage* message, int32 minValue,
									int32 maxValue, orientation posture,
									thumb_style thumbType = B_BLOCK_THUMB,
									uint32 flags = B_NAVIGABLE | B_WILL_DRAW
										| B_FRAME_EVENTS);

								ColorSlider(BMessage* archive);
 								~ColorSlider();
			void				SetColors(rgb_color start, rgb_color end);
			void				SetColors(BList* colors);

	virtual void				DrawBar();

private:
			rgb_color			fColorStart;
			rgb_color			fColorEnd;
			BBitmap*			gradient;
};

#endif
