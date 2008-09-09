/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>

#include "CMYControl.h"
#include "UtilityClasses.h"
#include "PixelOperations.h"

CMYControl::CMYControl(BPoint position, rgb_color c)
	:	VisualColorControl(position,c,"C","M","Y","A")
{
}

int32 CMYControl::value_at_1()
{
//	return 255 - ((value>>8) & 0xFF);
	return 255 - value.bytes[2];
}

int32 CMYControl::value_at_2()
{
//	return 255 - ((value>>16) & 0xFF);
	return 255 - value.bytes[1];
}

int32 CMYControl::value_at_3()
{
//	return 255 - ((value>>24) & 0xFF);
	return 255 - value.bytes[0];
}


void CMYControl::MouseDown(BPoint point)
{
	BRect plate_rect = BRect(Bounds().Width()-PLATE_WIDTH+2,2,Bounds().Width()-2,Bounds().Height()-2);
	if (plate_rect.Contains(point)) {
		// If the point is on the color-plate we drag that color.
		// Start a drag'n'drop session.
		BBitmap *dragged_map = new BBitmap(BRect(0,0,15,15),B_RGB32,TRUE);
		BView *dragger_view = new BView(BRect(0,0,15,15),"dragger_view",B_FOLLOW_NONE,B_WILL_DRAW);
		rgb_color c = ValueAsColor();
		dragged_map->AddChild(dragger_view);
		dragged_map->Lock();
		dragger_view->SetHighColor(c);
		dragger_view->FillRect(dragger_view->Bounds());
		dragger_view->Sync();
		dragged_map->Unlock();
		BMessage dragger_message(B_PASTE);
		dragger_message.AddData("RGBColor",B_RGB_COLOR_TYPE,&c,sizeof(rgb_color));
		DragMessage(&dragger_message,dragged_map,BPoint(7,7));
	}
	else {
		uint32 buttons;
		Window()->CurrentMessage()->FindInt32("buttons",(int32*)&buttons);
		int32 *changed_value;
		int32 previous_value = value.word;
		if (Message() != NULL) {
			// if invocation-message has "buttons" we should replace it
			if (Message()->HasInt32("buttons"))
				Message()->ReplaceInt32("buttons",buttons);
		}

	//	int32 red_value = (value>>8) & 0xFF;
	//	int32 green_value = (value>>16) & 0xFF;
	//	int32 blue_value = (value>>24) & 0xFF;
	//	int32 alpha_value = value & 0xFF;

		int32 red_value = value.bytes[2];
		int32 green_value = value.bytes[1];
		int32 blue_value = value.bytes[0];
		int32 alpha_value = value.bytes[3];


		if (((int32)(point.y / COLOR_HEIGHT)) == 0)
			changed_value = &red_value;
		else if	(((int32)(point.y / COLOR_HEIGHT)) == 1)
			changed_value = &green_value;
		else if (((int32)(point.y / COLOR_HEIGHT)) == 2)
			changed_value = &blue_value;
		else
			changed_value = &alpha_value;

		bool change_all_values_linear = FALSE;
		bool change_all_values_proportional = FALSE;
		int32 red_orig = red_value;
		int32 green_orig = green_value;
		int32 blue_orig = blue_value;
		int32 orig_x = (int32)point.x;
		if (modifiers() & B_LEFT_SHIFT_KEY) {
			change_all_values_linear = TRUE;
		}
		else if (modifiers() & B_LEFT_CONTROL_KEY) {
			change_all_values_proportional = TRUE;
		}
		while (buttons) {
			// make sure the value does not exceed limits
			if ((change_all_values_linear == FALSE) && (change_all_values_proportional == FALSE)) {
				*changed_value = (int32)(255 - (((point.x-4-ramp_left_edge)/RAMP_WIDTH)*255));
				*changed_value = min_c(*changed_value,255);
				*changed_value = max_c(*changed_value,0);

				if (changed_value == &alpha_value)
					alpha_value = 255 - alpha_value;
			}
			else if (change_all_values_linear) {
				// Change all of the values
				red_value = (int32)(255 - (((point.x-4-ramp_left_edge)/RAMP_WIDTH)*255));
				red_value = min_c(red_value,255);
				red_value = max_c(red_value,0);

				green_value = (int32)(255 - (((point.x-4-ramp_left_edge)/RAMP_WIDTH)*255));
				green_value = min_c(green_value,255);
				green_value = max_c(green_value,0);

				blue_value = (int32)(255 - (((point.x-4-ramp_left_edge)/RAMP_WIDTH)*255));
				blue_value = min_c(blue_value,255);
				blue_value = max_c(blue_value,0);
			}
			else if (change_all_values_proportional) {
				// Change all of the values
				red_value = (int32)(red_orig - (point.x-orig_x));
				red_value = min_c(red_value,255);
				red_value = max_c(red_value,0);

				green_value = (int32)(green_orig - (point.x-orig_x));
				green_value = min_c(green_value,255);
				green_value = max_c(green_value,0);

				blue_value = (int32)(blue_orig - (point.x-orig_x));
				blue_value = min_c(blue_value,255);
				blue_value = max_c(blue_value,0);
			}

			value.bytes[0] = blue_value;
			value.bytes[1] = green_value;
			value.bytes[2] = red_value;
			value.bytes[3] = alpha_value;

			if (value.word != uint32(previous_value)) {
				previous_value = value.word;
				CalcRamps();
				Draw(Bounds());
			}
			GetMouse(&point,&buttons);
		}

		if (Message() != NULL) {
			// if invocation-message has "color" we should replace it
			if (Message()->HasInt32("color"))
				Message()->ReplaceInt32("color",value.word);
		}
		Invoke();
	}
}


void CMYControl::CalcRamps()
{
	// All the ramps are 256 pixels in width and 1 pixel in height
	// and in 32-bit mode.

	// The order of colors in a bitmap is BGRA.
	uint32 *bits = (uint32*)ramp1->Bits();

	union {
		uint8	bytes[4];
		uint32	word;
	} color,black,white;

	for (int32 i=0;i<256;i++) {
//		*bits++ = (value & 0xFF000000) + (value & 0x00FF0000) + ((255<<8) - ((i<<8) & 0x0000FF00));
		color.bytes[0] = value.bytes[0];
		color.bytes[1] = value.bytes[1];
		color.bytes[2] = 255 - i;
		color.bytes[3] = value.bytes[3];
		*bits++ = color.word;
	}
	bits = (uint32*)ramp2->Bits();
	for (int32 i=0;i<256;i++) {
//		*bits++ = (value & 0xFF000000) + ((255<<16) - ((i<<16) & 0x00FF0000)) + (value & 0x0000FF00);
		color.bytes[0] = value.bytes[0];
		color.bytes[1] = 255-i;
		color.bytes[2] = value.bytes[2];
		color.bytes[3] = value.bytes[3];
		*bits++ = color.word;
	}
	bits = (uint32*)ramp3->Bits();
	for (int32 i=0;i<256;i++) {
//		*bits++ = ((255<<24) - ((i<<24) & 0xFF000000)) + (value & 0x00FF0000) + (value & 0x0000FF00);
		color.bytes[0] = 255-i;
		color.bytes[1] = value.bytes[1];
		color.bytes[2] = value.bytes[2];
		color.bytes[3] = value.bytes[3];
		*bits++ = color.word;
	}
	bits = (uint32*)ramp4->Bits();
	black.word = 0x00000000;
	black.bytes[3] = 0xFF;
	white.word = 0xFFFFFFFF;
	color.word = value.word;
	for (int32 i=0;i<256;i++) {
		if ((i%2) == 0) {
			color.word = mix_2_pixels_fixed(value.word,black.word,32768/255*i);
		}
		else
			color.word = mix_2_pixels_fixed(value.word,white.word,32768/255*i);
		*bits++ = color.word;
	}
}
