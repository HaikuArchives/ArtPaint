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
