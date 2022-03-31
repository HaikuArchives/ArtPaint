/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "AddOnTypes.h"
#include "$HEADER_NAME"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "$MENU_NAME";
	char menu_help_string[255] = "$MENU_HELP_TEXT";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = GENERIC_ADD_ON;

	extern Manipulator* manipulator_creator(BBitmap*);
#ifdef __cplusplus
}
#endif


Manipulator* manipulator_creator(BBitmap *bm)
{
	return new $MANIPULATOR_NAME(bm);
}



$MANIPULATOR_NAME::$MANIPULATOR_NAME(BBitmap *bm)
		: WindowGUIManipulator()
{
	SetPreviewBitmap(bm);
}


$MANIPULATOR_NAME::~$MANIPULATOR_NAME()
{
}


BBitmap* $MANIPULATOR_NAME::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
}

int32 $MANIPULATOR_NAME::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
}


void $MANIPULATOR_NAME::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
}


void $MANIPULATOR_NAME::SetPreviewBitmap(BBitmap *bm)
{
}


void $MANIPULATOR_NAME::Reset(Selection*)
{
}

BView* $MANIPULATOR_NAME::MakeConfigurationView(const BMessenger& target)
{
}


ManipulatorSettings* $MANIPULATOR_NAME::ReturnSettings()
{
}

void $MANIPULATOR_NAME::ChangeSettings(ManipulatorSettings *s)
{
}

const char* $MANIPULATOR_NAME::ReturnName()
{
	return "A WindowGUIManipulator Name";
}

const char* $MANIPULATOR_NAME::ReturnHelpString()
{
	return "A string that is displayed in the status-bar while the manipulator"
		" is active. Should not be this long though.";
}
