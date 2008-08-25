/* 

	Filename:	Impressionist.h
	Contents:	Impressionist-manipulator declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef IMPRESSIONIST_H
#define IMPRESSIONIST_H

#include "GUIManipulator.h"
#include "Controls.h"
#include "UtilityClasses.h"
#include "DataQueue.h"

#define	STROKE_LENGTH_CHANGED		'Slnc'
#define STROKE_DIRECTION_CHANGED	'Bdic'
#define STROKE_WIDTH_CHANGED		'Swic'
#define	MAX_STROKE_LENGTH			50
#define	MIN_STROKE_LENGTH			1
#define MAX_STROKE_WIDTH			20
#define MIN_STROKE_WIDTH			2


struct thread_data {
	BBitmap			*target;
	BBitmap			*source;
	
	int32			start_y;
	int32			end_y;
	int32			left;
	int32			right;
	ViewManipulator	*the_manipulator;	
	float			stroke_direction;
	float			stroke_length;
	float			stroke_width;
};

struct data_item {
	int32 x1;
	int32 y1;
	int32 x2;
	int32 y2;
	int32 cx;
	int32 cy;
	float width;
};

class ImpressionistManipulator : public GUIManipulator {
BBitmap					*target_bitmap;
BBitmap					*original_bitmap;	// This must be deleted in destructor
BWindow					*impressionist_window;
manipulator_data		the_data;
BView					*status_view;
float					progress_step;
float					stroke_direction;
float					stroke_length;
float					stroke_width;
BPoint					preview_location;

DataQueue<data_item>	*data_queue;


static	int32		CalculateEffect(void*);		
		void		calculate_effect();

static	int32		CalculateEntry(void*);
		int32		calculate_func(int32,int32,int32,int32,float,float,float);

static	int32		RenderEntry(void*);
		int32		render_func(BBitmap*,BBitmap*);

		bool		calculating;

		BNode		*node;
		bool		status;
public:
			ImpressionistManipulator(BView*,manipulator_data&);
			~ImpressionistManipulator();
			
BBitmap*	ManipulateBitmap(BBitmap*,BView*,float);
void		FinishManipulation(bool);
void		MouseDown(BPoint,uint32,uint32,GET_MOUSE);

void		ChangeValue(int32);
void		ChangeWidth(int32);
void		ChangeTransparency(int32);
void		ChangeDirection(int32);
void		CalculatePreview(BBitmap*);		
};


// This AddOnWindow should send message to the target-view, when
// it finds out that the user has finished with the interface.
// The message should be HS_OPERATION_FINISHED and should continue a
// boolean 'status' indicating whether the user accepted the manipulations
// or discarded them.
class ImpressionistWindow : public BWindow {
BView						*target_view;
ImpressionistManipulator	*the_manipulator;	
BButton						*ok_button,*cancel_button;
BBox						*background;
BitmapViewBox				*preview_view;
BBitmap						*preview_bitmap;

bool						status;

public:
		ImpressionistWindow(BView*,ImpressionistManipulator*,float,float,float);
		~ImpressionistWindow();

	
void	MessageReceived(BMessage*);

void	UpdatePreview();
};
#endif



