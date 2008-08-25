/* 

	Filename:	HistogramAddOn.h
	Contents:	An example of add-on using gui.	
	Author:		Heikki Suhonen
	
*/

#ifndef HISTOGRAM_ADD_ON_H
#define HISTOGRAM_ADD_ON_H

#include "ViewManipulator.h"
#include "UtilityClasses.h"

#define	RED_HISTOGRAM	'RDhi'
#define	BLUE_HISTOGRAM	'BLhi'
#define GREEN_HISTOGRAM	'GRhi'

class HistogramWindow;

class HistogramManipulator : public ViewManipulator {
HistogramWindow		*the_window;
BBitmap				*the_buffer;

BNode				*node;
	
public:
		HistogramManipulator(BView *target,manipulator_data&);
		~HistogramManipulator();
		
void			MouseDown(BPoint location,uint32 buttons,uint32 modifiers,GET_MOUSE);
BBitmap*		generateHistogram(int32 color);
};


class HistogramWindow : public BWindow {
		BitmapView				*bgview;
		BView					*target;
		BBitmap					*histogram;
		HistogramManipulator	*the_manipulator;				
		
public:
		HistogramWindow(BView*,HistogramManipulator*);
		~HistogramWindow();

void	MessageReceived(BMessage*);
};
#endif





