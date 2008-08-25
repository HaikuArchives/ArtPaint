/* 

	Filename:	Bump.h
	Contents:	Bump-manipulator declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef BUMP_H
#define BUMP_H

#include "GUIManipulator.h"
#include "Controls.h"
#include "UtilityClasses.h"

#define	BUMP_AMOUNT_CHANGED			'Wamc'
#define	BUMP_LENGTH_CHANGED			'Wlec'

#define	MAX_BUMP_AMOUNT		100
#define MAX_BUMP_LENGTH		200

#define	INITIAL_BUMP_AMOUNT		30
#define	INITIAL_BUMP_LENGTH		30

enum preview_levels {
	FULL_CALCULATION,
	GOOD_PREVIEW,
	QUICK_PREVIEW
};

struct thread_data {
	uint32			*target_bits;
	uint32			*source_bits;
	int32			target_bpr;
	int32			source_bpr;
	
	int32			start_y;
	int32			end_y;
	float 			center_x;
	float			center_y;
	
	float			top;
	float			bottom;

	
	ViewManipulator	*the_manipulator;	
	
	int32			bump_amount;
	int32			bump_length;
	int32			dampening;
};

class BumpManipulator : public GUIManipulator {
BBitmap				*target_bitmap;
BBitmap				*original_bitmap;	// This must be deleted in destructor
BWindow				*bump_window;
manipulator_data	the_data;
BView				*status_view;
float				progress_step;
bool				preview_enabled;
preview_levels		preview_level;

static	int32		CalculateBump(void*);		
		void		calculate_bump();
static	int32		threadEntry(void*);
		int32		bump_func(uint32*,uint32*,int32,int32,int32,int32,int32,int32,int32,float,float,float,float);

int32				bump_amount;
int32				bump_length;

float				center_x;
float				center_y;
float				*sin_table;

BNode				*node;
bool				status;

public:
			BumpManipulator(BView*,manipulator_data&);
			~BumpManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,BView*,float);
void		FinishManipulation(bool);
void		MouseDown(BPoint,uint32,uint32,GET_MOUSE);
void		ChangeValue(int32,preview_levels);
void 		ChangeBumpLength(int32,preview_levels);
};


// This AddOnWindow should send message to the target-view, when
// it finds out that the user has finished with the interface.
// The message should be HS_OPERATION_FINISHED and should continue a
// boolean 'status' indicating whether the user accepted the manipulations
// or discarded them.
class BumpWindow : public BWindow {
BView					*target_view;
BumpManipulator			*the_manipulator;	
BButton					*ok_button,*cancel_button;
BStatusBar				*preview_progress_bar;
BBox					*background;

bool					status;
bool					preview_finished;

public:
		BumpWindow(BView*,BumpManipulator*,int32,int32);
		~BumpWindow();

	
void	MessageReceived(BMessage*);

BView*	DisplayProgressBar();
void	DisplayConfirmButtons();
void	PreviewFinished(bool f) { preview_finished = f; }
};


inline 	asm	float reciprocal_of_square_root(register float number)
{
	machine		603
	frsqrte		fp1,number;	// Estimates reciprocal of square-root
	blr
}


inline	uint32 add_pixels(uint32 c1,uint32 c2,float coeff_c2)
{
	// This function adds c2*coeff_c2 to c1 and does not check if
	// values exceed 255 or 0.
	return	((uint32)(((c1 >> 24) & 0xFF) + ((c2 >> 24) & 0xFF)*coeff_c2) << 24) |
			((uint32)(((c1 >> 16) & 0xFF) + ((c2 >> 16) & 0xFF)*coeff_c2) << 16) |
			((uint32)(((c1 >> 8) & 0xFF) + ((c2 >> 8) & 0xFF)*coeff_c2) << 8) |
			((uint32)(((c1) & 0xFF) + ((c2) & 0xFF)*coeff_c2));
}

#endif



