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

#include "BlurTool.h"

#include "BitmapDrawer.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Selection.h"
#include "StringServer.h"
#include "ToolScript.h"


#include <CheckBox.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <SeparatorView.h>
#include <Window.h>


using ArtPaint::Interface::NumberSliderControl;


BlurTool::BlurTool()
	: DrawingTool(StringServer::ReturnString(BLUR_TOOL_NAME_STRING), BLUR_TOOL)
{
	fOptions = SIZE_OPTION | CONTINUITY_OPTION;
	fOptionsCount = 2;

	SetOption(SIZE_OPTION,1);
	SetOption(CONTINUITY_OPTION,B_CONTROL_OFF);
}


BlurTool::~BlurTool()
{
}


ToolScript*
BlurTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	/*
		This function uses a convolution matrix to do the blurring.
		The matrix is following:

				1/9		1/9		1/9

				1/9		1/9		1/9

				1/9		1/9		1/9
	*/
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BPoint prev_point;
	BWindow* window = view->Window();
	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer* drawer = new BitmapDrawer(bitmap);

	ToolScript* the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	selection = view->GetSelection();

	BRect bounds = bitmap->Bounds();
	uint32* bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow()/4;

	// this is the bitmap where the blurred image will be first made
	BBitmap* blurred = new BBitmap(BRect(0, 0, fToolSettings.size+1, fToolSettings.size+1),
		B_RGB_32_BIT);
	int32* blurred_bits;
	int32 blurred_bpr = blurred->BytesPerRow()/4;
	int32 previous_size = fToolSettings.size;

	float half_size;
	BRect rc;
	// for the quick calculation of square-roots
	int32 sqrt_table[5500];
	for (int32 i=0;i<5500;i++)
		sqrt_table[i] = (int32)sqrt(i);

	half_size = fToolSettings.size/2;
	prev_point = point - BPoint(1,1);
	SetLastUpdatedRect(BRect(point, point));
	while (buttons) {
		if ((fToolSettings.continuity == B_CONTROL_ON)
			|| (fToolSettings.size != previous_size) || (point != prev_point)) {
			if (fToolSettings.size != previous_size) {
				delete blurred;
				half_size = fToolSettings.size/2;
				blurred = new BBitmap(BRect(0,0,fToolSettings.size+1,fToolSettings.size+1),
					B_RGB_32_BIT);
				previous_size = fToolSettings.size;
			}

			blurred_bits = (int32*)blurred->Bits();

			rc = BRect(point.x - half_size, point.y - half_size,
				point.x + half_size, point.y + half_size);
			rc = rc & bounds;

			BPoint left_top = rc.LeftTop();
			uint32 new_pixel;
			float red,green,blue,alpha;
			int32 x_dist,y_sqr;
			for (int32 y=0;y<rc.Height()+1;y++) {
				y_sqr = (int32)(point.y - rc.top - y);
				y_sqr *= y_sqr;
				for (int32 x=0;x<rc.Width()+1;x++) {
					x_dist = (int32)(point.x-rc.left-x);
					if ((sqrt_table[x_dist*x_dist + y_sqr] <= half_size) &&
						(selection->ContainsPoint(left_top+BPoint(x,y))) ) {
						red=0;green=0;blue=0;alpha=0;
						for (int32 dy=-1;dy<2;dy++) {
							for (int32 dx=-1;dx<2;dx++) {
								int32 x_coord =
									(int32)min_c(max_c(left_top.x+x+dx,0), bounds.right);
								int32 y_coord =
									(int32)min_c(max_c(left_top.y+y+dy,0),bounds.bottom);
								new_pixel = drawer->GetPixel(x_coord,y_coord);

								blue += (float)((new_pixel>>24)&0xFF)/9.0;
								green += (float)((new_pixel>>16)&0xFF)/9.0;
								red += (float)((new_pixel>>8)&0xFF)/9.0;
								alpha += (float)((new_pixel)&0xFF)/9.0;
							}
						}
						// At this point we should round the values.
						*blurred_bits = (uint32)blue << 24 | (uint32)green << 16
							| (uint32)red << 8 | (uint32)alpha;
					} else {
						*blurred_bits = drawer->GetPixel(left_top + BPoint(x,y));
					}
					blurred_bits++;
				}
				blurred_bits += blurred_bpr - (int32)rc.Width();
			}

			blurred_bits = (int32*)blurred->Bits();
			if (rc.IsValid()) {
				for (int32 y=0;y<rc.Height()+1;y++) {
					for (int32 x=0;x<rc.Width()+1;x++) {
						*(bits_origin + (int32)(left_top.x + x) +
							(int32)((left_top.y+y)*bpr)) = *blurred_bits;
						blurred_bits++;
					}
					blurred_bits += blurred_bpr - (int32)rc.Width();
				}
			}


			prev_point = point;
			the_script->AddPoint(point);
		} else {
			rc = BRect(0,0,-1,-1);
		}

		window->Lock();
		if (rc.IsValid()) {
			view->UpdateImage(rc);
			view->Sync();
			SetLastUpdatedRect(LastUpdatedRect() | rc);
		}
		view->getCoords(&point,&buttons);
		window->Unlock();

		snooze(20 * 1000);
	}
	delete blurred;
	delete drawer;
	return the_script;
}


int32
BlurTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
BlurTool::ConfigView()
{
	return new BlurToolConfigView(this);
}


const void*
BlurTool::ToolCursor() const
{
	return HS_BLUR_CURSOR;
}


const char*
BlurTool::HelpString(bool isInUse) const
{
	return StringServer::ReturnString((isInUse ? BLUR_TOOL_IN_USE_STRING
		: BLUR_TOOL_READY_STRING));
}


// #pragma mark -- BlurToolConfigView


BlurToolConfigView::BlurToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("value", 0x00000000);
		message->AddInt32("option", CONTINUITY_OPTION);

		fContinuity = new BCheckBox(StringServer::ReturnString(CONTINUOUS_STRING),
			message);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fBlurSize =
			new NumberSliderControl(StringServer::ReturnString(SIZE_STRING),
			"1", message, 1, 100, false);
		layout->AddView(fBlurSize);

		BSeparatorView* view =
			new BSeparatorView(StringServer::ReturnString(MODE_STRING),
			B_HORIZONTAL, B_FANCY_BORDER, BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_CENTER));
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

		BGridLayout* gridLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fBlurSize->LabelLayoutItem(), 0, 0)
			.Add(fBlurSize->TextViewLayoutItem(), 1, 0)
			.Add(fBlurSize->Slider(), 2, 0);
		gridLayout->SetMaxColumnWidth(1, StringWidth("1000"));
		gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, 5.0)
			.Add(gridLayout->View())
			.AddStrut(5.0)
			.Add(view)
			.AddGroup(B_HORIZONTAL)
				.AddStrut(5.0)
				.Add(fContinuity)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(CONTINUITY_OPTION) != B_CONTROL_OFF)
			fContinuity->SetValue(B_CONTROL_ON);
	}
}


void
BlurToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fBlurSize->SetTarget(this);
	fContinuity->SetTarget(this);
}
