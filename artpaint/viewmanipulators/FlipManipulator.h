/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "Manipulator.h"

#ifndef FLIP_MANIPULATOR_H
#define	FLIP_MANIPULATOR_H


class HorizFlipManipulator : public Manipulator {
public:
						HorizFlipManipulator();
		BBitmap*		ManipulateBitmap(BBitmap* original, BStatusBar*);

		const char*		ReturnName();
		void			SetSelection(Selection* new_selection) { selection = new_selection; };
        void		    SetTransformSelectionOnly(bool select_only)
                            { transform_selection_only = select_only; }

		BBitmap*		ManipulateSelectionBitmap();

private:
		Selection*		selection;
        bool            transform_selection_only;
};


class VertFlipManipulator : public Manipulator {
public:
						VertFlipManipulator();
		BBitmap*		ManipulateBitmap(BBitmap* original, BStatusBar*);

		const char*		ReturnName();
		void			SetSelection(Selection* new_selection) { selection = new_selection; };
        void		    SetTransformSelectionOnly(bool select_only)
                            { transform_selection_only = select_only; }

		BBitmap*		ManipulateSelectionBitmap();

private:
		Selection*		selection;
        bool            transform_selection_only;
};


#endif // FLIP_MANIPULATOR_H
