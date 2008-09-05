/* 

	Filename:	Template.h
	Contents:	A template header for the ArtPaint add-ons.	
	Author:	
	
*/


#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "WindowGUIManipulator.h"

class $MANIPULATOR_NAME : public WindowGUIManipulator {
public:
			$MANIPULATOR_NAME(BBitmap*);
			~$MANIPULATOR_NAME();
			
void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);	
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString();
char*		ReturnName();

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(BMessenger*);

void		ChangeSettings(ManipulatorSettings*);		
};

#endif



