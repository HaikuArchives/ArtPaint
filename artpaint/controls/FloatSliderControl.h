/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef FLOAT_CONTROL_SLIDER_BOX_H
#define FLOAT_CONTROL_SLIDER_BOX_H

#include <Box.h>
#include <Messenger.h>
#include <Slider.h>


class BLayoutItem;
class BSlider;


namespace ArtPaint {
	namespace Interface {

class FloatControl;

class FloatSliderControl : public BBox {
public:
								FloatSliderControl(const char* label,
									const char* text, BMessage* message,
									float minRange, float maxRange,
									bool layout, bool continuous = true,
									border_style borderStyle = B_NO_BORDER,
									thumb_style thumbStyle = B_TRIANGLE_THUMB,
									uint8 resolution = 1);
	virtual						~FloatSliderControl();

	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				SetEnabled(bool enabled);

			void				SetValue(float value);
			float				Value() const;
			void				SetTarget(const BMessenger& target);

			void				SetMessage(BMessage* message);

			void				SetMinMax(float min, float max);
			void				SetResolution(uint8 resolution);

			BString				Label() const;

			BSlider*			Slider() const;
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

			BSlider*			fSlider;
			BMessage*			fMessage;
			FloatControl*		fFloatControl;
			float				fMult;
};

	}	// namespace Interface
}	// namespace ArtPaint


#endif	//	CONTROL_SLIDER_BOX_H
