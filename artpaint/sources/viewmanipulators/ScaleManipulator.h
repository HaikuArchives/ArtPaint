/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef SCALE_MANIPULATOR_H
#define	SCALE_MANIPULATOR_H

#include "WindowGUIManipulator.h"


#include <Messenger.h>


#define	WIDTH_CHANGED		'Wich'
#define	HEIGHT_CHANGED		'Hech'

#define MULTIPLY_WIDTH		'mlWi'
#define MULTIPLY_HEIGHT		'mlHe'

#define RESTORE_WIDTH		'Rswd'
#define RESTORE_HEIGHT		'Rshg'

#define	PROPORTION_CHANGED	'Prpc'

class BButton;
class ScaleManipulatorView;


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}
using ArtPaint::Interface::NumberControl;


class ScaleManipulatorSettings : public ManipulatorSettings {
public:
	ScaleManipulatorSettings()
		: ManipulatorSettings() {
		width_coefficient = 1.0;
		height_coefficient = 1.0;
	}

	ScaleManipulatorSettings(ScaleManipulatorSettings *s)
		: ManipulatorSettings() {
		width_coefficient = s->width_coefficient;
		height_coefficient = s->height_coefficient;
	}


	float	height_coefficient;
	float	width_coefficient;
};



class ScaleManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, Selection* s, BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, s, stb); }

	BBitmap						*preview_bitmap;
	BBitmap						*copy_of_the_preview_bitmap;

	ScaleManipulatorView		*configuration_view;
	ScaleManipulatorSettings	*settings;



	float						original_width;
	float						original_height;

public:
	ScaleManipulator(BBitmap*);
	~ScaleManipulator();

	BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap *original,
					Selection*,BStatusBar*);
	int32		PreviewBitmap(Selection*,bool,BRegion* =NULL);

	void		MouseDown(BPoint,uint32,BView*,bool);
	void		SetValues(float,float);

	BView*		MakeConfigurationView(const BMessenger& target);
	void		Reset(Selection*);
	void		SetPreviewBitmap(BBitmap*);

	const	char*	ReturnHelpString();
	const	char*	ReturnName();


	ManipulatorSettings*	ReturnSettings();
};


class ScaleManipulatorView : public WindowGUIManipulatorView {
public:
								ScaleManipulatorView(ScaleManipulator*,
									const BMessenger& target);
	virtual						~ScaleManipulatorView() {}

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

			bool				MaintainProportions() {
									return maintain_proportions;
								}
			void				SetValues(float width, float height);

private:
			void				_SetTarget(BView* view);
			void				_SetValues(float width, float height);
			BButton*			_MakeButton(const char* label,
									uint32 what, float coefficient);

private:
			BMessenger			fTarget;
			float				original_width;
			float				original_height;
			float				current_width;
			float				current_height;
			bool				maintain_proportions;

			ScaleManipulator*	fManipulator;
			NumberControl*		width_control;
			NumberControl*		height_control;
};

#endif
