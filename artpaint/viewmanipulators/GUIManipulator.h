/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	GUI_MANIPULATOR_H
#define	GUI_MANIPULATOR_H

#include "Manipulator.h"

#include <Region.h>


class BNode;
class BMessage;
class BPoint;
class BView;


#define	HS_MANIPULATOR_ADJUSTING_STARTED	'Mast'
#define	HS_MANIPULATOR_ADJUSTING_FINISHED	'Mafi'

enum draw_resolutions {
	DRAW_NOTHING = 0,
	DRAW_ONLY_GUI = -1,
	DRAW_16_X_16 = 16,
	DRAW_8_X_8 = 8,
	DRAW_4_X_4 = 4,
	DRAW_2_X_2 = 2,
	DRAW_1_X_1 = 1
			 };

class GUIManipulator : public Manipulator {
protected:
			BBitmap*	ManipulateBitmap(BBitmap* b, BStatusBar*) { return b; }

public:
						GUIManipulator() : Manipulator() { fEnabled = true; }

	virtual void		MouseDown(BPoint, uint32, BView*, bool) {}
	virtual	int32		PreviewBitmap(bool full_quality = FALSE, BRegion* updated_region = NULL) = 0;
	virtual	BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*) = 0;
	virtual	BRegion		Draw(BView*, float) { return BRegion(); }
	virtual	void		Reset() = 0;
	virtual	void		SetPreviewBitmap(BBitmap*) = 0;
	virtual	const void*	ManipulatorCursor() { return NULL; }
	virtual	const char*	ReturnHelpString() = 0;

	ManipulatorSettings*	ReturnSettings() = 0;

	virtual	status_t	ReadSettings(BNode*) { return B_OK; }
	virtual	status_t	WriteSettings(BNode*) { return B_OK; }

	virtual status_t	Save(BMessage& settings) const { return B_OK; }
	virtual status_t	Restore(const BMessage& settings) { return B_OK; }

			void		EnableWindow(bool enable) { fEnabled = enable; }
			bool		IsWindowEnabled() { return fEnabled; }

private:
			bool		fEnabled;
};


#endif // GUI_MANIPULATOR_H
