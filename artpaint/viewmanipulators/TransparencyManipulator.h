/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TRANSPARENCY_MANIPULATOR_H
#define TRANSPARENCY_MANIPULATOR_H

#include "ImageAdapter.h"
#include "ManipulatorSettings.h"
#include "UtilityClasses.h"
#include "WindowGUIManipulator.h"


#include <Messenger.h>


#define	TRANSPARENCY_CHANGED			'TrCh'
#define	MOUSE_TRACKING_FINISHED			'Mstf'

enum transparency_modes {
	RELATIVE_TRANSPARENCY,
	ABSOLUTE_TRANSPARENCY
};


class BSlider;
class Layer;
class TransparencyManipulatorSettings;
class TransparencyManipulatorView;


class TransparencyManipulator : public WindowGUIManipulator, public ImageAdapter {
	BBitmap*	ManipulateBitmap(BBitmap* b, BStatusBar* stb)
		{ return WindowGUIManipulator::ManipulateBitmap(b, stb); }

BBitmap				*preview_bitmap;
BBitmap				*copy_of_the_preview_bitmap;

Layer				*preview_layer;

int32				transparency;
int32				last_calculated_resolution;
int32				lowest_available_quality;
int32				highest_available_quality;

float				previous_transparency_change;
float				original_transparency_coefficient;


TransparencyManipulatorSettings	*settings;
TransparencyManipulatorView		*config_view;

	Selection*	selection;

public:
			TransparencyManipulator(BBitmap*);
			~TransparencyManipulator();

BBitmap*	ManipulateBitmap(ManipulatorSettings*, BBitmap*, BStatusBar*);
int32		PreviewBitmap(bool full_quality = FALSE,
				BRegion* updated_region = NULL);
BView*		MakeConfigurationView(const BMessenger& target);

void		Reset();
void		SetPreviewBitmap(BBitmap*);

const	char*	ReturnHelpString();
const	char*	ReturnName();

void		SetTransparency(float);

ManipulatorSettings*	ReturnSettings();

void		SetSelection(Selection* new_selection)
				{ selection = new_selection; };
};


class TransparencyManipulatorSettings : public ManipulatorSettings {
public:
	TransparencyManipulatorSettings()
		: ManipulatorSettings() {
			transparency = 0;
		}

	TransparencyManipulatorSettings(TransparencyManipulatorSettings *s)
		: ManipulatorSettings() {
			transparency = s->transparency;
		}


	float	transparency;
};




class TransparencyManipulatorView : public WindowGUIManipulatorView {
public:
										TransparencyManipulatorView(
											TransparencyManipulator* manipulator,
											const BMessenger& target);
	virtual								~TransparencyManipulatorView() {}

	virtual	void						AllAttached();
	virtual	void						AttachedToWindow();
	virtual	void						MessageReceived(BMessage* message);

			void						ChangeSettings(TransparencyManipulatorSettings*);

private:
			BMessenger					fTarget;
			bool						fTracking;
			TransparencyManipulatorSettings	settings;

			TransparencyManipulator*	fManipulator;
			BSlider*					fTransparency;
};

#endif



