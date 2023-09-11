/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "Manipulator.h"

#ifndef ROTATE_90_MANIPULATOR_H
#define	ROTATE_90_MANIPULATOR_H


class Rotate90ClockwiseManipulator : public Manipulator {
public:
						Rotate90ClockwiseManipulator();
		BBitmap*		ManipulateBitmap(BBitmap* original, BStatusBar*);

		const char*		ReturnName();
		void			SetSelection(Selection* new_selection) { selection = new_selection; };

private:
		Selection*		selection;
};


class Rotate90CounterclockwiseManipulator : public Manipulator {
public:
						Rotate90CounterclockwiseManipulator();
		BBitmap*		ManipulateBitmap(BBitmap* original, BStatusBar*);
		void			SetSelection(Selection* new_selection) { selection = new_selection; };
		const char*		ReturnName();

private:
		Selection*		selection;
};


#endif // ROTATE_90_MANIPULATOR_H
