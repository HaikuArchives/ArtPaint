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

extern "C" __declspec(dllexport) Manipulator* manipulator_creator(BBitmap*);
extern "C" __declspec(dllexport) char name[255] = "$MENU_NAME";
extern "C" __declspec(dllexport) char menu_help_string[255] = "$MENU_HELP_TEXT";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = GENERIC_ADD_ON;


Manipulator* manipulator_creator(BBitmap*)
{
	return new $MANIPULATOR_NAME();
}



$MANIPULATOR_NAME::$MANIPULATOR_NAME()
		: Manipulator()
{
}


$MANIPULATOR_NAME::~$MANIPULATOR_NAME()
{
}


BBitmap* $MANIPULATOR_NAME::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	if (selection->IsEmpty()) {
		// Here handle the whole image.
	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
	}
}

char* $MANIPULATOR_NAME::ReturnName()
{
	return "Template Add-On";
}
