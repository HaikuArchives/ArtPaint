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

	virtual void				WindowActivated(bool state);
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual void				FrameMoved(BPoint newPosition);
	virtual void				FrameResized(float width, float height);
	virtual void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				KeyUp(const char* bytes, int32 numBytes);
	virtual void				MouseDown(BPoint point);
	virtual void				MouseUp(BPoint point);
	virtual void				MouseMoved(BPoint point, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				Pulse();

	virtual void				SetLabel(const char* label);
	virtual	void				SetLimitLabels(const char* minLabel,
									const char* maxLabel);
	virtual	void				SetValue(int32 value);
	virtual void				SetPosition(float);
	virtual void				SetEnabled(bool on);

	virtual	void				Draw(BRect updateRect);
	virtual void				DrawSlider();
	virtual void				DrawBar();
	virtual void				DrawHashMarks();
	virtual void				DrawThumb();
	virtual void				DrawFocusMark();
	virtual	void				DrawText();

	virtual	void				SetFlags(uint32 flags);
	virtual	void				SetResizingMode(uint32 mode);

	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				ResizeToPreferred();

	virtual	void				SetModificationMessage(BMessage* message);

	virtual void				SetSnoozeAmount(int32 microSeconds);

	virtual	void				SetKeyIncrementValue(int32 value);

	virtual	void				SetHashMarkCount(int32 count);

	virtual	void				SetHashMarks(hash_mark_location where);

	virtual	void				SetStyle(thumb_style style);

	virtual	void				SetBarColor(rgb_color color);
	virtual	void				UseFillColor(bool useFill,
									const rgb_color* color = NULL);
	virtual void				SetOrientation(orientation);

	virtual void				SetBarThickness(float thickness);

	virtual void				SetFont(const BFont* font,
									uint32 properties = B_FONT_ALL);

	virtual void				SetLimits(int32 minimum, int32 maximum);

protected:
	virtual	void				LayoutInvalidated(bool descendants);

private:
			rgb_color			fColorStart;
			rgb_color			fColorEnd;
			BBitmap*			gradient;
};

#endif
