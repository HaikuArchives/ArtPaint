/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TRANSLATION_MANIPULATOR_H
#define TRANSLATION_MANIPULATOR_H

#include "Cursors.h"
#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


class Selection;
class TranslationManipulatorView;
class TranslationManipulatorSettings;


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}
using ArtPaint::Interface::NumberControl;


class TranslationManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, Selection* s, BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, s, stb); }

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
	BView*		MakeConfigurationView(const BMessenger& target);
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


class TranslationManipulatorView : public WindowGUIManipulatorView {
public:
										TranslationManipulatorView(
											TranslationManipulator* manipulator,
											const BMessenger& target);
	virtual								~TranslationManipulatorView() {}

	virtual	void						AttachedToWindow();
	virtual	void						MessageReceived(BMessage* message);

			void						SetValues(float x, float y);
			void						SetTarget(const BMessenger& target);

private:
			BMessenger					fTarget;

			NumberControl*				fXControl;
			NumberControl*				fYControl;
			TranslationManipulator*		fManipulator;
};

#endif	// TRANSLATION_MANIPULATOR_H
