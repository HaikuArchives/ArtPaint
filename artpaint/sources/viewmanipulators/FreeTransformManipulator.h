/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	FREE_TRANSFORM_MANIPULATOR_H
#define	FREE_TRANSFORM_MANIPULATOR_H

#include "WindowGUIManipulator.h"


enum {
	RESIZING_MODE = 1,
	TRANSLATING_MODE,
	ROTATING_MODE
};

class FreeTransformManipulatorView;

class FreeTransformManipulatorSettings : public ManipulatorSettings {
public:
		FreeTransformManipulatorSettings() {
			rotation = 0.0;
			x_translation = 0.0;
			y_translation = 0.0;
			x_scale_factor = 1.0;
			y_scale_factor = 1.0;
		}

		FreeTransformManipulatorSettings(const FreeTransformManipulatorSettings &s) {
			rotation = s.rotation;
			x_translation = s.x_translation;
			y_translation = s.y_translation;
			x_scale_factor = s.x_scale_factor;
			y_scale_factor = s.y_scale_factor;
		}

		const FreeTransformManipulatorSettings& operator=(const FreeTransformManipulatorSettings &s) {
			rotation = s.rotation;
			x_translation = s.x_translation;
			y_translation = s.y_translation;
			x_scale_factor = s.x_scale_factor;
			y_scale_factor = s.y_scale_factor;

			return *this;
		}

		bool operator==(const FreeTransformManipulatorSettings &s) {
			return (	(rotation == s.rotation) && (x_translation == s.x_translation) &&
					(y_translation == s.y_translation) && (x_scale_factor == s.x_scale_factor) &&
					(y_scale_factor == s.y_scale_factor));

		}

		bool operator!=(const FreeTransformManipulatorSettings &s) {
			return !(*this == s);
		}


// The rotation is in degrees between -180 and 180. The rotation will be done around the center
// of the image.
float	rotation;

// These are floats in order to avoid casting when calculating the result.
float	x_translation;
float	y_translation;

// When these factors are 1.0 the result is the same size as the original. A factor of 0.5 means that
// the dimension is half from the original.
float	x_scale_factor;
float	y_scale_factor;
};


class FreeTransformManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, Selection* s, BStatusBar* stb)
		{ return WindowGUIManipulator::ManipulateBitmap(b, s, stb); };

	BBitmap						*preview_bitmap;
	BBitmap						*copy_of_the_preview_bitmap;

	FreeTransformManipulatorView		*configuration_view;
	FreeTransformManipulatorSettings	settings;
	FreeTransformManipulatorSettings	previous_settings;

	int32								transformation_mode;
	BPoint								starting_point;

public:
			FreeTransformManipulator(BBitmap*);
			~FreeTransformManipulator();

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap *original,Selection*,BStatusBar*);
int32		PreviewBitmap(Selection*,bool,BRegion* =NULL);

void			MouseDown(BPoint,uint32,BView*,bool);
void			ChangeSettings(ManipulatorSettings* settings);

BView*			MakeConfigurationView(const BMessenger& target);
void			Reset(Selection*);
void			SetPreviewBitmap(BBitmap*);

const char*		ReturnHelpString() { return "Click on the image to rotate, move or stretch it."; }
const char*		ReturnName() { return "Free 2D Transform"; }


ManipulatorSettings*	ReturnSettings();
};



class FreeTransformManipulatorView : public WindowGUIManipulatorView {
	FreeTransformManipulator	*manipulator;

	FreeTransformManipulatorSettings	settings;

	BMessenger *target;

public:
	FreeTransformManipulatorView(FreeTransformManipulator* manipulator,
								 const BMessenger& target);
	~FreeTransformManipulatorView();

	void	AttachedToWindow();

	void	MessageReceived(BMessage*);
	void ChangeSettings(ManipulatorSettings* settings);
};

#endif
