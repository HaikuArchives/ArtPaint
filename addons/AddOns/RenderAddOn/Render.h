/* 

	Filename:	Render.h
	Contents:	RenderManipulator-class declarations.	
	Author:		Heikki Suhonen
	
*/


#ifndef RENDER_MANIPULATOR_H
#define RENDER_MANIPULATOR_H

#include "WindowGUIManipulator.h"

class RenderManipulator : public WindowGUIManipulator {
public:
			RenderManipulator(BBitmap*);
			~RenderManipulator();
			
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



