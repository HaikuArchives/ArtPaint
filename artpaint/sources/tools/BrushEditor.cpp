
/*

	Filename:	BrushEditor.cpp
	Contents:	BrushEditor-class definitions
	Author:		Heikki Suhonen

*/

#include <Bitmap.h>
#include <Button.h>
#include <math.h>
#include <RadioButton.h>

#include "BrushEditor.h"
#include "UtilityClasses.h"
#include "Controls.h"
#include "StringServer.h"
#include "HSPolygon.h"
#include "BrushStoreWindow.h"


#define PI M_PI

BrushEditor* BrushEditor::the_editor = NULL;


BrushEditor::BrushEditor(BRect frame,Brush *brush)
	: 	BBox(frame)
{
	the_brush = brush;
	b_info = the_brush->GetInfo();

	float max_divider = 0;

	float height = BRUSH_PREVIEW_HEIGHT;
	float width = BRUSH_PREVIEW_WIDTH;
	ResizeTo(frame.Width(),height+2*EXTRA_EDGE);
	BMessage *a_message;
	brush_view = new BrushView(BRect(EXTRA_EDGE,EXTRA_EDGE,width-EXTRA_EDGE,height-EXTRA_EDGE),the_brush);
	AddChild(brush_view);
	BRect slider_frame = BRect(brush_view->Frame().RightTop()+BPoint(EXTRA_EDGE,0),Bounds().RightTop()+BPoint(-EXTRA_EDGE,20+EXTRA_EDGE));


	a_message = new BMessage(BRUSH_SHAPE_CHANGED);
	a_message->AddInt32("shape",HS_RECTANGULAR_BRUSH);
	rectangle_button = new BRadioButton(slider_frame,"rectangle button",StringServer::ReturnString(RECTANGLE_STRING),a_message);
	AddChild(rectangle_button);
	rectangle_button->ResizeToPreferred();
	slider_frame = rectangle_button->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height());

	a_message = new BMessage(BRUSH_SHAPE_CHANGED);
	a_message->AddInt32("shape",HS_ELLIPTICAL_BRUSH);
	ellipse_button = new BRadioButton(slider_frame,"ellipse button",StringServer::ReturnString(ELLIPSE_STRING),a_message);
	AddChild(ellipse_button);

	slider_frame.OffsetBy(0,slider_frame.Height());
	store_button = new BButton(slider_frame,"store_button",StringServer::ReturnString(STORE_BRUSH_STRING),new BMessage(BRUSH_STORING_REQUESTED));
	store_button->ResizeTo(store_button->Frame().Width(),brush_view->Frame().bottom-store_button->Frame().top);
	AddChild(store_button);

	slider_frame = BRect(brush_view->Frame().LeftBottom()+BPoint(0,EXTRA_EDGE),Bounds().RightBottom()-BPoint(EXTRA_EDGE,0));
	a_message = new BMessage(BRUSH_WIDTH_CHANGED);
	a_message->AddInt32("value", int32(b_info.width));
	width_slider = new ControlSliderBox(slider_frame,"brush width",StringServer::ReturnString(WIDTH_STRING),"0",a_message,1,100);
	AddChild(width_slider);
	slider_frame = width_slider->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+EXTRA_EDGE);
	max_divider = max_c(max_divider,width_slider->Divider());

	a_message = new BMessage(BRUSH_HEIGHT_CHANGED);
	a_message->AddInt32("value", int32(b_info.height));
	height_slider = new ControlSliderBox(slider_frame,"brush height",StringServer::ReturnString(HEIGHT_STRING),"0",a_message,1,100);
	AddChild(height_slider);
	slider_frame = height_slider->Frame();
	slider_frame.OffsetBy(0,slider_frame.Height()+EXTRA_EDGE);
	max_divider = max_c(max_divider,height_slider->Divider());


	a_message = new BMessage(BRUSH_EDGE_CHANGED);
	a_message->AddInt32("value", int32(b_info.fade_length));
	fade_slider = new ControlSliderBox(slider_frame,"brush edge",StringServer::ReturnString(FADE_STRING),"0",a_message,0,100);
	AddChild(fade_slider);
	max_divider = max_c(max_divider,fade_slider->Divider());

	ResizeTo(Frame().Width(),fade_slider->Frame().bottom+EXTRA_EDGE);

	height_slider->SetDivider(max_divider);
	width_slider->SetDivider(max_divider);
	fade_slider->SetDivider(max_divider);
}


BrushEditor::~BrushEditor()
{
	the_editor = NULL;
}


BView* BrushEditor::CreateBrushEditor(BRect rect,Brush *brush)
{
	if (the_editor != NULL)
		return NULL;

	else {
		the_editor = new BrushEditor(rect,brush);
		return the_editor;
	}
}

void BrushEditor::BrushModified()
{
	if (the_editor != NULL) {
		BWindow *window = the_editor->Window();
		if (window != NULL)
			window->Lock();

		the_editor->brush_view->BrushModified();
		if (window != NULL) {
			window->PostMessage(BRUSH_ALTERED,the_editor);
		}

		if (window != NULL)
			window->Unlock();
	}
}

void BrushEditor::AttachedToWindow()
{
	BMessenger *a_messenger = new BMessenger(this);
	width_slider->SetTarget(a_messenger);

	a_messenger = new BMessenger(this);
	height_slider->SetTarget(a_messenger);

	a_messenger = new BMessenger(this);
	fade_slider->SetTarget(a_messenger);

	rectangle_button->SetTarget(this);
	ellipse_button->SetTarget(this);
	if (b_info.shape == HS_RECTANGULAR_BRUSH) {
		rectangle_button->SetValue(B_CONTROL_ON);
	}
	else if (b_info.shape == HS_ELLIPTICAL_BRUSH) {
		ellipse_button->SetValue(B_CONTROL_ON);
	}

	store_button->SetTarget(this);
}

void BrushEditor::MessageReceived(BMessage *message)
{
	int32 value;
	bool final;

	switch (message->what) {
		case BRUSH_WIDTH_CHANGED:
			if (message->FindInt32("value",&value) == B_OK) {
				b_info.width = value;
				the_brush->ModifyBrush(b_info);
				brush_view->BrushModified();
				if (message->FindBool("final",&final) == B_OK) {
					if (final == TRUE)
						the_brush->CreateDiffBrushes();
				}
			}
			break;
		case BRUSH_HEIGHT_CHANGED:
			if (message->FindInt32("value",&value) == B_OK) {
				b_info.height = value;
				the_brush->ModifyBrush(b_info);
				brush_view->BrushModified();
				if (message->FindBool("final",&final) == B_OK) {
					if (final == TRUE)
						the_brush->CreateDiffBrushes();
				}
			}
			break;
		case BRUSH_EDGE_CHANGED:
			if (message->FindInt32("value",&value) == B_OK) {
				b_info.fade_length = value;
				the_brush->ModifyBrush(b_info);
				brush_view->BrushModified();
				if (message->FindBool("final",&final) == B_OK) {
					if (final == TRUE)
						the_brush->CreateDiffBrushes();
				}
			}
			break;
		case BRUSH_SHAPE_CHANGED:
			if (message->FindInt32("shape",&value) == B_OK) {
				b_info.shape = value;
				the_brush->ModifyBrush(b_info);
				the_brush->CreateDiffBrushes();
				brush_view->BrushModified();
			}
			break;
		case BRUSH_ALTERED:
			// Here something has altered the brush and we should reflect it
			// in our controls and such things.
			b_info = the_brush->GetInfo();
			width_slider->setValue(int32(b_info.width));
			height_slider->setValue(int32(b_info.height));
			fade_slider->setValue(int32(b_info.fade_length));
			brush_view->BrushModified();
			// Also set the correct radio-button
			if (b_info.shape == HS_RECTANGULAR_BRUSH) {
				rectangle_button->SetValue(true);
			}
			else if (b_info.shape == HS_ELLIPTICAL_BRUSH) {
				ellipse_button->SetValue(true);
			}
			break;
		case BRUSH_STORING_REQUESTED:
			BrushStoreWindow::AddBrush(the_brush);
			break;
		default:
			BBox::MessageReceived(message);
			break;
	}
}




BrushView::BrushView(BRect frame,Brush *brush)
	:	BView(frame,"Brush View",B_FOLLOW_TOP|B_FOLLOW_LEFT,B_WILL_DRAW)
{
	the_brush = brush;
	float preview_width = frame.Width()-2;
	float preview_height = frame.Height()-2;
	brush_preview = new BBitmap(BRect(0,0,preview_width-1,preview_height-1),B_RGB_32_BIT);
	draw_controls = FALSE;
	the_brush->PreviewBrush(brush_preview);
}


BrushView::~BrushView()
{

}


void BrushView::Draw(BRect)
{
	DrawBitmap(brush_preview,BPoint(1,1));
	SetPenSize(1);
	SetHighColor(0,0,0,255);
	StrokeLine(BPoint(0,0),Bounds().LeftBottom());
	StrokeLine(BPoint(0,0),Bounds().RightTop());
//	SetHighColor(255,255,255,255);
	StrokeLine(Bounds().RightTop(),Bounds().RightBottom());
	StrokeLine(Bounds().LeftBottom(),Bounds().RightBottom());

	if (draw_controls == TRUE) {
		float r1 = Bounds().Width()/2;
		float r2 = Bounds().Height()/2;
		BPoint point_list[12];
		point_list[0] = BPoint(0,r2-2);
		point_list[1] = BPoint(0,0);
		point_list[2] = BPoint(-r1*.5,0);
		point_list[3] = BPoint(0,0);
		point_list[4] = BPoint(0,-r2+7);
		point_list[5] = BPoint(-2,-r2+7);
		point_list[6] = BPoint(0,-r2+2);
		point_list[7] = BPoint(2,-r2+7);
		point_list[8] = BPoint(0,-r2+7);
		point_list[9] = BPoint(0,0);
		point_list[10] = BPoint(r1*.5,0);
		point_list[11] = BPoint(0,0);

		HSPolygon *poly = new HSPolygon(point_list, 12);
		poly->Rotate(BPoint(0, 0), the_brush->GetInfo().angle);
		poly->TranslateBy(int32(r1 - 1), int32(r2 - 1));
		BPolygon *bpoly = poly->GetBPolygon();
		SetHighColor(150, 0, 0, 255);
		StrokePolygon(bpoly, false);
		FillPolygon(bpoly);
		delete poly;
		delete bpoly;
	}
}


void BrushView::MessageReceived(BMessage *message)
{
	brush_info *info;
	int32 size;
	switch (message->what) {
		case HS_BRUSH_DRAGGED:
			message->FindData("brush data",B_ANY_TYPE,(const void**)&info,&size);
			if (size == sizeof(brush_info)) {
				the_brush->ModifyBrush(*info);
				the_brush->PreviewBrush(brush_preview);
				Invalidate();
				the_brush->CreateDiffBrushes();
				if ((Parent() != NULL) && (Window() != NULL)) {
					Window()->PostMessage(BRUSH_ALTERED,Parent());
				}
			}
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void BrushView::MouseDown(BPoint point)
{
	BPoint c;
	c.x = Bounds().Width()/2;
	c.y = Bounds().Height()/2;
	brush_info info = the_brush->GetInfo();
	uint32 buttons;

	GetMouse(&point,&buttons);
	float angle = info.angle;
	float prev_angle;
	if (point.x == c.x)
		angle = 0;
	else if (point.y == c.y)
		angle = 90;
	else {
		angle = atan2(point.x-c.x,c.y-point.y)*180/PI;
	}
	prev_angle = angle;
	if (TRUE/*buttons == B_PRIMARY_MOUSE_BUTTON*/) {
		while (buttons) {
			if (angle != prev_angle) {
				the_brush->ModifyBrush(info);
				the_brush->PreviewBrush(brush_preview);
				Window()->Lock();
				Draw(Bounds());
				Window()->Unlock();
				prev_angle = angle;
			}
			Window()->Lock();
			GetMouse(&point,&buttons);
			Window()->Unlock();
			if (point.x == c.x)
				angle = 0;
			else if (point.y == c.y)
				angle = 90;
			else {
				angle = atan2(point.x-c.x,c.y-point.y)*180/PI;
			}
			info.angle += angle - prev_angle;
			snooze(20 *1000);
		}

		the_brush->CreateDiffBrushes();
		Window()->PostMessage(BRUSH_ALTERED,Parent());
	}
//	else {
//		info = the_brush->GetInfo();
//		BMessage *a_message = new BMessage(HS_BRUSH_DRAGGED);
//		a_message->AddData("brush data",B_ANY_TYPE,&info,sizeof(brush_info));
//		DragMessage(a_message,BRect(0,0,BRUSH_PREVIEW_WIDTH-1,BRUSH_PREVIEW_HEIGHT-1));
//		delete a_message;
//	}
}

void BrushView::MouseMoved(BPoint,uint32 transit,const BMessage*)
{
	if (Window() != NULL) {
		Window()->Lock();
		if (transit == B_ENTERED_VIEW) {
			draw_controls = TRUE;
			Draw(Bounds());
		}
		if (transit == B_EXITED_VIEW) {
			draw_controls = FALSE;
			Draw(Bounds());
		}
		Window()->Unlock();
	}
}


void BrushView::BrushModified()
{
	the_brush->PreviewBrush(brush_preview);
	Draw(Bounds());
}
