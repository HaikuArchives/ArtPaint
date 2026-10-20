/*
 * Copyright 2026, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#ifndef MULTI_SLIDER_H
#define MULTI_SLIDER_H

#include <Bitmap.h>
#include <ObjectList.h>
#include <Slider.h>


class MultiSlider: public BSlider
{
public:
						MultiSlider(BRect frame, const char* name,
							const char* label, BMessage* message,
							int32 minValue, int32 maxValue,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 resizingMode = B_FOLLOW_LEFT_TOP,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

						MultiSlider(BRect frame, const char* name,
							const char* label, BMessage* message,
							int32 minValue, int32 maxValue,
							orientation posture,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 resizingMode = B_FOLLOW_LEFT_TOP,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

						MultiSlider(const char* name, const char* label,
							BMessage* message, int32 minValue,
							int32 maxValue, orientation posture,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW | B_FRAME_EVENTS);

						MultiSlider(BMessage* archive);
						~MultiSlider();

	virtual	void		AttachedToWindow();
			void		Init();

	virtual void		DrawBar();
	virtual	BRect		ThumbFrame(int32 thumb) const;
	virtual float		Position(int32 thumb) const;
	virtual	void		SetPosition(int32 thumb, float position);
	virtual	void		SetValue(int32 thumb, int32 value);
	virtual	int32		Value(int32 thumb);

	virtual void		DrawSlider();
	virtual	void		DrawThumbs();
	virtual void		DrawThumb(int32 thumb);
	virtual void		MouseDown(BPoint point);
	virtual void 		MouseUp(BPoint point);
	virtual void		MouseMoved(BPoint point, uint32 transit, const BMessage* message);
	virtual void		KeyDown(const char* bytes, int32 numBytes);
	virtual void		KeyUp(const char* bytes, int32 numBytes);

	virtual void		AddThumb(float newPosition);
	virtual void		RemoveThumb(int32 thumb);
			BObjectList<float> Positions() const;
private:
			BObjectList<float> fPositions;
			int32		fSelectedThumb;
			BObjectList<BPoint> fLocations;
			BPoint 		fInitialLocation;

			void		_DrawBlockThumb(int32 thumb);
			void		_DrawTriangleThumb(int32 thumb);
			float		_MinPosition() const;
			float		_MaxPosition() const;
			void		_SetLocationForValue(int32 thumb, int32 value);
			BPoint		_Location(int32 thumb);
			bool		_ConstrainPoint(BPoint& point, BPoint compare,
							float minPos = -1, float maxPos = -1);
};


#endif  // COLOR_MULTI_SLIDER_H
