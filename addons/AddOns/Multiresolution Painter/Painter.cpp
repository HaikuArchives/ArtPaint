/* 

	Filename:	Painter.cpp
	Contents:	Definitions for painter add-on.	
	Author:		Heikki Suhonen
	
*/

#include <algo.h>
#include <Node.h>
#include <StatusBar.h>
#include <StopWatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector.h>

#include "AddOns.h"
#include "FloatTextControl.h"
#include "ImageProcessingLibrary.h"
#include "LayoutMatrix.h"
#include "Painter.h"
#include "PixelOperations.h"

extern "C" __declspec(dllexport) char name[255] = "Painter…";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Starts add-on that paints the image with brush-strokes.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new PainterManipulator(bm);	
}



PainterManipulator::PainterManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	preview_width = 90;
	preview_height = 90;
	preview_area = BRect(0,0,-1,-1);
	progress_bar = NULL;	
		
	ipLibrary = new ImageProcessingLibrary();
		
	SetPreviewBitmap(bm);
}


PainterManipulator::~PainterManipulator()
{
	delete config_view;
	delete ipLibrary;
}


BBitmap* PainterManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)	
{
//	BStopWatch watch("painter");
	PainterManipulatorSettings *new_settings = dynamic_cast<PainterManipulatorSettings*>(set);
	
	if (new_settings == NULL)
		return NULL;
		
	if (original == NULL)
		return NULL;
		
	source_bitmap = original; 
	target_bitmap = new BBitmap(original->Bounds(),B_RGB32,true); 
	BView *view = new BView(target_bitmap->Bounds(),"view",B_FOLLOW_NONE,B_WILL_DRAW);

	target_bitmap->Lock();
	target_bitmap->AddChild(view);
	target_bitmap->Unlock();
	
	current_selection = selection;
	current_settings = *new_settings;
	progress_bar = status_bar;
	
	paint(source_bitmap,target_bitmap);
		
	// copy the bits from target_bitmap to source_bitmap
	uint32 *source = (uint32*)target_bitmap->Bits();
	uint32 *target = (uint32*)source_bitmap->Bits();
	uint32 bits_length = source_bitmap->BitsLength();
		
	memcpy(target,source,bits_length);		
	
	delete target_bitmap;

	return original;
}

int32 PainterManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	// The preview will be done only for a part of the image and at full resolution
	current_settings = settings;
	if (full_quality) {
		BBitmap *target = config_view->ReturnPreviewView()->ReturnBitmap();
		paint(preview_source,target);
		config_view->ReturnPreviewView()->RedisplayBitmap();			
	}

	return DRAW_NOTHING;
}


void PainterManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// Here we select the proper preview area.
	BRect new_area = fit_rectangle(preview_bitmap->Bounds(),point,preview_width,preview_height);
	BBitmap *target = config_view->ReturnPreviewView()->ReturnBitmap();
	
	if (new_area != preview_area) {
		preview_area = new_area;
		BRect target_area = new_area;
		target_area.OffsetTo(0,0);
		target->Lock();
		target->ChildAt(0)->DrawBitmap(preview_bitmap,new_area,target_area);
		target->Unlock();
		config_view->ReturnPreviewView()->RedisplayBitmap();
		
		preview_source->Lock();
		preview_source->ChildAt(0)->DrawBitmap(preview_bitmap,new_area,target_area);
		preview_source->Unlock();
	}
}


void PainterManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		if (bm != NULL) {
			preview_bitmap = bm;
		}
		else {
			preview_bitmap = NULL;
		}

		if (preview_area.IsValid() == false) {
			preview_area = fit_rectangle(preview_bitmap->Bounds(),BPoint(0,0),preview_width,preview_height);
		}
	}
}


void PainterManipulator::Reset(Selection*)
{
}

BView* PainterManipulator::MakeConfigurationView(BMessenger *target)
{
	if (config_view == NULL) {
		config_view = new PainterManipulatorView(this,target,preview_width,preview_height);
		config_view->ChangeSettings(settings);
	}

	BRect rect = config_view->ReturnPreviewView()->ReturnBitmap()->Bounds();
	rect.OffsetTo(0,0);
	preview_source = new BBitmap(rect,B_RGB32,true);
	preview_source->Lock();
	preview_source->AddChild(new BView(rect,"preview_source_view",B_FOLLOW_NONE,B_WILL_DRAW));
	rect = preview_area;
	rect.OffsetTo(BPoint(0,0));
	preview_source->ChildAt(0)->DrawBitmap(preview_bitmap,preview_area,rect);
	preview_source->Unlock();
	
	return config_view;
}


ManipulatorSettings* PainterManipulator::ReturnSettings()
{
	return new PainterManipulatorSettings(settings);
}

void PainterManipulator::ChangeSettings(ManipulatorSettings *s)
{
	PainterManipulatorSettings *new_settings;
	new_settings = dynamic_cast<PainterManipulatorSettings*>(s);

	if (new_settings != NULL) {
		settings = *new_settings;
	}
}


status_t PainterManipulator::ReadSettings(BNode *node)
{
	PainterManipulatorSettings s;
	
	if (node->ReadAttr("settings",0,0,&s,sizeof(PainterManipulatorSettings)) == sizeof(PainterManipulatorSettings)) {
		settings = s;
		return B_NO_ERROR;
	}

	return B_ERROR;
}

status_t PainterManipulator::WriteSettings(BNode *node)
{
	node->WriteAttr("settings",0,0,&settings,sizeof(PainterManipulatorSettings));

	return B_NO_ERROR;
}


char* PainterManipulator::ReturnName()
{
	return "Painter";
}

char* PainterManipulator::ReturnHelpString()
{
	return "Use the controls to adjust the appeareance of the strokes. Click on image to set the preview area.";
}


// Here start the functions that do the actual painting of the image
status_t PainterManipulator::paint(BBitmap *source,BBitmap *target)
{
	BBitmap *reference_image = new BBitmap(source->Bounds(),B_RGB32,false);

	float blur_factor = current_settings.blurring_factor;

	target->Lock();
	BView *view = target->ChildAt(0);
	if (view != NULL) {
		rgb_color c = { 255,255,255,0 };
		view->SetHighColor(c);
		view->FillRect(view->Bounds());
		view->Sync();
		target->Unlock();
	}
	else {
		target->Unlock();
		return B_ERROR;
	}
	
	float brush_radius = current_settings.brush_radius;
	int32 brush_size = 10000;
		
	for (int32 i=current_settings.brush_count;i>0;i--) {
		if (brush_radius < brush_size) {
			brush_size = brush_radius;
			printf("brush_size: %d\n",brush_size);
		
			uint32 *source_bits = (uint32*)source->Bits();
			uint32 *reference_bits = (uint32*)reference_image->Bits();
			uint32 bits_length = source->BitsLength();
			memcpy(reference_bits,source_bits,bits_length);		
		
			ipLibrary->gaussian_blur(reference_image,blur_factor*brush_size);
			if (i == current_settings.brush_count)
				paint_first_stroke_layer(target,reference_image,brush_size);						
			else			
				paint_stroke_layer(target,reference_image,brush_size);						

		}
		
		brush_radius *= current_settings.brush_ratio;

		if (brush_radius < 1.0)
			break;
	}

	delete reference_image;
	
	return B_OK;	
}
 

status_t PainterManipulator::paint_stroke_layer(BBitmap *target,BBitmap *reference_image,int32 brush_size)
{	
	
	// Assumed is that reference_image and target are of same size
	uint32 *t_bits = (uint32*)target->Bits();
	uint32 *r_bits = (uint32*)reference_image->Bits();
	
	int32 bpr = target->BytesPerRow()/4;
	int32 height = target->Bounds().IntegerHeight()+1;
	
	// Create the difference map.
	int16 *difference_map = new int16[bpr*height];
	for (int32 y=0;y<height;y++) {
		for (int32 x=0;x<bpr;x++) {
			difference_map[x + y*bpr] = abs((int16)color_distance(t_bits[x+y*bpr],r_bits[x+y*bpr]));
		}
	}	

	int32 grid = brush_size;
	int32 half_grid = grid/2;
	int32 T = current_settings.approximation_threshold;
		
	vector<stroke*> stroke_array;

	BitmapAnalyzer *analyzer = new BitmapAnalyzer(reference_image);
	float R = brush_size;	


	vector<int32> point_vector;
	for (int32 y=0;y<height;y+=grid) {
		for (int32 x=0;x<bpr;x+=grid) {
			point_vector.push_back(y*bpr+x);
		}
	}	
	random_shuffle(point_vector.begin(),point_vector.end());

	target->Lock();
	BView *view = target->ChildAt(0);
	view->PushState();
	if (current_settings.opacity < 1.0) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_COMPOSITE);
	}
	else
		view->SetDrawingMode(B_OP_COPY);

	view->SetPenSize(2*brush_size+1);
	view->SetLineMode(B_ROUND_CAP,B_ROUND_JOIN);

	for (int32 i=0;i<point_vector.size();i++) {
		int32 x = point_vector[i] % bpr;
		int32 y = point_vector[i] / bpr;
					
		int32 left = max_c(0,x-half_grid);
		int32 top = max_c(0,y-half_grid);
		int32 right = min_c(bpr,x+half_grid);
		int32 bottom = min_c(height,y+half_grid);
		
		int32 area_error = 0;
		for (int32 dy=top;dy<bottom;dy++) {
			for (int32 dx=left;dx<right;dx++) {
				area_error += difference_map[dx + dy*bpr];
			}
		}				
		area_error = area_error / (grid*grid);
		
		if (area_error > T) {
			int16 max_error = 0;
			int32 stroke_x,stroke_y;
			stroke_x = left;
			stroke_y = top;
			for (int32 dy=top;dy<bottom;dy++) {
				for (int32 dx=left;dx<right;dx++) {
					int16 error = difference_map[dx + dy*bpr];
					if (error >= max_error) {
						max_error = error;
						stroke_x = dx;
						stroke_y = dy;
					}
				}
			}					

			stroke *s = make_stroke(stroke_x,stroke_y,r_bits,analyzer,t_bits,bpr,height,R);							
			if (s != NULL) {		
				s->color.bytes[3] *= current_settings.opacity;
				view->SetHighColor(s->color.bytes[2],s->color.bytes[1],s->color.bytes[0],s->color.bytes[3]);
				if (s->shape == NULL) {
					view->FillEllipse(s->point,brush_size,brush_size);			
				}
				else {
					view->StrokeShape(s->shape);
				}
				delete s;
			}
		}
		if ((progress_bar != NULL) && (i % 100 == 0)) {
			float addition = 100.0 / point_vector.size() * 100.0 / current_settings.brush_count;
			if (progress_bar->LockLooper()) {
				progress_bar->Update(addition);
				progress_bar->UnlockLooper();
			}
		}
	}

	view->Sync();
	view->PopState();
	target->Unlock();

	delete analyzer;

	delete[] difference_map;
	
	return B_OK;
}

status_t PainterManipulator::paint_first_stroke_layer(BBitmap *target,BBitmap *reference_image,int32 brush_size)
{	
	
	// Assumed is that reference_image and target are of same size
	uint32 *t_bits = (uint32*)target->Bits();
	uint32 *r_bits = (uint32*)reference_image->Bits();
	
	int32 bpr = target->BytesPerRow()/4;
	int32 height = target->Bounds().IntegerHeight()+1;
	
	int32 grid = brush_size;
	int32 half_grid = grid/2;

	vector<stroke*> stroke_array;
	
	BitmapAnalyzer *analyzer = new BitmapAnalyzer(reference_image);
	float R = brush_size;


	vector<int32> point_vector;
	for (int32 y=0;y<height;y+=grid) {
		for (int32 x=0;x<bpr;x+=grid) {
			point_vector.push_back(y*bpr+x);
		}
	}	
	random_shuffle(point_vector.begin(),point_vector.end());

	target->Lock();
	BView *view = target->ChildAt(0);
	view->PushState();
	if (current_settings.opacity < 1.0) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA,B_ALPHA_COMPOSITE);
	}
	else
		view->SetDrawingMode(B_OP_COPY);

	view->SetPenSize(2*brush_size+1);
	view->SetLineMode(B_ROUND_CAP,B_ROUND_JOIN);

	for (int32 i=0;i<point_vector.size();i++) {
		int32 x = point_vector[i] % bpr;
		int32 y = point_vector[i] / bpr;
		stroke *s = make_stroke(x,y,r_bits,analyzer,bpr,height,R);							
		if (s != NULL) {		
			s->color.bytes[3] *= current_settings.opacity;
			view->SetHighColor(s->color.bytes[2],s->color.bytes[1],s->color.bytes[0],s->color.bytes[3]);
			if (s->shape == NULL) {
				view->FillEllipse(s->point,brush_size,brush_size);			
			}
			else {
				view->StrokeShape(s->shape);
			}
			delete s;
		}
		if ((progress_bar != NULL) && (i % 100 == 0)) {
			float addition = 100.0 / point_vector.size() * 100.0 / current_settings.brush_count;
			if (progress_bar->LockLooper()) {
				progress_bar->Update(addition);
				progress_bar->UnlockLooper();
			}
		}
	}

	view->Sync();
	view->PopState();
	target->Unlock();

	delete analyzer;
	
	return B_OK;
}

stroke* PainterManipulator::make_stroke(int32 x,int32 y,uint32 *r_bits,BitmapAnalyzer *r_analyzer,uint32 *t_bits,int32 bpr,int32 height,float R)
{
	stroke *s = new stroke;
	s->color.word = r_bits[x + y*bpr];

	if (r_analyzer->GradientMagnitude(x,y) == 0) {
		s->point.x = x;
		s->point.y = y;
		return s;
	}

	s->shape = new BShape();
	s->shape->MoveTo(BPoint(x,y));
	s->shape->LineTo(BPoint(x,y));	
	int32 max_length = current_settings.max_length;
	int32 min_length = current_settings.min_length;
	
	BPoint last_direction(0,0);
	
	for (int32 i=1;i<=max_length;i++) {
		// check that 0<=x<=bpr and 0<=y<=height
		if (x<0 || x>bpr || y<0 || y>height) {
			break;		
		}
				
		// differs more from the stroke than from the previous result
		if (abs(color_distance(*(r_bits+x+y*bpr),*(t_bits+x+y*bpr))) < abs(color_distance(*(r_bits+x+y*bpr),s->color.word))) {
			break;
		}
		
		// vanishing gradient
		if (r_analyzer->GradientMagnitude(x,y) == 0)
			break;

		BPoint grad_dir = r_analyzer->GradientDirection(x,y);
		BPoint grad_normal(-grad_dir.y,grad_dir.x);
		
		// minimize curvature
		if (last_direction.x*grad_normal.x + last_direction.y*grad_normal.y < 0) {
			grad_normal = BPoint(-grad_normal.x,-grad_normal.y);
		}
		
		// Calculate the new point
		x = x + round(grad_normal.x*R);
		y = y + round(grad_normal.y*R);

		// Add the point to shape
		if (s->shape->LineTo(BPoint(x,y)) != B_OK) {
			printf("Error adding a line\n");
			break;
		}
				
		// change the previous direction
		last_direction = grad_normal;
	}
	
	s->shape->Close();	// close the stroke
	
	return s;
}



stroke* PainterManipulator::make_stroke(int32 x,int32 y,uint32 *r_bits,BitmapAnalyzer *r_analyzer,int32 bpr,int32 height,float R)
{
	stroke *s = new stroke;
	s->color.word = r_bits[x + y*bpr];

	if (r_analyzer->GradientMagnitude(x,y) == 0) {
		s->point.x = x;
		s->point.y = y;
		return s;
	}
	
	s->shape = new BShape();
	s->shape->MoveTo(BPoint(x,y));
	s->shape->LineTo(BPoint(x,y));	
	
	int32 max_length = current_settings.max_length;
	int32 min_length = current_settings.min_length;
	
	BPoint last_direction(0,0);
	
	for (int32 i=1;i<=max_length;i++) {
		// check that 0<=x<=bpr and 0<=y<=height
		if (x<0 || x>bpr || y<0 || y>height) {
			break;		
		}
				

		// vanishing gradient
		if (r_analyzer->GradientMagnitude(x,y) == 0)
			break;


		BPoint grad_dir = r_analyzer->GradientDirection(x,y);
		BPoint grad_normal(-grad_dir.y,grad_dir.x);
		
		// minimize curvature
		if (last_direction.x*grad_normal.x + last_direction.y*grad_normal.y < 0) {
			grad_normal = BPoint(-grad_normal.x,-grad_normal.y);
		}
		
		// Calculate the new point
		x = x + round(grad_normal.x*R);
		y = y + round(grad_normal.y*R);

		// Add the point to shape
		if (s->shape->LineTo(BPoint(x,y)) != B_OK) {
			printf("Error adding a line\n");
			break;
		}
				
		// change the previous direction
		last_direction = grad_normal;
	}
	
	s->shape->Close();	// Close the stroke
	
	return s;
}


// -------------------------------------
PainterManipulatorView::PainterManipulatorView(PainterManipulator *manip,BMessenger *t,int32 w,int32 h)
	: WindowGUIManipulatorView(BRect(0,0,0,0))
{
	target = BMessenger(*t);
	manipulator = manip;

	preview = new PreviewView(BRect(4,4,104,104),w,h);
	AddChild(preview);

	slider1 = new RangeSlider(BRect(108,4,108,4),"slider1","Stroke Length",new BMessage(STROKE_LENGTH_CHANGED),0,25);
	slider1->SetLimitLabels("Short","Long");
	slider1->ResizeToPreferred();
	slider1->ResizeTo(200,slider1->Frame().Height());
//	slider1->SetModificationMessage(new BMessage(STROKE_LENGTH_CHANGED));
	AddChild(slider1);
	rgb_color c;
	c.red = 50;
	c.green = 50;
	c.blue = 200;
	c.alpha = 255;
	slider1->UseFillColor(true,&c);	





	BRect slider_rect = slider1->Frame();
	slider_rect.OffsetBy(0,slider_rect.Height()+4);


	LayoutMatrix *matrix = new LayoutMatrix(slider_rect);
	AddChild(matrix);
	
	count_control = new FloatTextControl(BRect(0,0,0,0),"count","Brush Count",2,
											new BMessage(BRUSH_COUNT_CHANGED),1,9,0);

	ratio_control = new FloatTextControl(BRect(0,0,0,0),"ratio","Brush Ratio",0.5,
											new BMessage(BRUSH_RATIO_CHANGED),0,1.0,2);

	size_control = new FloatTextControl(BRect(0,0,0,0),"size","Brush Size",2,
											new BMessage(BRUSH_SIZE_CHANGED),1,20,0);
	
	matrix->AddView(count_control,0,0);
	matrix->AddView(ratio_control,1,0);
	matrix->AddView(size_control,2,0);
	matrix->DoLayout();

	slider_rect.OffsetBy(0,matrix->Frame().Height());

	slider2 = new BSlider(slider_rect,"slider2","Noise",new BMessage(BLUR_FACTOR_CHANGED),1,40,B_TRIANGLE_THUMB);
	AddChild(slider2);
	
	slider_rect.OffsetBy(0,slider_rect.Height()+4);
	slider3 = new BSlider(slider_rect,"slider3","Accuracy",new BMessage(ERROR_AMOUNT_CHANGED),1,300,B_TRIANGLE_THUMB);
	slider3->SetLimitLabels("Low","High");
	AddChild(slider3);

	slider_rect.OffsetBy(0,slider_rect.Height()+4);
	slider4 = new BSlider(slider_rect,"slider4","Opacity",new BMessage(OPACITY_CHANGED),0,255,B_TRIANGLE_THUMB);
	slider4->SetLimitLabels("Transparent","Opaque");
	AddChild(slider4);
	

	BRect rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = slider1->Frame().right + 4;
	rc.bottom = max_c(preview->Frame().bottom + 4,slider4->Frame().bottom + 4);
	
	ResizeTo(rc.Width(),rc.Height());
}


void PainterManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
}

void PainterManipulatorView::AllAttached()
{
	slider1->SetTarget(this);
	slider2->SetTarget(this);
	slider3->SetTarget(this);
	slider4->SetTarget(this);
	
	count_control->SetTarget(this);
	ratio_control->SetTarget(this);
	size_control->SetTarget(this);
	
	ChangeSettings(settings);
}


void PainterManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BRUSH_COUNT_CHANGED:
			settings.brush_count = count_control->Value();
			manipulator->ChangeSettings(&settings);			
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case BRUSH_RATIO_CHANGED:
			settings.brush_ratio = ratio_control->Value();
			manipulator->ChangeSettings(&settings);			
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case BRUSH_SIZE_CHANGED:
			settings.brush_radius = size_control->Value();
			manipulator->ChangeSettings(&settings);			
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case STROKE_LENGTH_CHANGED:
			settings.min_length = slider1->LowerValue();
			settings.max_length = slider1->HigherValue();			
			manipulator->ChangeSettings(&settings);			
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;
			
		case BLUR_FACTOR_CHANGED:
			settings.blurring_factor = 0.1 + slider2->Value()*0.1;
			manipulator->ChangeSettings(&settings);			
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;

		case ERROR_AMOUNT_CHANGED:
			settings.approximation_threshold = 300-slider3->Value();
			manipulator->ChangeSettings(&settings);			
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;			

		case OPACITY_CHANGED:
			settings.opacity = slider4->Value()/255.0;
			manipulator->ChangeSettings(&settings);
			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;
			
		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void PainterManipulatorView::ChangeSettings(const PainterManipulatorSettings& set)
{
	settings = set;

	if (LockLooper()) {
		slider1->SetLowerValue(set.min_length);
		slider1->SetHigherValue(set.max_length);
		slider2->SetValue((set.blurring_factor-0.1)/0.1);
		slider3->SetValue(300-set.approximation_threshold);
		slider4->SetValue(255*set.opacity);

		count_control->SetValue(set.brush_count);
		ratio_control->SetValue(set.brush_ratio);
		size_control->SetValue(set.brush_radius);

		UnlockLooper();
	}
}



BRect fit_rectangle(BRect bound,BPoint c,int32 w,int32 h)
{
	// Assumed is that bound has offset at 0,0
	BRect rect;
	rect.left = rect.top = 0;
	rect.right = min_c(w-1,bound.IntegerWidth());
	rect.bottom = min_c(h-1,bound.IntegerHeight());

	rect.OffsetBy(min_c(bound.right+1-w,max_c(0,c.x-w/2)),0);
	rect.OffsetBy(0,min_c(bound.bottom+1-h,max_c(0,c.y-h/2)));
	
	return rect;
}


