/* 

	Filename:	ColorSelectorTool.cpp
	Contents:	ColorSelectorTool-class definitions	
	Author:		Heikki Suhonen
	
*/

#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <RadioButton.h>
#include <Screen.h>
#include <stdio.h>
#include <stdlib.h>
#include <Window.h>

#include "Cursors.h"
#include "ColorSelectorTool.h"
#include "StatusView.h"
#include "ColorPalette.h"
#include "StringServer.h"

ColorSelectorTool::ColorSelectorTool()
	: DrawingTool(StringServer::ReturnString(COLOR_SELECTOR_TOOL_NAME_STRING),COLOR_SELECTOR_TOOL)
{
	options = SIZE_OPTION | MODE_OPTION;
	number_of_options = 2;
	
	SetOption(SIZE_OPTION,1);
	SetOption(MODE_OPTION,HS_ALL_BUTTONS);
}


ColorSelectorTool::~ColorSelectorTool()
{

}


ToolScript* ColorSelectorTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint view_point)
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
		bool select_foreground = (buttons & B_PRIMARY_MOUSE_BUTTON != 0x00);
		
		// for the quick calculation of square-roots
		float sqrt_table[500];
		for (int32 i=0;i<500;i++)
			sqrt_table[i] = sqrt(i);

		float half_size = settings.size/2;
		BRect rc = BRect(point.x-half_size,point.y-half_size,point.x+half_size,point.y+half_size);		
		BRect bounds = bitmap->Bounds();
		rc = rc & bounds;
		
		while (buttons) {
			rc = BRect(point.x-half_size,point.y-half_size,point.x+half_size,point.y+half_size);
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
					if (sqrt_table[x_dist*x_dist + y_sqr] <= half_size) {
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
				color = (((uint32)blue) << 24) | (((uint32)green) << 16) | (((uint32)red) << 8) | ((uint32)alpha);	
			}
			
			window->Lock();
			view->getCoords(&point,&buttons,&view_point);	
			// If we have not yet opened the cs window and the user moves the mouse,
			// we open the window
			if ((cs_window == NULL) && ((fabs(point.x-original_point.x)>4) || (fabs(point.y-original_point.y)>4))) 		
				cs_window = new ColorSelectorWindow(view->ConvertToScreen(view_point));						
			
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


		// Close the color selector window
		if (cs_window != NULL) {
			cs_window->Lock();
			cs_window->Quit();	

			window->Lock();
			window->Activate(true);
			window->Unlock();
		}		
		rgb_color new_color = BGRAColorToRGB(color);
		if (select_foreground) {
			((PaintApplication*)be_app)->SetColor(new_color,TRUE);
		}
		else {
			((PaintApplication*)be_app)->SetColor(new_color,FALSE);
		}
		// Inform all the selected color views about change in colors.
		BMessage *color_change_message = new BMessage(HS_COLOR_CHANGED);
		SelectedColorsView::sendMessageToAll(color_change_message);
		delete color_change_message;
				
		delete drawer;
	}		

	return NULL;	
}


BView* ColorSelectorTool::makeConfigView()
{
	ColorSelectorToolConfigView *target_view = new ColorSelectorToolConfigView(BRect(0,0,150,0),this);

	return target_view;
}


const char* ColorSelectorTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(COLOR_SELECTOR_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(COLOR_SELECTOR_TOOL_IN_USE_STRING);		
}

const void* ColorSelectorTool::ReturnToolCursor()
{
	return HS_COLOR_SELECTOR_CURSOR;
}


ColorSelectorView::ColorSelectorView(BRect frame)
	:	BView(frame,"color selector-view",B_FOLLOW_NONE,B_WILL_DRAW)
{
	char string[256];
	float width = 0;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	sprintf(string,"%s:",StringServer::ReturnString(RED_STRING));
	BStringView *label_view = new BStringView(BRect(2,2,2,2),"label view",string);
	font_height fHeight;
	label_view->GetFontHeight(&fHeight);

	sprintf(string," %s:",StringServer::ReturnString(RED_STRING));
	width = label_view->StringWidth(string);
	
	sprintf(string," %s:",StringServer::ReturnString(GREEN_STRING));
	width = max_c(width,label_view->StringWidth(string));
	
	sprintf(string," %s:",StringServer::ReturnString(BLUE_STRING));
	width = max_c(width,label_view->StringWidth(string));

	sprintf(string," %s:",StringServer::ReturnString(ALPHA_STRING));
	width = max_c(width,label_view->StringWidth(string));
		
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	
	// The red color-view
	BRect label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	red_view = new BStringView(label_frame,"red view","");
	AddChild(red_view);
			
	sprintf(string,"%s:",StringServer::ReturnString(GREEN_STRING));
	label_view = new BStringView(BRect(2,label_view->Frame().bottom,2,label_view->Frame().bottom),"label view",string);	
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	green_view = new BStringView(label_frame,"green view","");
	AddChild(green_view);

	sprintf(string,"%s:",StringServer::ReturnString(BLUE_STRING));
	label_view = new BStringView(BRect(2,label_view->Frame().bottom,2,label_view->Frame().bottom),"label view",string);	
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	blue_view = new BStringView(label_frame,"blue view","");	
	AddChild(blue_view);

	sprintf(string,"%s:",StringServer::ReturnString(ALPHA_STRING));
	label_view = new BStringView(BRect(2,label_view->Frame().bottom,2,label_view->Frame().bottom),"label view",string);	
	label_view->ResizeTo(width,fHeight.ascent+fHeight.descent);
	label_view->SetAlignment(B_ALIGN_RIGHT);
	AddChild(label_view);
	label_frame = label_view->Frame();
	label_frame.OffsetBy(label_frame.Width(),0);
	alpha_view = new BStringView(label_frame,"alpha view","");
	AddChild(alpha_view);

	ResizeTo(alpha_view->Frame().right+40,alpha_view->Frame().bottom+2);
}


void ColorSelectorView::Draw(BRect area)
{
	BView::Draw(area);
	BRect color_rect = BRect(red_view->Frame().right+2,red_view->Frame().top+2,Bounds().right-2,alpha_view->Frame().bottom-2);
	SetHighColor(120,120,0,255);
	StrokeRect(color_rect,B_MIXED_COLORS);
	color_rect.InsetBy(1,1);
	SetHighColor(BGRAColorToRGB(selected_color));
	FillRect(color_rect);	
}

void ColorSelectorView::ChangeValue(uint32 new_color)
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

ColorSelectorWindow::ColorSelectorWindow(BPoint cursor_location)
	:	BWindow(BRect(0,0,0,0),"Color Selector Window",B_BORDERED_WINDOW,0)
{
	// Here store the screen's bounds rectangle.
	BScreen *a_screen = new BScreen();
	screen_bounds = a_screen->Frame();
	delete a_screen;
	cs_view = new ColorSelectorView(Bounds());
	AddChild(cs_view);			

	// Here we must resize the window and move it to the correct position.
	ResizeTo(cs_view->Frame().Width(),cs_view->Frame().Height());
	Move(cursor_location);		

	Show();
}

void ColorSelectorWindow::ChangeValue(uint32 color)
{
	cs_view->ChangeValue(color);
}	


void ColorSelectorWindow::Move(BPoint cursor_location)
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




ColorSelectorToolConfigView::ColorSelectorToolConfigView(BRect rect,DrawingTool *t)
	: DrawingToolConfigView(rect,t)
{
	// The ownership of this message is then transferred to the controller.
	BMessage *message;

	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);


	// First add the controller for size.
	message = new BMessage(OPTION_CHANGED);
	message->AddInt32("option",SIZE_OPTION);
	message->AddInt32("value",tool->GetCurrentValue(SIZE_OPTION));	
	size_slider = new ControlSliderBox(controller_frame,"size",StringServer::ReturnString(SIZE_STRING),"1",message,1,10);			
	AddChild(size_slider);
		
	ResizeTo(size_slider->Bounds().Width()+2*EXTRA_EDGE,size_slider->Frame().bottom + EXTRA_EDGE);	
}


void ColorSelectorToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	size_slider->SetTarget(new BMessenger(this));
}