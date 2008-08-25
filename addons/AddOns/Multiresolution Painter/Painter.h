/* 

	Filename:	Painter.h
	Contents:	Declarations for painter add-on.	
	Author:		Heikki Suhonen
	Notes:		The ideas in this add-on are based on article
				by Herzmann, A., Painterly Rendering with Curved Brush
				Strokes of Multiple Sizes. SIGGRAPH '98, pp. 453-480.
	
*/


#ifndef PAINTER_H
#define PAINTER_H

#include <Shape.h>

#include "WindowGUIManipulator.h"
#include "Controls.h"
#include "PreviewView.h"
#include "RangeSlider.h"
#include "BitmapAnalyzer.h"

class ImageProcessingLibrary;

class	PainterManipulatorSettings : public ManipulatorSettings {
public:
		PainterManipulatorSettings()
			: ManipulatorSettings() {
			blurring_factor = 1.0;
			approximation_threshold = 100;
			brush_radius = 2;
			brush_count = 2;
			brush_ratio = 0.5;
			min_length = 0;
			max_length = 10;
			opacity = 1.0;
		}

		PainterManipulatorSettings(const PainterManipulatorSettings& s)
			: ManipulatorSettings() {
			blurring_factor = s.blurring_factor;
			approximation_threshold = s.approximation_threshold;
			brush_radius = s.brush_radius;
			brush_count = s.brush_count;
			brush_ratio = s.brush_ratio;
			min_length = s.min_length;
			max_length = s.max_length;
			opacity = s.opacity;
		}

		PainterManipulatorSettings& operator=(const PainterManipulatorSettings& s) {
			blurring_factor = s.blurring_factor;
			approximation_threshold = s.approximation_threshold;
			brush_radius = s.brush_radius;
			brush_count = s.brush_count;
			brush_ratio = s.brush_ratio;
			min_length = s.min_length;
			max_length = s.max_length;
			opacity = s.opacity;
			return *this;
		}

		bool operator==(PainterManipulatorSettings s) {
			return 	(blurring_factor == s.blurring_factor) &&
					(approximation_threshold == s.approximation_threshold) &&
					(brush_radius == s.brush_radius) &&
					(brush_count == s.brush_count) &&
					(brush_ratio == s.brush_ratio) &&
					(min_length == s.min_length) &&
					(max_length == s.max_length) &&
					(opacity == s.opacity);
		}
		
		bool operator!=(PainterManipulatorSettings s) {
			return !(*this == s);
		}


		float	blurring_factor;
		int32	approximation_threshold;	

		float	brush_radius;
		int32	brush_count;
		float	brush_ratio;

		int32	min_length;
		int32	max_length;

		float	opacity;
};


class PainterManipulatorView;

struct stroke {
	BShape *shape;
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	BPoint point;
	
	stroke() { shape = NULL; }
	~stroke() { delete shape; }
};

class PainterManipulator : public WindowGUIManipulator {
			BBitmap	*preview_bitmap;
			BBitmap	*preview_source;

			PainterManipulatorSettings	settings;
			PainterManipulatorSettings	previous_settings;

			ImageProcessingLibrary		*ipLibrary;
			
			PainterManipulatorView		*config_view;
			
		
			PainterManipulatorSettings	current_settings;
			Selection	*current_selection;				
			BBitmap		*source_bitmap;
			BBitmap		*target_bitmap;
			BStatusBar	*progress_bar;

			int32		preview_width;
			int32		preview_height;
			BRect		preview_area;

status_t	paint(BBitmap *source,BBitmap *target);
status_t	paint_stroke_layer(BBitmap *target,BBitmap *reference_image,int32 brush_size);
status_t	paint_first_stroke_layer(BBitmap *target,BBitmap *reference_image,int32 brush_size);
stroke*		make_stroke(int32 x,int32 y,uint32 *r_bits,BitmapAnalyzer *r_analyzer,uint32 *t_bits,int32 bpr,int32 height,float R);
stroke*		make_stroke(int32 x,int32 y,uint32 *r_bits,BitmapAnalyzer *r_analyzer,int32 bpr,int32 height,float R);


public:
			PainterManipulator(BBitmap*);
			~PainterManipulator();
			
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


status_t	ReadSettings(BNode *node);
status_t	WriteSettings(BNode *node);
};



#define	STROKE_LENGTH_CHANGED	'Srln'
#define	BLUR_FACTOR_CHANGED		'Blfc'
#define	ERROR_AMOUNT_CHANGED	'Eram'
#define	OPACITY_CHANGED			'Opch'

#define	BRUSH_COUNT_CHANGED		'Bcch'
#define	BRUSH_RATIO_CHANGED		'Brrc'
#define	BRUSH_SIZE_CHANGED		'Brsc'


class FloatTextControl;

class PainterManipulatorView : public WindowGUIManipulatorView {
		BMessenger						target;
		PainterManipulator				*manipulator;
		PainterManipulatorSettings		settings;	

		PreviewView						*preview;	

		RangeSlider						*slider1;
		BSlider							*slider2;
		BSlider							*slider3;
		BSlider							*slider4;		

		FloatTextControl				*count_control;
		FloatTextControl				*ratio_control;
		FloatTextControl				*size_control;
		
public:
		PainterManipulatorView(PainterManipulator*,BMessenger*,int32,int32);

void	AllAttached();
void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ChangeSettings(const PainterManipulatorSettings&);

PreviewView*	ReturnPreviewView() { return preview; }
};


BRect	fit_rectangle(BRect,BPoint,int32,int32);

inline float color_distance(int32 c1,int32 c2)
{
	float dist = 0;
	
	dist += ((c1 & 0xFF) - (c2 & 0xFF));
	dist += (((c1>>8) & 0xFF) - ((c2>>8) & 0xFF));
	dist += (((c1>>16) & 0xFF) - ((c2>>16) & 0xFF));
	dist += (((c1>>24) & 0xFF) - ((c2>>24) & 0xFF));
		
	return dist;
}

#endif



