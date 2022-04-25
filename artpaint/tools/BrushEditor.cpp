/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "BrushEditor.h"

#include "BrushStoreWindow.h"
#include "HSPolygon.h"
#include "FloatSliderControl.h"
#include "NumberSliderControl.h"
#include "UtilityClasses.h"


#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>


#include <math.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


enum {
	kExtraEdge				= 4,
	kBrushAltered			= 'kbal',
	kBrushSizeChanged		= 'kbsz',
	kBrushRatioChanged		= 'kbrc',
	kBrushFadeChanged		= 'kbfc',
	kBrushShapeChanged		= 'kbsc',
	kBrushStoreRequest		= 'kbsr',
	kBrushResetRequest		= 'kbrr',
	kBrushAngleChanged 		= 'kbac',
};


BrushEditor* BrushEditor::fBrushEditor = NULL;
using ArtPaint::Interface::NumberSliderControl;


BrushEditor::BrushEditor(Brush* brush)
	: BBox(B_FANCY_BORDER, NULL)
	, fBrush(brush)
	, fBrushInfo(fBrush->GetInfo())
{
	float height = BRUSH_PREVIEW_HEIGHT - kExtraEdge;
	float width = BRUSH_PREVIEW_WIDTH - kExtraEdge;

	fBrushView = new BrushView(BRect(kExtraEdge, kExtraEdge, width, height),
		fBrush);

	BMessage* message = new BMessage(kBrushShapeChanged);
	message->AddInt32("shape", HS_RECTANGULAR_BRUSH);

	fRectangle = new BRadioButton(B_TRANSLATE("Rectangle"),
		message);

	message = new BMessage(kBrushShapeChanged);
	message->AddInt32("shape", HS_ELLIPTICAL_BRUSH);

	fEllipse = new BRadioButton(B_TRANSLATE("Ellipse"),
		message);

	fStoreBrush = new BButton(B_TRANSLATE("Store brush"),
		new BMessage(kBrushStoreRequest));

	fResetBrush = new BButton(B_TRANSLATE("Reset brush"),
		new BMessage(kBrushResetRequest));

	message = new BMessage(kBrushSizeChanged);
	message->AddInt32("value", int32(fBrushInfo.width));

	fBrushSize =
		new NumberSliderControl(B_TRANSLATE("Size:"), "0",
		message, 1, 500, false);

	message = new BMessage(kBrushRatioChanged);
	message->AddFloat("value", float(fBrushInfo.width/fBrushInfo.height));

	fBrushRatio =
		new FloatSliderControl(B_TRANSLATE("Ratio:"), "0",
		message, 1.0, 20.0, false);

	message = new BMessage(kBrushAngleChanged);
	message->AddInt32("value", int32(fBrushInfo.angle));

	fBrushAngle =
		new NumberSliderControl(B_TRANSLATE("Angle:"), "0",
		message, -179, 180, false);

	message = new BMessage(kBrushFadeChanged);
	message->AddInt32("value", int32(fBrushInfo.fade_length));

	fBrushFade =
		new NumberSliderControl(B_TRANSLATE("Fade:"), "0",
		message, 0, 100, false);

	BSeparatorView* view = new BSeparatorView(B_HORIZONTAL, B_FANCY_BORDER);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BGridLayout* gridLayout = BGridLayoutBuilder(5.0, 5.0)
		.Add(fBrushSize, 0, 0, 0, 0)
		.Add(fBrushSize->LabelLayoutItem(), 0, 0)
		.Add(fBrushSize->TextViewLayoutItem(), 1, 0)
		.Add(fBrushSize->Slider(), 2, 0)
		.Add(fBrushRatio, 0, 1, 0, 0)
		.Add(fBrushRatio->LabelLayoutItem(), 0, 1)
		.Add(fBrushRatio->TextViewLayoutItem(), 1, 1)
		.Add(fBrushRatio->Slider(), 2, 1)
		.Add(fBrushAngle, 0, 2, 0, 0)
		.Add(fBrushAngle->LabelLayoutItem(), 0, 2)
		.Add(fBrushAngle->TextViewLayoutItem(), 1, 2)
		.Add(fBrushAngle->Slider(), 2, 2)
		.Add(fBrushFade, 0, 3, 0, 0)
		.Add(fBrushFade->LabelLayoutItem(), 0, 3)
		.Add(fBrushFade->TextViewLayoutItem(), 1, 3)
		.Add(fBrushFade->Slider(), 2, 3);
	gridLayout->SetMaxColumnWidth(1, StringWidth("1000"));
	gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.AddGroup(B_HORIZONTAL, 5.0)
			.AddStrut(5.0)
			.Add(fBrushView)
			.AddGroup(B_VERTICAL, 5.0)
				.Add(fEllipse)
				.Add(fRectangle)
				.AddGlue()
			.End()
			.AddGlue()
		.End()
		.AddStrut(1.0)
		.Add(view)
		.AddStrut(1.0)
		.Add(gridLayout->View())
		.AddStrut(1.0)
		.AddGroup(B_HORIZONTAL)
			.Add(fResetBrush)
			.AddGlue()
			.Add(fStoreBrush)
		.End()
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
			window->PostMessage(kBrushAltered, fBrushEditor);

		if (window && locked)
			window->Unlock();
	}
}


void
BrushEditor::AttachedToWindow()
{
	fBrushFade->SetTarget(this);
	fBrushSize->SetTarget(this);
	fBrushRatio->SetTarget(this);
	fBrushAngle->SetTarget(this);

	fEllipse->SetTarget(this);
	fRectangle->SetTarget(this);
	fStoreBrush->SetTarget(this);
	fResetBrush->SetTarget(this);

	if (fBrushInfo.shape == HS_RECTANGULAR_BRUSH)
		fRectangle->SetValue(B_CONTROL_ON);

	if (fBrushInfo.shape == HS_ELLIPTICAL_BRUSH)
		fEllipse->SetValue(B_CONTROL_ON);
}


void
BrushEditor::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kBrushSizeChanged: {
			int32 value;
			if (message->FindInt32("value", &value) == B_OK) {
				fBrushInfo.width = (int32)(value * fBrushRatio->Value());
				fBrushInfo.height = value;

				fBrush->ModifyBrush(fBrushInfo);
				fBrushView->BrushModified();

				bool final;
				if (message->FindBool("final", &final) == B_OK && final)
					fBrush->CreateDiffBrushes();
			}
		}	break;

		case kBrushRatioChanged: {
			float value;
			if (message->FindFloat("value", &value) == B_OK) {
				fBrushInfo.width = (int32)(value * fBrushSize->Value());
				fBrushInfo.height = fBrushSize->Value();

				fBrush->ModifyBrush(fBrushInfo);
				fBrushView->BrushModified();

				bool final;
				if (message->FindBool("final", &final) == B_OK && final)
					fBrush->CreateDiffBrushes();
			}
		}	break;

		case kBrushAngleChanged: {
			int32 value;
			if (message->FindInt32("value", &value) == B_OK) {
				fBrushInfo.angle = value;
				fBrush->ModifyBrush(fBrushInfo);
				fBrushView->BrushModified();

				bool final;
				if (message->FindBool("final", &final) == B_OK && final)
					fBrush->CreateDiffBrushes();
			}
		}	break;

		case kBrushFadeChanged: {
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

		case kBrushShapeChanged: {
			int32 value;
			if (message->FindInt32("shape", &value) == B_OK) {
				fBrushInfo.shape = value;
				fBrush->ModifyBrush(fBrushInfo);
				fBrush->CreateDiffBrushes();
				fBrushView->BrushModified();
			}
		}	break;

		case kBrushAltered: {
			// Here something has altered the brush and we should reflect it
			// in our controls and such things.
			fBrushInfo = fBrush->GetInfo();
			float ratio = (float)(fBrushInfo.width / fBrushInfo.height);
			fBrushSize->SetValue(int32(fBrushInfo.width));
			if (ratio < 1.0) {
				ratio = 1.0 / ratio;
				fBrushSize->SetValue(int32(fBrushInfo.height));
			}

			fBrushRatio->SetValue(float(ratio));

			fBrushAngle->SetValue(int32(fBrushInfo.angle));
			fBrushFade->SetValue(int32(fBrushInfo.fade_length));
			fBrushView->BrushModified();

			if (fBrushInfo.shape == HS_RECTANGULAR_BRUSH)
				fRectangle->SetValue(B_CONTROL_ON);

			if (fBrushInfo.shape == HS_ELLIPTICAL_BRUSH)
				fEllipse->SetValue(B_CONTROL_ON);
		}	break;

		case kBrushStoreRequest: {
			BrushStoreWindow::AddBrush(fBrush);
		}	break;

		case kBrushResetRequest: {
			fBrushSize->SetValue(30);
			fBrushRatio->SetValue(1.0);
			fBrushAngle->SetValue(0);
			fBrushFade->SetValue(2);
			fRectangle->SetValue(B_CONTROL_OFF);
			fEllipse->SetValue(B_CONTROL_ON);

			fBrushInfo.shape = HS_ELLIPTICAL_BRUSH;
			fBrushInfo.width = 30;
			fBrushInfo.height = 30;
			fBrushInfo.angle = 0;
			fBrushInfo.fade_length = 2;
			fBrush->ModifyBrush(fBrushInfo);
			fBrush->CreateDiffBrushes();

			fBrushView->BrushModified();
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
BrushView::MessageReceived(BMessage* message)
{
//	switch (message->what) {
//		case HS_BRUSH_DRAGGED: {
//			int32 size;
//			brush_info* info;
//			message->FindData("brush data",B_ANY_TYPE,(const void**)&info,&size);
//			if (size == sizeof(brush_info)) {
//				fBrush->ModifyBrush(*info);
//				fBrush->PreviewBrush(fBrushPreview);
//				Invalidate();
//				fBrush->CreateDiffBrushes();
//				if (Window() && Parent())
//					Window()->PostMessage(kBrushAltered, Parent());
//			}
//			break;
//
//		default: {
			BView::MessageReceived(message);
//		}	break;
//	}
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
		angle = atan2(point.x-c.x,c.y-point.y) * 180 / M_PI;
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
			info.angle = angle;
			snooze(20000);
		}

		fBrush->CreateDiffBrushes();
		Window()->PostMessage(kBrushAltered, Parent());
	} /* else {
			info = fBrush->GetInfo();
			BMessage* message = new BMessage(HS_BRUSH_DRAGGED);
			message->AddData("brush data",B_ANY_TYPE,&info,sizeof(brush_info));
			DragMessage(message,BRect(0,0,fBrushPreview_WIDTH-1,fBrushPreview_HEIGHT-1));
			delete message;
	} */
}


void
BrushView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	if (Window() && Window()->Lock()) {
		switch (transit) {
			default:	break;
			case B_ENTERED_VIEW: {
				fDrawControls = true;
				Draw(Bounds());
			}	break;

			case B_EXITED_VIEW: {
				fDrawControls = false;
				Draw(Bounds());
			}	break;
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
