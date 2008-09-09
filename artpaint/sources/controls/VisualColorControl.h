/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef VISUAL_COLOR_CONTROL_H
#define VISUAL_COLOR_CONTROL_H

#include <Control.h>
class NumberControl;

// this is height that is reserved per color
#define	COLOR_HEIGHT 20

// these define the width of the controls
#define	PLATE_WIDTH	40
#define	RAMP_WIDTH	255

class VisualColorControl : public BControl {

	BPicture		*up_arrow;
	BPicture		*down_arrow;

	NumberControl	*control1;
	NumberControl	*control2;
	NumberControl	*control3;
	NumberControl	*control4;

	char			*label1;
	char			*label2;
	char			*label3;
	char			*label4;


	int32			previous_value_at_1;
	int32			previous_value_at_2;
	int32			previous_value_at_3;
	int32			previous_value_at_4;

// This method will be implemented in subclasses.
virtual		void	CalcRamps() = 0;

protected:
			BBitmap	*ramp1;		// Upper color-ramp
			BBitmap	*ramp2;		// Middle color-ramp
			BBitmap	*ramp3;		// Lower color-ramp
			BBitmap	*ramp4;		// This is usually the alpha-ramp

			// This holds the color-value.
			union {
				uint8 	bytes[4];
				uint32	word;
			} value;

			int32	ramp_left_edge;	// The left edge where the color-ramps start.

// These methods will be implemented in the subclasses. They return
// what value each component has. The numbers refer to the ramp numbers
// from top to bottom.
virtual		int32	value_at_1() = 0;
virtual		int32	value_at_2() = 0;
virtual		int32	value_at_3() = 0;
virtual		int32	value_at_4() { return value.bytes[3]; }

// These eight methods will be overriden in subclasses if needed.
// They are used when drawing the arrows over the ramps.
virtual		float	max_value_at_1() { return 255; }
virtual		float	max_value_at_2() { return 255; }
virtual		float	max_value_at_3() { return 255; }
virtual		float	max_value_at_4() { return 255; }

virtual		float	min_value_at_1() { return 0; }
virtual		float	min_value_at_2() { return 0; }
virtual		float	min_value_at_3() { return 0; }
virtual		float	min_value_at_4() { return 0; }

public:
		VisualColorControl(BPoint position, rgb_color c,char*,char*,char*,char*);
virtual	~VisualColorControl();

		void		AttachedToWindow();
		void		Draw(BRect);
		void		MessageReceived(BMessage*);
virtual	void		SetValue(int32 val);
virtual	void		SetValue(rgb_color c);
		rgb_color	ValueAsColor();
};
#endif
