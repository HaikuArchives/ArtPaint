/* 

	Filename:	Impressionist.h
	Contents:	Impressionist-manipulator declaration.	
	Author:		Heikki Suhonen
	
*/


#ifndef IMPRESSIONIST_H
#define IMPRESSIONIST_H

#include "WindowGUIManipulator.h"
#include "Controls.h"
//#include "UtilityClasses.h"
#include "DataQueue.h"

#define	STROKE_LENGTH_CHANGED		'Slnc'
#define STROKE_DIRECTION_CHANGED	'Bdic'
#define STROKE_WIDTH_CHANGED		'Swic'
#define	MAX_STROKE_LENGTH			50
#define	MIN_STROKE_LENGTH			1
#define MAX_STROKE_WIDTH			20
#define MIN_STROKE_WIDTH			2

#define	PREVIEW_HEIGHT				100
#define	PREVIEW_WIDTH				100

class ImpressionistManipulatorSettings : public ManipulatorSettings {
public:
	ImpressionistManipulatorSettings()
		: ManipulatorSettings()	{
		width = 2.0;
		length = 2.0;	
		direction = 45.0;
		preview_location = BPoint(0,0);
	}


	ImpressionistManipulatorSettings(ImpressionistManipulatorSettings *s)
		: ManipulatorSettings() {
		width = s->width;
		length = s->length;
		direction = s->direction;
		preview_location = s->preview_location; 		
	}

	ImpressionistManipulatorSettings operator=(ImpressionistManipulatorSettings s) {
		width = s.width;
		length = s.length;
		direction = s.direction;
		preview_location = s.preview_location;
		return this;
	}

	bool operator==(ImpressionistManipulatorSettings s) {
		return ((width == s.width) && (length == s.length) && (direction == s.direction) && (preview_location == s.preview_location)); 			
	}
	bool operator!=(ImpressionistManipulatorSettings s) {
		return !(*this == s);
	}
	
	float	width;
	float	length;
	float	direction;
	BPoint	preview_location;	
};


class ImpressionistManipulatorView;

//struct thread_data {
//	BBitmap			*target;
//	BBitmap			*source;
//	
//	int32			start_y;
//	int32			end_y;
//	int32			left;
//	int32			right;
//	ViewManipulator	*the_manipulator;	
//	float			stroke_direction;
//	float			stroke_length;
//	float			stroke_width;
//};

struct data_item {
	int32 x1;
	int32 y1;
	int32 x2;
	int32 y2;
	int32 cx;
	int32 cy;
	float width;
};

class ImpressionistManipulator : public WindowGUIManipulator {
BBitmap					*preview_bitmap;
BBitmap					*small_preview_bitmap;

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


		BBitmap*	CalculateIntensityMap(BBitmap*);	// parameter 32-bit, returns 8-bit
		BBitmap*	CalculateBlur(BBitmap*);	// parameter 8-bit, returns 8-bit, returns parameter
		BBitmap*	CalculateSobel(BBitmap*);	// parameter 8-bit, returns 8-bit, dels parameter



const	int32		random_array_length;
		int32		*random_array;


ImpressionistManipulatorView		*config_view;
ImpressionistManipulatorSettings	settings;
ImpressionistManipulatorSettings	previous_settings;
public:
			ImpressionistManipulator(BBitmap*);
			~ImpressionistManipulator();
			
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
int32		PreviewBitmap(Selection*,bool,BRegion*) { return DRAW_NOTHING; }
void		SetPreviewBitmap(BBitmap*);
void		MouseDown(BPoint,uint32,BView*,bool);
void		CalculatePreview(BBitmap*);		
void		Reset(Selection*) {};
char*		ReturnHelpString() { return "Use the sliders to control the appearance of effect."; }
char*		ReturnName() { return "Impressionist"; }

BView*		MakeConfigurationView(BMessenger*);

ManipulatorSettings*	ReturnSettings();
void		ChangeSettings(ManipulatorSettings*);
};


class ImpressionistManipulatorView : public WindowGUIManipulatorView {
		ImpressionistManipulatorSettings	settings;
		
public:
		ImpressionistManipulatorView(BRect,ImpressionistManipulator*);

void	AttachedToWindow();
void	MessageReceived(BMessage*);

void	ChangeSettings(ImpressionistManipulatorSettings*);
};

// This AddOnWindow should send message to the target-view, when
// it finds out that the user has finished with the interface.
// The message should be HS_OPERATION_FINISHED and should continue a
// boolean 'status' indicating whether the user accepted the manipulations
// or discarded them.
//class ImpressionistWindow : public BWindow {
//BView						*target_view;
//ImpressionistManipulator	*the_manipulator;	
//BButton						*ok_button,*cancel_button;
//BBox						*background;
//BitmapViewBox				*preview_view;
//BBitmap						*preview_bitmap;
//
//bool						status;
//
//public:
//		ImpressionistWindow(BView*,ImpressionistManipulator*,float,float,float);
//		~ImpressionistWindow();
//
//	
//void	MessageReceived(BMessage*);
//
//void	UpdatePreview();
//};
#endif



