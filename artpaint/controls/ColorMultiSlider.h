/*
 * Copyright 2026, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef COLOR_MULTI_SLIDER_H
#define COLOR_MULTI_SLIDER_H

#include <Bitmap.h>
#include <ObjectList.h>

#include "MultiSlider.h"


class ColorMultiSlider: public MultiSlider
{
public:
						ColorMultiSlider(BRect frame, const char* name,
							const char* label, BMessage* message,
							int32 minValue, int32 maxValue,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 resizingMode = B_FOLLOW_LEFT_TOP,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

						ColorMultiSlider(BRect frame, const char* name,
							const char* label, BMessage* message,
							int32 minValue, int32 maxValue,
							orientation posture,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 resizingMode = B_FOLLOW_LEFT_TOP,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

						ColorMultiSlider(const char* name, const char* label,
							BMessage* message, int32 minValue,
							int32 maxValue, orientation posture,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

						ColorMultiSlider(BMessage* archive);
						~ColorMultiSlider();

	virtual	void		Init();
			void		SetColors(rgb_color start, rgb_color end);
			void		SetColors(BList* colors);
			void		SetColorsByPosition(BList* colors);

	virtual void		DrawBar();

private:
			rgb_color	fColorStart;
			rgb_color	fColorEnd;
			BBitmap*	gradient;
};


#endif  // COLOR_MULTI_SLIDER_H
