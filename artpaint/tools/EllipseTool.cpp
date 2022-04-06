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

#include "EllipseTool.h"

#include "BitmapDrawer.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "PaintApplication.h"
#include "Selection.h"
#include "StringServer.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


EllipseTool::EllipseTool()
	: DrawingTool(StringServer::ReturnString(ELLIPSE_TOOL_NAME_STRING),
		ELLIPSE_TOOL)
{
	fOptions = FILL_ENABLED_OPTION | SIZE_OPTION | SHAPE_OPTION;
	fOptionsCount = 3;

	SetOption(FILL_ENABLED_OPTION,B_CONTROL_OFF);
	SetOption(SIZE_OPTION,1);
	SetOption(SHAPE_OPTION,HS_CENTER_TO_CORNER);
}


EllipseTool::~EllipseTool()
{
}


ToolScript*
EllipseTool::UseTool(ImageView *view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow *window = view->Window();
	drawing_mode old_mode;
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript* the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));
	Selection *selection = view->GetSelection();

	if (window != NULL) {
		BPoint original_point;
		BRect bitmap_rect,old_rect,new_rect;
		window->Lock();
		old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		window->Unlock();
		rgb_color c=((PaintApplication*)be_app)->Color(true);
		original_point = point;
		bitmap_rect = BRect(point,point);
		old_rect = new_rect = view->convertBitmapRectToView(bitmap_rect);
		window->Lock();
		view->StrokeEllipse(new_rect);
		window->Unlock();

		while (buttons) {
			window->Lock();
			if (old_rect != new_rect) {
				view->Draw(old_rect);
				view->StrokeElxlipse(new_rect);
				old_rect = new_rect;
			}
			view->getCoords(&point,&buttons);
			window->Unlock();
			bitmap_rect = MakeRectFromPoints(original_point, point);
			if (modifiers() & B_LEFT_CONTROL_KEY) {
				// Make the rectangle square.
				float max_distance = max_c(bitmap_rect.Height(),bitmap_rect.Width());
				if (original_point.x == bitmap_rect.left)
					bitmap_rect.right = bitmap_rect.left + max_distance;
				else
					bitmap_rect.left = bitmap_rect.right - max_distance;

				if (original_point.y == bitmap_rect.top)
					bitmap_rect.bottom = bitmap_rect.top + max_distance;
				else
					bitmap_rect.top = bitmap_rect.bottom - max_distance;
			}
			if (GetCurrentValue(SHAPE_OPTION) == HS_CENTER_TO_CORNER) {
				// Make the the rectangle original corner be at the center of
				// new rectangle.
				float y_distance = bitmap_rect.Height();
				float x_distance = bitmap_rect.Width();

				if (bitmap_rect.left == original_point.x)
					bitmap_rect.left = bitmap_rect.left - x_distance;
				else
					bitmap_rect.right = bitmap_rect.right + x_distance;

				if (bitmap_rect.top == original_point.y)
					bitmap_rect.top = bitmap_rect.top - y_distance;
				else
					bitmap_rect.bottom = bitmap_rect.bottom + y_distance;

			}
			new_rect = view->convertBitmapRectToView(bitmap_rect);
			snooze(20 * 1000);
		}

		BitmapDrawer *drawer = new BitmapDrawer(bitmap);
		bool use_fill = (GetCurrentValue(FILL_ENABLED_OPTION) == B_CONTROL_ON);
		bool use_anti_aliasing = (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_ON);
		drawer->DrawEllipse(bitmap_rect,RGBColorToBGRA(c), use_fill,
			use_anti_aliasing, selection);
		delete drawer;

		the_script->AddPoint(bitmap_rect.LeftTop());
		the_script->AddPoint(bitmap_rect.RightTop());
		the_script->AddPoint(bitmap_rect.RightBottom());
		the_script->AddPoint(bitmap_rect.LeftBottom());

		bitmap_rect.InsetBy(-1,-1);
		window->Lock();
		view->SetDrawingMode(old_mode);
		view->UpdateImage(bitmap_rect);
		view->Sync();
		window->Unlock();
		SetLastUpdatedRect(bitmap_rect);
	}

	return the_script;
}


int32
EllipseTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_OK;
}


BView*
EllipseTool::ConfigView()
{
	return new EllipseToolConfigView(this);
}


const void*
EllipseTool::ToolCursor() const
{
	return HS_ELLIPSE_CURSOR;
}


const char*
EllipseTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString(isInUse ? ELLIPSE_TOOL_IN_USE_STRING
		: ELLIPSE_TOOL_READY_STRING);
}


// #pragma mark -- EllipseToolConfigView


EllipseToolConfigView::EllipseToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option",FILL_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);
		fFillEllipse =
			new BCheckBox(StringServer::ReturnString(FILL_ELLIPSE_STRING),
				message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", HS_CORNER_TO_CORNER);
		fCorner2Corner =
			new BRadioButton(StringServer::ReturnString(CORNER_TO_CORNER_STRING),
				message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", HS_CENTER_TO_CORNER);
		fCenter2Corner =
			new BRadioButton(StringServer::ReturnString(CENTER_TO_CORNER_STRING),
				message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ANTI_ALIASING_LEVEL_OPTION);
		message->AddInt32("value", 0x00000000);

		fAntiAlias =
			new BCheckBox(StringServer::ReturnString(ENABLE_ANTI_ALIASING_STRING),
			message);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fFillEllipse)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(StringServer::ReturnString(MODE_STRING)))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fCorner2Corner)
				.Add(fCenter2Corner)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(StringServer::ReturnString(OPTIONS_STRING)))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fAntiAlias)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(FILL_ENABLED_OPTION) != B_CONTROL_OFF)
			fFillEllipse->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CORNER_TO_CORNER)
			fCorner2Corner->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CENTER_TO_CORNER)
			fCenter2Corner->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF)
			fAntiAlias->SetValue(B_CONTROL_ON);
	}
}


void
EllipseToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fFillEllipse->SetTarget(this);
	fCorner2Corner->SetTarget(this);
	fCenter2Corner->SetTarget(this);
	fAntiAlias->SetTarget(this);
}
