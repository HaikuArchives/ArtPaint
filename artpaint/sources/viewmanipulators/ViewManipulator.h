/*

	Filename:	ViewManipulator.h
	Contents:	Declaration for abstract ViewManipulator-base-class
	Author:		Heikki Suhonen

*/

#ifndef	VIEW_MANIPULATOR_H
#define	VIEW_MANIPULATOR_H


class Selection;



// This typedef represents a pointer to function that can read image-view's
// mouse coordinates.
typedef		void	(*GET_MOUSE)(BView*,BPoint*,uint32*,BPoint*);

// This typedef represents a pointer to a function that can be called
// from manipulator to update the image.
typedef		void	(*UPDATE_IMAGE)(BView*,BRect,bool);


#define	ADD_ON_API_VERSION	0x02

// This struct is used to transmit information from the view's internals
// to the ViewManipulator
struct manipulator_data {
	BBitmap			*composite_picture;
	BBitmap			*current_layer;
	bool			manipulate_composite;	// This is to be used only with internal
											// manipulators (i.e. not with add-ons).
											// The default is FALSE.

	bool			manipulate_horizontal;	// These fields contain information about
	bool			manipulate_vertical;	// The direction of manipulation, e.g.
											// a vertical flip.


	BNode			*node;					// This allows the add-ons to save the
											// settings as attributes to their files.


	Selection		*selection;				// This is a pointer to view's selection.
											// If selection is not empty, the manipulator
											// should only manipulate selected area.

	UPDATE_IMAGE	image_updater;
};

// These constants are used in communication between various actions
// and the ImageView-class.
#define	HS_ACTION_FINISHED		'AcFn'

// This is also defined in MessageConstants.h. If we change this definition, we must
// change it there too.
#ifndef	HS_OPERATION_FINISHED
#define	HS_OPERATION_FINISHED	'OpFn'
#endif


// This class is a base class for classes that invoke events when user
// presses down a mouse-button in the image. The derived classes also
// implement the Draw-function that can be used to draw some info
// onto the image. If manipulator creates a UI, it should send a HS_OPERATION_FINISHED,
// when the user closes the interface. It should contain member 'status' that is boolean
// and if it is true indicates that user accepted the manipulation.
// Whenever the action started from mouse-down ends the manipulator should
// send a message containing HS_ACTION_FINISHED.
class ViewManipulator {
protected:
				BView	*target_view;		// This should not be deleted or changed.

public:
				// We should get the manipulator_data in here.
				ViewManipulator(BView *target)
					: target_view(target) {};

virtual			~ViewManipulator() {};


// If the manipulator needs to draw some user interface
// to the target view this is the place to do it.
// The target_view has already been locked when this is called.
virtual	void	Draw(float) {};

// This function should create a thread and return as quickly as possible.
// This is called whenever the mouse is pressed down in the image-view.
virtual	void	MouseDown(BPoint location,uint32 buttons,uint32 modifiers,GET_MOUSE mouse_function) = 0;


};
#endif
