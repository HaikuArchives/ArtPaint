/*

	Filename:	TranslationManipulator.h
	Contents:	TranslationManipulator-class declaration
	Author:		Heikki Suhonen

*/


#include "StatusBarGUIManipulator.h"

#ifndef	TRANSLATION_MANIPULATOR_H
#define	TRANSLATION_MANIPULATOR_H


class TranslationManipulatorView;
class TranslationManipulatorSettings;
class NumberControl;

// This is a translation-cursor.
const unsigned char HS_TRANSLATION_CURSOR[] =
		{
			0x10, 0x01, 0x07, 0x07,

			// here starts the image data
			0x01, 0x00, 0x03, 0x80,		// lines 0 and 1
			0x07, 0xC0, 0x01, 0x00,		// lines 2 and 3
			0x01, 0x00, 0x21, 0x08,		// lines 4 and 5
			0x60, 0x0C, 0xFC, 0x7E,		// lines 6 and 7
			0x60, 0x0C, 0x21, 0x08,		// lines 8 and 9
			0x01, 0x00, 0x01, 0x00,		// lines 10 and 11
			0x07, 0xC0, 0x03, 0x80,		// lines 12 and 13
			0x01, 0x00, 0x00, 0x00,		// lines 14 and 15

			// here starts the mask-data
			0x03, 0x80, 0x07, 0xC0,		// lines 0 and 1
			0x0F, 0xE0, 0x03, 0x80,		// lines 2 and 3
			0x23, 0x88, 0x63, 0x8C,		// lines 4 and 5
			0xFC, 0x7E, 0xFC, 0x7E,		// lines 6 and 7
			0xFC, 0x7E, 0x63, 0x8C,		// lines 8 and 9
			0x23, 0x88, 0x03, 0x80,		// lines 10 and 11
			0x0F, 0xE0, 0x07, 0xC0,		// lines 12 and 13
			0x03, 0x80, 0x00, 0x00,		// lines 14 and 15
		};

class TranslationManipulator: public StatusBarGUIManipulator {
		BBitmap	*preview_bitmap;
		BBitmap	*copy_of_the_preview_bitmap;

		int32	previous_x_translation;
		int32	previous_y_translation;

		BPoint	previous_point;

		BRect	uncleared_rect;

		TranslationManipulatorSettings	*settings;
		TranslationManipulatorView		*config_view;


		int32	last_calculated_resolution;
		int32	lowest_available_quality;
		int32	highest_available_quality;

public:
			TranslationManipulator(BBitmap*);
			~TranslationManipulator();

void			MouseDown(BPoint,uint32,BView*,bool);

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap *original,Selection*,BStatusBar*);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion *updated_region=NULL);
BView*		MakeConfigurationView(float width, float height, BMessenger*);
void			SetPreviewBitmap(BBitmap*);
void			Reset(Selection*);

const	char*	ReturnName();
const	char*	ReturnHelpString();

const	void*	ManipulatorCursor() { return HS_TRANSLATION_CURSOR; }

ManipulatorSettings*	ReturnSettings();
void		SetValues(float,float);
};



class TranslationManipulatorSettings : public ManipulatorSettings {
public:
		TranslationManipulatorSettings()
			: ManipulatorSettings() {
				x_translation = 0;
				y_translation = 0;
			}

		TranslationManipulatorSettings(TranslationManipulatorSettings *s)
			: ManipulatorSettings() {
				x_translation = s->x_translation;
				y_translation = s->y_translation;
			}


float	x_translation;
float	y_translation;
};



class TranslationManipulatorView : public BView {
		NumberControl			*x_control;
		NumberControl			*y_control;
		BMessenger				*target;
		TranslationManipulator	*manipulator;

public:
		TranslationManipulatorView(BRect,TranslationManipulator*);
		~TranslationManipulatorView();

void		AttachedToWindow();
void		MessageReceived(BMessage*);

void		SetValues(float x, float y);
void		SetTarget(const BMessenger *t);
};

#endif
