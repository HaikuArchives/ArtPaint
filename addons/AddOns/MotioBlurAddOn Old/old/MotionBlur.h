/* 

	Filename:	MotionBlur.h
	Contents:	MotionBlur-manipulator declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef COLOR_BALANCE_H
#define COLOR_BALANCE_H

#include "GUIManipulator.h"
#include "Controls.h"
#include "UtilityClasses.h"

#define	BLUR_AMOUNT_CHANGED			'Bamc'
#define	BLUR_TRANSPARENCY_CHANGED	'Btpc'
#define BLUR_DIRECTION_CHANGED		'Bdic'
#define	MAX_BLUR_AMOUNT				50

struct thread_data {
	uint32			*target_bits;
	uint32			*source_bits;
	int32			target_bpr;
	int32			source_bpr;
	
	int32			start_y;
	int32			end_y;
	ViewManipulator	*the_manipulator;	
	
	int32			blur_amount;
	int32			blur_alpha;
};


struct blur_delta {
	int32	dx;
	int32	dy;
	float	coeff;
};

class MotionBlurManipulator : public GUIManipulator {
BBitmap				*target_bitmap;
BBitmap				*original_bitmap;	// This must be deleted in destructor
BWindow				*blur_window;
manipulator_data	the_data;
int32				blur_alpha;			// B_CONTROL_ON == yes
BView				*status_view;
float				progress_step;
int32				number_of_deltas;
blur_delta			*deltas;
int32				direction;
int32				blur_amount;
bool				preview_enabled;

		BBitmap*	ExtendAndCopyBitmap(BBitmap*,int32);
		void		CopySmallBitmapToExtended(BBitmap*,BBitmap*,int32);

		void		CalculateDeltas();
static	int32		CalculateBlur(void*);		
		void		calculate_blur();
static	int32		threadEntry(void*);
		int32		blur_func(uint32*,uint32*,int32,int32,int32,int32,int32,int32);
		

		BNode		*node;
		bool		status;
		
		
public:
			MotionBlurManipulator(BView*,manipulator_data&);
			~MotionBlurManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,BView*,float);
void		FinishManipulation(bool);
void		ChangeValue(int32);
void		ChangeTransparency(int32);
void		ChangeDirection(int32);
};


// This AddOnWindow should send message to the target-view, when
// it finds out that the user has finished with the interface.
// The message should be HS_OPERATION_FINISHED and should continue a
// boolean 'status' indicating whether the user accepted the manipulations
// or discarded them.
class MotionBlurWindow : public BWindow {
BView					*target_view;
MotionBlurManipulator			*the_manipulator;	
BButton					*ok_button,*cancel_button;
BStatusBar				*preview_progress_bar;
BBox					*background;

bool					status;
public:
		MotionBlurWindow(BView*,MotionBlurManipulator*,int32,int32,int32);
		~MotionBlurWindow();

	
void	MessageReceived(BMessage*);

BView*	DisplayProgressBar();
void	DisplayConfirmButtons();
};
#endif



