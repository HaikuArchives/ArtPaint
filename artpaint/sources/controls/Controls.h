/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef CONTROLS_H
#define CONTROLS_H


#include <Slider.h>


class ControlSlider : public BSlider {
public:
						ControlSlider(const char* name, const char* label,
							BMessage* message, int32 minValue, int32 maxValue,
							thumb_style thumbType);

						ControlSlider(BRect frame, const char* name,
							const char* label, BMessage* message, int32 rangeMin,
							int32 rangeMax, thumb_style knob);

	virtual	void		MouseDown(BPoint where);

private:
	static	int32		track_entry(void*);
			int32		track_mouse();
};

#endif
