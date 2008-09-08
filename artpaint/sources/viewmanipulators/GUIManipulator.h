/*

	Filename:		GUIManipulator.h
	Contents:		Declaration for abstract base-class for manipulators that create a GUI
	Author:		Heikki Suhonen

*/

#ifndef	GUI_MANIPULATOR_H
#define	GUI_MANIPULATOR_H

#include <Region.h>
#include <Node.h>

#include "Manipulator.h"

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
BBitmap*	ManipulateBitmap(BBitmap *b,Selection*,BStatusBar*) { return b; };

public:
					GUIManipulator() : Manipulator() {};
virtual				~GUIManipulator() {};


virtual	void		MouseDown(BPoint,uint32,BView*,bool) {};
virtual	int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion *updated_region=NULL) = 0;
virtual	BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*) = 0;
virtual	BRegion		Draw(BView*,float) { return BRegion(); };
virtual	void		Reset(Selection*) = 0;
virtual	void		SetPreviewBitmap(BBitmap*) = 0;
virtual	const	void*		ManipulatorCursor() { return NULL; };
virtual	const	char*		ReturnHelpString() = 0;
virtual	void		ChangeSettings(ManipulatorSettings*) {};

ManipulatorSettings*	ReturnSettings() = 0;

virtual	status_t	ReadSettings(BNode*) { return B_NO_ERROR; }
virtual	status_t	WriteSettings(BNode*) { return B_NO_ERROR; }
};
#endif
