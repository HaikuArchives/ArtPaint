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
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


EllipseTool::EllipseTool()
	:
	DrawingTool(B_TRANSLATE("Ellipse tool"), "p", ELLIPSE_TOOL)
{

	fOptions = FILL_ENABLED_OPTION | SIZE_OPTION | SHAPE_OPTION | ANTI_ALIASING_LEVEL_OPTION
	               | ROTATION_ENABLED_OPTION;
	fOptionsCount = 5;

	SetOption(FILL_ENABLED_OPTION, B_CONTROL_OFF);
	SetOption(SIZE_OPTION, 1);
	SetOption(SHAPE_OPTION, HS_CENTER_TO_CORNER);
	SetOption(ANTI_ALIASING_LEVEL_OPTION, B_CONTROL_OFF);
	SetOption(ROTATION_ENABLED_OPTION, B_CONTROL_OFF);
}


ToolScript*
EllipseTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	drawing_mode old_mode;
	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript* the_script
		= new ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));
	Selection* selection = view->GetSelection();

	if (window != NULL) {
		BPoint original_point, view_point;
		BRect bitmap_rect, old_rect, new_rect;
		window->Lock();
		rgb_color old_color = view->HighColor();
		old_mode = view->DrawingMode();
		view->SetDrawingMode(B_OP_INVERT);
		window->Unlock();
		bool use_fg_color = true;
		if (buttons == B_SECONDARY_MOUSE_BUTTON)
			use_fg_color = false;

		rgb_color c = ((PaintApplication*)be_app)->Color(use_fg_color);
		original_point = point;
		bitmap_rect = BRect(point, point);
		old_rect = new_rect = view->convertBitmapRectToView(bitmap_rect);
		window->Lock();
		view->SetHighColor(c);
		view->StrokeEllipse(new_rect, B_SOLID_HIGH);
		view->SetHighColor(old_color);
		window->Unlock();

		while (buttons) {
			window->Lock();
			if (old_rect != new_rect) {
				view->Draw(old_rect);
				view->SetHighColor(c);
				view->StrokeEllipse(new_rect);
				view->SetHighColor(old_color);
				old_rect = new_rect;
			}
			view->getCoords(&point, &buttons);
			window->Unlock();
			bitmap_rect = MakeRectFromPoints(original_point, point);
			if (modifiers() & B_SHIFT_KEY) {
				// Make the ellipse circular.
				float max_distance = max_c(bitmap_rect.Height(), bitmap_rect.Width());
				if (original_point.x == bitmap_rect.left)
					bitmap_rect.right = bitmap_rect.left + max_distance;
				else
					bitmap_rect.left = bitmap_rect.right - max_distance;

				if (original_point.y == bitmap_rect.top)
					bitmap_rect.bottom = bitmap_rect.top + max_distance;
				else
					bitmap_rect.top = bitmap_rect.bottom - max_distance;
			}
			if (modifiers() & B_COMMAND_KEY) {
				// Make the the ellipse's origin at the center of
				// new ellipse.
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

		float new_angle, prev_angle;
        if (GetCurrentValue(ROTATION_ENABLED_OPTION) == B_CONTROL_ON) {
			BPoint centroid = new_rect.LeftTop() + new_rect.RightTop()
				+ new_rect.RightBottom() + new_rect.LeftBottom();
			centroid.x /= 4;
			centroid.y /= 4;
			prev_angle = new_angle = 0;
			bool continue_rotating = true;
			float original_angle;
			if (centroid.x != point.x) {
				original_angle = atan((centroid.y - point.y)
					/ (centroid.x - point.x)) * 180 /M_PI;
			} else {
				original_angle = 90;
			}

			while (continue_rotating) {
				if (is_clicks_data_valid) {
					continue_rotating = FALSE;
					is_clicks_data_valid = FALSE;
				}
				else if (is_keys_data_valid) {
					if (last_key_event_bytes[0] == B_ESCAPE) {
						continue_rotating = FALSE;
						//draw_rectangle = FALSE;
					}
					is_keys_data_valid = FALSE;
				}
				else {
					// Here we should rotate the polygon
					window->Lock();
					if (new_angle != prev_angle) {
						BRect eraseRect(0, 0, 1, 1);
						float max_dim = ceil(max_c(new_rect.Width(), new_rect.Height()) / 2.);

						eraseRect.top = centroid.y - max_dim;
						eraseRect.bottom = centroid.y + max_dim;
						eraseRect.left = centroid.x - max_dim;
						eraseRect.right = centroid.x + max_dim;
						view->Draw(eraseRect);
						view->PushState();
						view->SetOrigin(centroid);
						float newAngleRad = new_angle * M_PI / 180.;
						view->RotateBy(newAngleRad);
						view->SetHighColor(c);
						view->StrokeEllipse(new_rect.OffsetByCopy(-centroid.x, -centroid.y));
						view->SetHighColor(old_color);
						view->PopState();
						prev_angle = new_angle;
					}
					view->getCoords(&point, &buttons, &view_point);
					window->Unlock();
					if (centroid.x != point.x) {
						new_angle = atan((centroid.y - view_point.y)
							/ (centroid.x - view_point.x)) * 180 / M_PI;
					} else {
						new_angle = 90;
					}
					new_angle -= original_angle;

					if (modifiers() & B_SHIFT_KEY)
						new_angle = SnapToAngle(22.5, new_angle);

					snooze(20 * 1000);
    			}
			}
		} else {
			new_angle = 0.;
		}

		BitmapDrawer *drawer = new BitmapDrawer(bitmap);
		bool use_fill = (GetCurrentValue(FILL_ENABLED_OPTION) == B_CONTROL_ON);
		bool use_anti_aliasing = (GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) == B_CONTROL_ON);
		drawer->DrawEllipse(bitmap_rect, RGBColorToBGRA(c), use_fill,
			use_anti_aliasing, selection, new_angle);
		delete drawer;

		the_script->AddPoint(bitmap_rect.LeftTop());
		the_script->AddPoint(bitmap_rect.RightTop());
		the_script->AddPoint(bitmap_rect.RightBottom());
		the_script->AddPoint(bitmap_rect.LeftBottom());

        float max_dim = ceil(max_c(bitmap_rect.Width(), bitmap_rect.Height()) / 2.);
		BPoint centroid = bitmap_rect.LeftTop() + bitmap_rect.RightTop()
		  + bitmap_rect.RightBottom() + bitmap_rect.LeftBottom();
		centroid.x /= 4;
		centroid.y /= 4;

		BRect updatedRect(0, 0, 1, 1);

		updatedRect.top = centroid.y - max_dim;
		updatedRect.left = centroid.x - max_dim;
		updatedRect.bottom = centroid.y + max_dim;
		updatedRect.right = centroid.x + max_dim;

		updatedRect.InsetBy(-10,-10);
		window->Lock();
		view->SetHighColor(old_color);
		view->SetDrawingMode(old_mode);
		view->UpdateImage(updatedRect);
		view->Sync();
		window->Unlock();
		SetLastUpdatedRect(updatedRect);
	}

	return the_script;
}


int32
EllipseTool::UseToolWithScript(ToolScript*, BBitmap*)
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
	return (isInUse ? B_TRANSLATE("Drawing an ellipse.")
					: B_TRANSLATE("Ellipse: SHIFT for circle, ALT for centered"));
}


// #pragma mark -- EllipseToolConfigView


EllipseToolConfigView::EllipseToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", FILL_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);
		fFillEllipse = new BCheckBox(B_TRANSLATE("Fill ellipse"), message);

        message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ROTATION_ENABLED_OPTION);
		message->AddInt32("value", 0x00000000);

		fRotation =
			new BCheckBox(B_TRANSLATE("Enable rotation"),
			message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", ANTI_ALIASING_LEVEL_OPTION);
		message->AddInt32("value", 0x00000000);

		fAntiAlias = new BCheckBox(B_TRANSLATE("Enable antialiasing"), message);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fFillEllipse)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddStrut(kWidgetSpacing)
			.Add(SeparatorView(B_TRANSLATE("Options")))
			.AddGroup(B_VERTICAL, kWidgetSpacing)
			    .Add(fRotation)
				.Add(fAntiAlias)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(FILL_ENABLED_OPTION) != B_CONTROL_OFF)
			fFillEllipse->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(ROTATION_ENABLED_OPTION) != B_CONTROL_OFF)
			fRotation->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(ANTI_ALIASING_LEVEL_OPTION) != B_CONTROL_OFF)
			fAntiAlias->SetValue(B_CONTROL_ON);
	}
}


void
EllipseToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fFillEllipse->SetTarget(this);
	fRotation->SetTarget(this);
	fAntiAlias->SetTarget(this);
}
