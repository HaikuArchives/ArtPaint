/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef COLOR_FLOAT_SLIDER_H
#define COLOR_FLOAT_SLIDER_H

#include "ColorSlider.h"

#include <Box.h>
#include <Messenger.h>
#include <Slider.h>


class BLayoutItem;
class BSlider;


namespace ArtPaint {
	namespace Interface {

class FloatControl;

class ColorFloatSlider : public BBox {
public:
								ColorFloatSlider(const char* label,
									const char* text, BMessage* message,
									float minRange, float maxRange,
									bool layout, bool continuous = true,
									border_style borderStyle = B_NO_BORDER,
									thumb_style thumbStyle = B_TRIANGLE_THUMB,
									uint8 resolution = 1);
	virtual						~ColorFloatSlider();

	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				SetEnabled(bool enabled);

			void				SetValue(float value);
			float				Value() const;
			void				SetTarget(const BMessenger& target);

			void				SetMessage(BMessage* message);

			void				SetMinMax(float min, float max);
			void				SetResolution(uint8 resolution);
			void				SetToolTip(const char* tip);

			ColorSlider*		Slider() const;
			FloatControl*		TextControl() const;

			BLayoutItem*		LabelLayoutItem() const;
			BLayoutItem*		TextViewLayoutItem() const;

private:
			void				_InitMessage();
			float				_FixValue(float value);
			void				_SendMessage(float value, bool final = true);

private:
			BMessenger			fTarget;

			float				fMinRange;
			float				fMaxRange;
			bool				fContinuous;

			ColorSlider*			fSlider;
			BMessage*			fMessage;
			FloatControl*		fFloatControl;
			float				fMult;
};

	}	// namespace Interface
}	// namespace ArtPaint

#endif	//	COLOR_FLOAT_SLIDER_H
