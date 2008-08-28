/* 

	Filename:	BasicManipulator.h
	Contents:	Declaration for abstract base-class for actual manipulators	
	Author:		Heikki Suhonen
	
*/

#ifndef	BASIC_MANIPULATOR
#define	BASIC_MANIPULATOR	


#include "ViewManipulator.h"


// The classes derived from this class can manipulate the bitmap in any way they like.
// The manipulation should be done in ManipulateBitmap-function.
class BasicManipulator : public ViewManipulator {
public:
					// We should get the manipulator_data in here.
					BasicManipulator(BView *target) 
						: ViewManipulator(target) {};

virtual				~BasicManipulator() {};


		void		MouseDown(BPoint,uint32,uint32,GET_MOUSE) {};	// See ViewManipulator.h for
														// description of this.

virtual	char*		Label() { return "Finishing operation"; }

// This function will be called for manipulators to manipulate the parameter
// original bitmap with the manipulators current settings. It will then return
// the new bitmap. The returned bitmap can be the same as the parameter. If
// a different bitmap is returned, the original bitmap must NOT be deleted.
// If paremeter progress is not NULL, we should send to that view B_UPDATE_STATUS_BAR
// messages. The deltas of all sent messages should add up to 100*prog_step.
// The number of sent messages should not be very high, but it would be a good
// idea to send at least 10 messages (if this function takes almost no time at all,
// we can probably send only 1 message).
virtual	BBitmap*	ManipulateBitmap(BBitmap *original,BView *progress=NULL,float prog_step=0) =0;
};
#endif
