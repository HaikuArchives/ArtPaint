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

#include "ColorSelectorTool.h"

#include "BitmapDrawer.h"
#include "ColorPalette.h"
#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "MessageConstants.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "StatusView.h"
#include "UtilityClasses.h"


#include <Bitmap.h>
#include <Catalog.h>
#include <GridLayout.h>
#include <GroupLayoutBuilder.h>
#include <InterfaceDefs.h>
#include <RadioButton.h>
#include <Screen.h>
#include <StringView.h>
#include <Window.h>


#include <stdio.h>
#include <stdlib.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


// #pragma mark -- ColorSelectorView


class ColorSelectorView : public BView {
public:
								ColorSelectorView(BRect frame);

	virtual	void				Draw(BRect updateRect);

			void				ChangeValue(uint32);

private:
			uint32				selected_color;
			BStringView*		red_view;
			BStringView*		green_view;
			BStringView*		blue_view;
			BStringView*		alpha_view;
};


ColorSelectorView::ColorSelectorView(BRect frame)
	: BView(frame, "color selector-view", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BString color_str;
	float width = 0;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	color_str.SetToFormat("%s:", B_TRANSLATE("Red"));
	BStringView *label_view = new BStringView(BRect(2,2,2,2),
		"label view", color_str);
	font_height fHeight;
	label_view->GetFontHeight(&fHeight);

	color_str.SetToFormat(" %s:", B_TRANSLATE("Red"));
	width = label_view->StringWidth(color_str);

	color_str.SetToFormat(" %s:", B_TRANSLATE("Green"));
	width = max_c(width,label_view->StringWidth(color_str));

	color_str.SetToFormat(" %s:", B_TRANSLATE("Blue"));
	width = max_c(width,label_view->StringWidth(color_str));

	color_str.SetToFormat(" %s:", B_TRANSLATE("Alpha"));
	width = max_c(width,label_view->StringWidth(color_str));

	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);

	// The red color-view
	BRect label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	red_view = new BStringView(label_frame,"red view","");
	AddChild(red_view);

	color_str.SetToFormat("%s:", B_TRANSLATE("Green"));
	label_view = new BStringView(BRect(2,label_view->Frame().bottom,2,
		label_view->Frame().bottom),"label view",color_str);
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	green_view = new BStringView(label_frame,"green view","");
	AddChild(green_view);

	color_str.SetToFormat("%s:", B_TRANSLATE("Blue"));
	label_view = new BStringView(BRect(2,label_view->Frame().bottom,2,
		label_view->Frame().bottom),"label view",color_str);
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	blue_view = new BStringView(label_frame,"blue view","");
	AddChild(blue_view);

	color_str.SetToFormat(" %s:", B_TRANSLATE("Alpha"));
	label_view = new BStringView(BRect(2,label_view->Frame().bottom,2,
		label_view->Frame().bottom),"label view",color_str);
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	alpha_view = new BStringView(label_frame,"alpha view","");
	AddChild(alpha_view);

	ResizeTo(alpha_view->Frame().right+40,alpha_view->Frame().bottom+2);
}


void
ColorSelectorView::Draw(BRect area)
{
	BView::Draw(area);
	BRect color_rect = BRect(red_view->Frame().right+2,red_view->Frame().top+2,
		Bounds().right-2,alpha_view->Frame().bottom-2);
	SetHighColor(120,120,0,255);
	StrokeRect(color_rect,B_MIXED_COLORS);
	color_rect.InsetBy(1,1);
	SetHighColor(BGRAColorToRGB(selected_color));
	FillRect(color_rect);
}


void
ColorSelectorView::ChangeValue(uint32 new_color)
{
	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	color.word = new_color;
	selected_color = new_color;
	Draw(Bounds());
	char a_string[20];
	sprintf(a_string,"%d",color.bytes[2]);
	red_view->SetText(a_string);

	sprintf(a_string,"%d",color.bytes[1]);
	green_view->SetText(a_string);

	sprintf(a_string,"%d",color.bytes[0]);
	blue_view->SetText(a_string);

	sprintf(a_string,"%d",color.bytes[3]);
	alpha_view->SetText(a_string);
}


// #pragma mark -- ColorSelectorWindow


class ColorSelectorWindow : public BWindow {
public:
								ColorSelectorWindow(BPoint cursorLocation);

			void				ChangeValue(uint32 color);
			void				Move(BPoint cursorLocation);

private:
			BRect 				screen_bounds;
			ColorSelectorView*	cs_view;
};


ColorSelectorWindow::ColorSelectorWindow(BPoint cursor_location)
	: BWindow(BRect(0,0,0,0), B_TRANSLATE("Color picker window"), B_BORDERED_WINDOW, 0)
{
	screen_bounds = BScreen().Frame();
	cs_view = new ColorSelectorView(Bounds());
	AddChild(cs_view);

	// Here we must resize the window and move it to the correct position.
	ResizeTo(cs_view->Frame().Width(),cs_view->Frame().Height());
	Move(cursor_location);

	Show();
}


void
ColorSelectorWindow::ChangeValue(uint32 color)
{
	cs_view->ChangeValue(color);
}


void
ColorSelectorWindow::Move(BPoint cursor_location)
{
	// This function moves the window so that it is near the cursor, but does not
	// interfere with it and does not go over screen borders. The cursor location
	// is expressed in screen-coordinates.

	// See if there is enough space on the same side of the cursor than we are now.
	float width = Bounds().Width();
	float height = Bounds().Height();

	float left,top;
	if (cursor_location.x > width + 20)
		left = cursor_location.x - width - 20;
	else
		left = cursor_location.x + 20;

	if (cursor_location.y > height + 20)
		top = cursor_location.y - height - 20;
	else
		top = cursor_location.y + 20;

	MoveTo(BPoint(left,top));
}


// #pragma mark -- ColorSelectorTool


ColorSelectorTool::ColorSelectorTool()
	: DrawingTool(B_TRANSLATE("Color picker tool"),
		COLOR_SELECTOR_TOOL)
{
	fOptions = SIZE_OPTION | MODE_OPTION;
	fOptionsCount = 2;

	SetOption(SIZE_OPTION,1);
	SetOption(MODE_OPTION,HS_ALL_BUTTONS);
}


ColorSelectorTool::~ColorSelectorTool()
{
}


ToolScript*
ColorSelectorTool::UseTool(ImageView *view, uint32 buttons, BPoint point,
	BPoint view_point)
{
	BWindow *window = view->Window();
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();

	if (window != NULL) {
		BitmapDrawer *drawer = new BitmapDrawer(bitmap);

		BPoint original_point,original_view_point,prev_view_point;

		ColorSelectorWindow *cs_window = NULL;
		original_point = point;

		prev_view_point = original_view_point = view_point;
		uint32 old_color,color;
		color = drawer->GetPixel(point);
		old_color = color - 1;
		bool select_foreground = (buttons & B_PRIMARY_MOUSE_BUTTON) != 0x00;

		// for the quick calculation of square-roots
		int num_sqrt = 500;
		float sqrt_table[num_sqrt];
		for (int32 i = 0; i < num_sqrt; i++)
			sqrt_table[i] = sqrt(i);

		float half_size = fToolSettings.size/2;
		BRect rc = BRect(point.x - half_size, point.y - half_size,
			point.x + half_size, point.y + half_size);
		BRect bounds = bitmap->Bounds();
		rc = rc & bounds;

		while (buttons) {
			rc = BRect(point.x - half_size, point.y - half_size,
				point.x + half_size, point.y + half_size);
			rc = rc & bounds;
			int32 x_dist,y_sqr;

			int32 width = rc.IntegerWidth();
			int32 height = rc.IntegerHeight();
			float red=0;
			float green=0;
			float blue=0;
			float alpha=0;
			int32 number_of_pixels=0;

			for (int32 y=0;y<height+1;y++) {
				y_sqr = (int32)(point.y - rc.top - y);
				y_sqr *= y_sqr;
				int32 real_y = (int32)(rc.top + y);
				int32 real_x;
				for (int32 x=0;x<width+1;x++) {
					x_dist = (int32)(point.x-rc.left-x);
					real_x = (int32)(rc.left+x);
					int32 index = x_dist * x_dist + y_sqr;
					if (index < num_sqrt &&
						sqrt_table[index] <= half_size) {
						uint32 tmp_color = drawer->GetPixel(real_x,real_y);
						red += (tmp_color >> 8) & 0xFF;
						green += (tmp_color >> 16) & 0xFF;
						blue += (tmp_color >> 24) & 0xFF;
						alpha += (tmp_color) & 0xFF;
						number_of_pixels++;
					}
				}
			}
			if (number_of_pixels > 0) {
				red /= number_of_pixels;
				green /= number_of_pixels;
				blue /= number_of_pixels;
				alpha /= number_of_pixels;
				color = (((uint32)blue) << 24) | (((uint32)green) << 16)
					| (((uint32)red) << 8) | ((uint32)alpha);
			}

			window->Lock();
			view->getCoords(&point,&buttons,&view_point);
			// If we have not yet opened the cs window and the user moves the mouse,
			// we open the window
			if ((cs_window == NULL) && ((fabs(point.x-original_point.x)>4)
				|| (fabs(point.y-original_point.y)>4))) {
				cs_window = new ColorSelectorWindow(view->ConvertToScreen(view_point));
			}

			// If we have opened the cs_window, we can operate on it.
			if (cs_window != NULL)	{
				cs_window->Lock();
				if (color != old_color) {
					cs_window->ChangeValue(color);
					old_color = color;
					rgb_color c = BGRAColorToRGB(color);
					ColorPaletteWindow::ChangePaletteColor(c);
				}
				if (cs_window->Frame().Contains(view->ConvertToScreen(view_point))) {
					cs_window->Move(view->ConvertToScreen(view_point));
				}
				cs_window->Unlock();
			}
			window->Unlock();
			snooze(20 * 1000);
		}


		// Close the color picker window
		if (cs_window != NULL) {
			cs_window->Lock();
			cs_window->Quit();

			window->Lock();
			window->Activate(true);
			window->Unlock();
		}
		rgb_color new_color = BGRAColorToRGB(color);
		((PaintApplication*)be_app)->SetColor(new_color, select_foreground);

		// Inform all the selected color views about change in colors.
		SelectedColorsView::SendMessageToAll(HS_COLOR_CHANGED);

		delete drawer;
	}
	return NULL;
}


BView*
ColorSelectorTool::ConfigView()
{
	return new ColorSelectorToolConfigView(this);
}


const void*
ColorSelectorTool::ToolCursor() const
{
	return HS_COLOR_SELECTOR_CURSOR;
}


const char*
ColorSelectorTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Picking a color.")
		: B_TRANSLATE("Color picker tool"));
}


// #pragma mark -- ColorSelectorToolConfigView


ColorSelectorToolConfigView::ColorSelectorToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fSizeSlider =
			new NumberSliderControl(B_TRANSLATE("Size:"),
			"1", message, 1, 10, false);

		BGridLayout* sizeLayout = LayoutSliderGrid(fSizeSlider);
		layout->AddView(sizeLayout->View());
	}
}


void
ColorSelectorToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fSizeSlider->SetTarget(this);
}
