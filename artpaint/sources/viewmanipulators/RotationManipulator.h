/*

	Filename:	RotationManipulator.h
	Contents:	RotationManipulator-class declaration
	Author:		Heikki Suhonen

*/

#include <TextControl.h>

#include "StatusBarGUIManipulator.h"

#ifndef	ROTATION_MANIPULATOR_H
#define	ROTATION_MANIPULATOR_H

// This manipulator handles angles. The supported angle value are
// between -180˚ and +180. Negative angles are clockwise and positive counterclockwise.
// This is a rotation-cursor.


//class RotationDrawer;
class RotationManipulatorConfigurationView;
class HSPolygon;

class RotationManipulatorSettings : public ManipulatorSettings {
public:
	RotationManipulatorSettings()
		: ManipulatorSettings() { angle = 0; origo = BPoint(0,0); }

	RotationManipulatorSettings(RotationManipulatorSettings *s)
		: ManipulatorSettings() { angle = s->angle; origo = s->origo; }


	float	angle;
	BPoint	origo;
};

const unsigned char HS_ROTATION_CURSOR[] =
		{
			0x10, 0x01, 0x03, 0x03,

			// here starts the image data
			0x00, 0x00, 0x00, 0x00,		// lines 0 and 1
			0x18, 0x1C, 0x24, 0x18,		// lines 2 and 3
			0x24, 0x14, 0x18, 0x02,		// lines 4 and 5
			0x00, 0x02, 0x00, 0x02,		// lines 6 and 7
			0x00, 0x02, 0x00, 0x02,		// lines 8 and 9
			0x00, 0x04, 0x70, 0x04,		// lines 10 and 11
			0x60, 0x08, 0x58, 0x30,		// lines 12 and 13
			0x07, 0xC0, 0x00, 0x00,		// lines 14 and 15

			// here starts the mask-data
			0x00, 0x00, 0x18, 0x3C,		// lines 0 and 1
			0x38, 0x3C, 0x64, 0x38,		// lines 2 and 3
			0x64, 0x34, 0x38, 0x06,		// lines 4 and 5
			0x00, 0x06, 0x00, 0x06,		// lines 6 and 7
			0x00, 0x06, 0x00, 0x06,		// lines 8 and 9
			0xF0, 0x0C, 0xF0, 0x0C,		// lines 10 and 11
			0xF8, 0x38, 0xDF, 0xF0,		// lines 12 and 13
			0x07, 0xC0, 0x00, 0x00		// lines 14 and 15
		};

class RotationManipulator: public StatusBarGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, Selection* s, BStatusBar* stb)
		{ return StatusBarGUIManipulator::ManipulateBitmap(b, s, stb); };

		BBitmap	*copy_of_the_preview_bitmap;
		BBitmap	*preview_bitmap;


		float	previous_angle;
		float	starting_angle;
		BPoint	previous_origo;
		int32	last_calculated_resolution;
		int32	lowest_available_quality;
		int32	highest_available_quality;

		RotationManipulatorSettings				*settings;

		HSPolygon								*view_polygon;

		RotationManipulatorConfigurationView	*config_view;

		bool	move_origo;

public:
			RotationManipulator(BBitmap*);
			~RotationManipulator();

void		SetPreviewBitmap(BBitmap*);

BRegion		Draw(BView*,float);
void		MouseDown(BPoint,uint32,BView*,bool);

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion *updated_region=NULL);
void			Reset(Selection*);


const char*	ReturnHelpString();
const char*	ReturnName();

BView*		MakeConfigurationView(float,float,BMessenger*);

ManipulatorSettings*	ReturnSettings() { return new RotationManipulatorSettings(settings); }

const	void*		ManipulatorCursor() { return HS_ROTATION_CURSOR; };


void		SetAngle(float);
};




class RotationManipulatorConfigurationView : public BView {
	BTextControl		*text_control;
	BMessenger			*target;
	RotationManipulator	*manipulator;

public:
		RotationManipulatorConfigurationView(BRect,RotationManipulator*);

void	AttachedToWindow();
void	MessageReceived(BMessage*);


void	SetAngle(float angle);
void	SetTarget(const BMessenger*);

};


#endif
