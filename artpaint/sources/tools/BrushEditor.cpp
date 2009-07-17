/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "BrushEditor.h"

#include "UtilityClasses.h"
#include "Controls.h"
#include "StringServer.h"
#include "HSPolygon.h"
#include "BrushStoreWindow.h"


#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>


#include <math.h>


#define	BRUSH_WIDTH_CHANGED		'BWCh'
#define BRUSH_HEIGHT_CHANGED	'BHCh'
#define BRUSH_EDGE_CHANGED		'BeCh'
#define BRUSH_SHAPE_CHANGED		'Bshc'
#define	BRUSH_STORING_REQUESTED	'Bsrq'


BrushEditor* BrushEditor::fBrushEditor = NULL;


BrushEditor::BrushEditor(Brush* brush)
	: BBox(B_FANCY_BORDER, NULL)
	, fBrush(brush)
{
	fBrushInfo = fBrush->GetInfo();

	float height = BRUSH_PREVIEW_HEIGHT - EXTRA_EDGE;
	float width = BRUSH_PREVIEW_WIDTH - EXTRA_EDGE;

	fBrushView = new BrushView(BRect(EXTRA_EDGE, EXTRA_EDGE, width, height),
		fBrush);

	BMessage* message = new BMessage(BRUSH_SHAPE_CHANGED);
	message->AddInt32("shape", HS_RECTANGULAR_BRUSH);

	fRectangle = new BRadioButton(StringServer::ReturnString(RECTANGLE_STRING),
		message);

	message = new BMessage(BRUSH_SHAPE_CHANGED);
	message->AddInt32("shape", HS_ELLIPTICAL_BRUSH);

	fEllipse = new BRadioButton(StringServer::ReturnString(ELLIPSE_STRING),
		message);

	fStoreBrush = new BButton(StringServer::ReturnString(STORE_BRUSH_STRING),
		new BMessage(BRUSH_STORING_REQUESTED));

	message = new BMessage(BRUSH_WIDTH_CHANGED);
	message->AddInt32("value", int32(fBrushInfo.width));

	fWidthSlider = new ControlSliderBox("brush width",
		StringServer::ReturnString(WIDTH_STRING), "0", message, 0, 100);

	message = new BMessage(BRUSH_HEIGHT_CHANGED);
	message->AddInt32("value", int32(fBrushInfo.height));

	fHeightSlider = new ControlSliderBox("brush height",
		StringServer::ReturnString(HEIGHT_STRING), "0", message, 0, 100);

	message = new BMessage(BRUSH_EDGE_CHANGED);
	message->AddInt32("value", int32(fBrushInfo.fade_length));

	fFadeSlider = new ControlSliderBox("brush edge",
		StringServer::ReturnString(FADE_STRING), "0", message, 0, 100);

	SetLayout(new BGroupLayout(B_VERTICAL));

	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.AddGroup(B_HORIZONTAL, 5.0)
			.Add(fBrushView)
			.AddGroup(B_VERTICAL, 5.0)
				.Add(fRectangle)
				.Add(fEllipse)
				.AddGlue()
			.End()
			.AddGlue()
		.End()
		.Add(fWidthSlider)
		.Add(fHeightSlider)
		.Add(fFadeSlider)
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(fStoreBrush)
		.End()
		.SetInsets(5.0, 5.0, 5.0, 5.0)
	);
}


BrushEditor::~BrushEditor()
{
	fBrushEditor = NULL;
}


BView*
BrushEditor::CreateBrushEditor(Brush* brush)
{
	if (fBrushEditor)
		return NULL;

	fBrushEditor = new BrushEditor(brush);
	return fBrushEditor;
}


void
BrushEditor::BrushModified()
{
	if (fBrushEditor) {
		BWindow* window = fBrushEditor->Window();

		bool locked = false;
		if (window && window->Lock())
			locked = true;

		fBrushEditor->fBrushView->BrushModified();
		if (window)
			window->PostMessage(BRUSH_ALTERED,fBrushEditor);

		if (window && locked)
			window->Unlock();
	}
}


void
BrushEditor::AttachedToWindow()
{
	fWidthSlider->SetTarget(new BMessenger(this));
	fHeightSlider->SetTarget(new BMessenger(this));
	fFadeSlider->SetTarget(new BMessenger(this));

	fEllipse->SetTarget(this);
	fRectangle->SetTarget(this);
	fStoreBrush->SetTarget(this);

	if (fBrushInfo.shape == HS_RECTANGULAR_BRUSH)
		fRectangle->SetValue(B_CONTROL_ON);

	if (fBrushInfo.shape == HS_ELLIPTICAL_BRUSH)
		fEllipse->SetValue(B_CONTROL_ON);
}


void
BrushEditor::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case BRUSH_WIDTH_CHANGED: {
			int32 value;
			if (message->FindInt32("value", &value) == B_OK) {
				fBrushInfo.width = value;
				fBrush->ModifyBrush(fBrushInfo);
				fBrushView->BrushModified();

				bool final;
				if (message->FindBool("final", &final) == B_OK && final)
					fBrush->CreateDiffBrushes();
			}
		}	break;

		case BRUSH_HEIGHT_CHANGED: {
			int32 value;
			if (message->FindInt32("value", &value) == B_OK) {
				fBrushInfo.height = value;
				fBrush->ModifyBrush(fBrushInfo);
				fBrushView->BrushModified();

				bool final;
				if (message->FindBool("final", &final) == B_OK && final)
					fBrush->CreateDiffBrushes();
			}
		}	break;

		case BRUSH_EDGE_CHANGED: {
			int32 value;
			if (message->FindInt32("value",&value) == B_OK) {
				fBrushInfo.fade_length = value;
				fBrush->ModifyBrush(fBrushInfo);
				fBrushView->BrushModified();

				bool final;
				if (message->FindBool("final", &final) == B_OK && final)
					fBrush->CreateDiffBrushes();
			}
		}	break;

		case BRUSH_SHAPE_CHANGED: {
			int32 value;
			if (message->FindInt32("shape", &value) == B_OK) {
				fBrushInfo.shape = value;
				fBrush->ModifyBrush(fBrushInfo);
				fBrush->CreateDiffBrushes();
				fBrushView->BrushModified();
			}
		}	break;

		case BRUSH_ALTERED: {
			// Here something has altered the brush and we should reflect it
			// in our controls and such things.
			fBrushInfo = fBrush->GetInfo();
			fWidthSlider->setValue(int32(fBrushInfo.width));
			fHeightSlider->setValue(int32(fBrushInfo.height));
			fFadeSlider->setValue(int32(fBrushInfo.fade_length));
			fBrushView->BrushModified();

			if (fBrushInfo.shape == HS_RECTANGULAR_BRUSH)
				fRectangle->SetValue(B_CONTROL_ON);

			if (fBrushInfo.shape == HS_ELLIPTICAL_BRUSH)
				fEllipse->SetValue(B_CONTROL_ON);
		}	break;

		case BRUSH_STORING_REQUESTED: {
			BrushStoreWindow::AddBrush(fBrush);
		}	break;

		default: {
			BView::MessageReceived(message);
		}	break;
	}
}


// #pragma mark -- BrushView


BrushView::BrushView(BRect frame, Brush* brush)
	: BView(frame, "brush view", B_FOLLOW_NONE, B_WILL_DRAW)
	, fDrawControls(false)
	, fBrush(brush)
	, fBrushPreview(NULL)
{
	SetExplicitMinSize(BSize(frame.Width(), frame.Height()));
	SetExplicitMaxSize(BSize(frame.Width(), frame.Height()));

	frame.InsetBy(1.0, 1.0);
	fBrushPreview = new BBitmap(BRect(0.0, 0.0, frame.Width() - 1.0,
		frame.Height() - 1.0), B_RGBA32);
	fBrush->PreviewBrush(fBrushPreview);
}


BrushView::~BrushView()
{
	delete fBrushPreview;
}


void
BrushView::Draw(BRect)
{
	DrawBitmap(fBrushPreview, BPoint(1.0, 1.0));

	SetPenSize(1);
	SetHighColor(0, 0, 0, 255);
	StrokeRect(Bounds());

	if (fDrawControls) {
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

		HSPolygon poly(point_list, 12);
		poly.Rotate(BPoint(0, 0), fBrush->GetInfo().angle);
		poly.TranslateBy(int32(r1 - 1), int32(r2 - 1));

		BPolygon* bpoly = poly.GetBPolygon();

		SetHighColor(150, 0, 0, 255);
		StrokePolygon(bpoly, false);
		FillPolygon(bpoly);

		delete bpoly;
	}
}


void
BrushView::MessageReceived(BMessage *message)
{
	brush_info *info;
	int32 size;
	switch (message->what) {
		case HS_BRUSH_DRAGGED:
			message->FindData("brush data",B_ANY_TYPE,(const void**)&info,&size);
			if (size == sizeof(brush_info)) {
				fBrush->ModifyBrush(*info);
				fBrush->PreviewBrush(fBrushPreview);
				Invalidate();
				fBrush->CreateDiffBrushes();
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


void
BrushView::MouseDown(BPoint point)
{
	BPoint c;
	c.x = Bounds().Width() / 2.0;
	c.y = Bounds().Height() / 2.0;
	brush_info info = fBrush->GetInfo();
	uint32 buttons;

	GetMouse(&point,&buttons);
	float angle = info.angle;
	float prev_angle;
	if (point.x == c.x)
		angle = 0;
	else if (point.y == c.y)
		angle = 90;
	else {
		angle = atan2(point.x-c.x,c.y-point.y)*180/M_PI;
	}
	prev_angle = angle;

	if (true /*buttons == B_PRIMARY_MOUSE_BUTTON*/) {
		while (buttons) {
			if (angle != prev_angle) {
				fBrush->ModifyBrush(info);
				fBrush->PreviewBrush(fBrushPreview);
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
				angle = atan2(point.x-c.x,c.y-point.y)*180/M_PI;
			}
			info.angle += angle - prev_angle;
			snooze(20 *1000);
		}

		fBrush->CreateDiffBrushes();
		Window()->PostMessage(BRUSH_ALTERED, Parent());
	} /* else {
		info = fBrush->GetInfo();
		BMessage *message = new BMessage(HS_BRUSH_DRAGGED);
		message->AddData("brush data",B_ANY_TYPE,&info,sizeof(brush_info));
		DragMessage(message,BRect(0,0,fBrushPreview_WIDTH-1,fBrushPreview_HEIGHT-1));
		delete message;
	} */
}


void
BrushView::MouseMoved(BPoint,uint32 transit,const BMessage*)
{
	if (Window() && Window()->Lock()) {
		if (transit == B_ENTERED_VIEW) {
			fDrawControls = true;
			Draw(Bounds());
		}

		if (transit == B_EXITED_VIEW) {
			fDrawControls = false;
			Draw(Bounds());
		}

		Window()->Unlock();
	}
}


void
BrushView::BrushModified()
{
	fBrush->PreviewBrush(fBrushPreview);
	Draw(Bounds());
}
