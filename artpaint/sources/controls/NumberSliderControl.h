/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef _CONTROL_SLIDER_BOX_H_
#define _CONTROL_SLIDER_BOX_H_

#include <Box.h>
#include <Messenger.h>
#include <Slider.h>


class BLayoutItem;
class BSlider;
class NumberControl;


namespace ArtPaint {
	namespace Interface {

class NumberSliderControl : public BBox {
public:
								NumberSliderControl(const char* label,
									const char* text, BMessage* message,
									int32 minRange, int32 maxRange,
									bool layout, bool continuos = true,
									border_style borderStyle = B_NO_BORDER,
									thumb_style thumbStyle = B_TRIANGLE_THUMB);
	virtual						~NumberSliderControl();

	virtual	void				AllAttached();
	virtual	void				MessageReceived(BMessage* message);

			void				SetValue(int32 value);
			void				SetTarget(const BMessenger& target);

			void				SetMessage(BMessage* message);

			BSlider*			Slider() const;
			NumberControl*		TextControl() const;

			BLayoutItem*		LabelLayoutItem() const;
			BLayoutItem*		TextViewLayoutItem() const;

private:
			void				_InitMessage();
			int32				_FixValue(int32 value);
			void				_SendMessage(int32 value, bool final = true);

private:
			BMessenger			fTarget;

			int32				fMinRange;
			int32				fMaxRange;
			bool				fContinuos;

			BSlider*			fSlider;
			BMessage*			fMessage;
			NumberControl*		fNumberControl;
};

	}	// namespace Interface
}	// namespace ArtPaint

#endif	//	_CONTROL_SLIDER_BOX_H_
