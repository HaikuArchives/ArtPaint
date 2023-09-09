/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2008, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <karsten.heimrich@gmx.de>
 *
 */
#ifndef POP_UP_SLIDER_H
#define	POP_UP_SLIDER_H

#include <Window.h>


class BMessage;
class BMessenger;
class BSlider;


class PopUpSlider : public BWindow {
public:
	static	PopUpSlider*	Instantiate(const BMessenger& target,
								BMessage* message, int32 minRange, int32 maxRange);

			void			Go();
			BSlider*		Slider() const;

private:
							PopUpSlider(BRect rect);

private:
			BSlider*		fSlider;
};


#endif // POP_UP_SLIDER_H
