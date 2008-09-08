/*

	Filename:	HSVControl.cpp
	Contents:	HSVControl-class definitions
	Author:		Heikki Suhonen

*/

#include <Bitmap.h>

#include "HSVControl.h"
#include "UtilityClasses.h"
#include "PixelOperations.h"

HSVControl::HSVControl(BPoint position, rgb_color c)
	:	VisualColorControl(position,c,"H","S","V","A")
{
	float red = value.bytes[2];
	float green = value.bytes[1];
	float blue = value.bytes[0];

	rgb_to_hsv(red,green,blue,&h_value,&s_value,&v_value);
}

int32 HSVControl::value_at_1()
{
	// Return the Hue-value
	return (int32)h_value;
}

int32 HSVControl::value_at_2()
{
	// Return the Saturation-value
	return (int32)s_value;
}

int32 HSVControl::value_at_3()
{
	// Return the Value-value
	return (int32)v_value;
}


void HSVControl::MouseDown(BPoint point)
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

		if (Message() != NULL) {
			// if invocation-message has "buttons" we should replace it
			if (Message()->HasInt32("buttons"))
				Message()->ReplaceInt32("buttons",buttons);
		}

		float previous_value = -500;
		float orig_h = h_value;
		float orig_s = s_value;
		float orig_v = v_value;
		int32 orig_x = (int32)point.x;
		if (modifiers() & B_LEFT_SHIFT_KEY) {
			float prev_h = -500;
			float prev_s = -500;
			float prev_v = -500;
			while (buttons) {
				// Change all the values linearily
				h_value = ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_1()-min_value_at_1());
				h_value = min_c(h_value,max_value_at_1());
				h_value = max_c(h_value,min_value_at_1());

				s_value = min_value_at_2() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_2()-min_value_at_2());
				s_value = min_c(s_value,max_value_at_2());
				s_value = max_c(s_value,min_value_at_2());

				v_value = min_value_at_3() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_3()-min_value_at_3());
				v_value = min_c(v_value,max_value_at_3());
				v_value = max_c(v_value,min_value_at_3());

//				float red_value = max_c(0,min_c(255,(1*y_value + 0.956*i_value + 0.621*q_value)));
//				float green_value = max_c(0,min_c(255,(1*y_value - 0.272*i_value - 0.647*q_value)));
//				float blue_value = max_c(0,min_c(255,(1*y_value - 1.105*i_value + 1.702*q_value)));
				float red_value;
				float green_value;
				float blue_value;

				hsv_to_rgb(h_value,s_value,v_value,&red_value,&green_value,&blue_value);

				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;

				if ((h_value != prev_h) || (s_value != prev_s) || (v_value != prev_v)) {
					prev_h = h_value;
					prev_s = s_value;
					prev_v = v_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		else if (modifiers() & B_LEFT_CONTROL_KEY) {
			float prev_h = -500;
			float prev_s = -500;
			float prev_v = -500;
			while (buttons) {
				// Change all the values proportionally
				h_value = orig_h - (orig_x-point.x);
				h_value = min_c(h_value,max_value_at_1());
				h_value = max_c(h_value,min_value_at_1());

				s_value = orig_s - (orig_x-point.x);
				s_value = min_c(s_value,max_value_at_2());
				s_value = max_c(s_value,min_value_at_2());

				v_value = orig_v - (orig_x-point.x);
				v_value = min_c(v_value,max_value_at_3());
				v_value = max_c(v_value,min_value_at_3());

				float red_value;
				float green_value;
				float blue_value;

				hsv_to_rgb(h_value,s_value,v_value,&red_value,&green_value,&blue_value);

				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;

				if ((h_value != prev_h) || (s_value != prev_s) || (v_value != prev_v)) {
					prev_h = h_value;
					prev_s = s_value;
					prev_v = v_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}



		else if (((int32)(point.y / COLOR_HEIGHT)) == 0) {
			// Here we change the Hue-value
			while (buttons) {
				// make sure the value does not exceed limits
				h_value = ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_1()-min_value_at_1());
				h_value = min_c(h_value,max_value_at_1());
				h_value = max_c(h_value,min_value_at_1());

				float red_value;
				float green_value;
				float blue_value;
				hsv_to_rgb(h_value,s_value,v_value,&red_value,&green_value,&blue_value);

				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;


				if (h_value != previous_value) {
					previous_value = h_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}

		else if	(((int32)(point.y / COLOR_HEIGHT)) == 1) {
			// Here we change the Saturation-value
			while (buttons) {
				// make sure the value does not exceed limits
				s_value = min_value_at_2() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_2()-min_value_at_2());
				s_value = min_c(s_value,max_value_at_2());
				s_value = max_c(s_value,min_value_at_2());

				float red_value;
				float green_value;
				float blue_value;

				hsv_to_rgb(h_value,s_value,v_value,&red_value,&green_value,&blue_value);

				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;

				if (s_value != previous_value) {
					previous_value = s_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}

		else if	(((int32)(point.y / COLOR_HEIGHT)) == 2){
			// Here we change the Value-value
			while (buttons) {
				// make sure the value does not exceed limits
				v_value = min_value_at_3() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_3()-min_value_at_3());
				v_value = min_c(v_value,max_value_at_3());
				v_value = max_c(v_value,min_value_at_3());

				float red_value;
				float green_value;
				float blue_value;

				hsv_to_rgb(h_value,s_value,v_value,&red_value,&green_value,&blue_value);

				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;

				if (v_value != previous_value) {
					previous_value = v_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		else {
			while (buttons) {
				// Change the alpha
				int32 alpha_value = (int32)(((point.x-4-ramp_left_edge)/RAMP_WIDTH)*255);
				alpha_value = min_c(alpha_value,255);
				alpha_value = max_c(alpha_value,0);

				value.bytes[3] = alpha_value;

				if (alpha_value != previous_value) {
					previous_value = alpha_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		if (Message() != NULL) {
			// if invocation-message has "color" we should replace it
			if (Message()->HasInt32("color"))
				Message()->ReplaceInt32("color",value.word);
		}
		Invoke();
	}
}


void HSVControl::CalcRamps()
{
	// all the ramps are 256 pixels in width and 1 pixel in height
	// and 32-bit mode

	// the order of colors in a bitmap is BGRA
	uint32 *bits = (uint32*)ramp1->Bits();
	union {
		uint8 bytes[4];
		uint32 word;
	} color,black,white;
	for (int32 i=0;i<256;i++) {
		float red_value;
		float green_value;
		float blue_value;
		float temp_value = i*(max_value_at_1()-min_value_at_1())/255.0+min_value_at_1();
		hsv_to_rgb(temp_value,s_value,v_value,&red_value,&green_value,&blue_value);
		color.bytes[0] = (uint8)blue_value;
		color.bytes[1] = (uint8)green_value;
		color.bytes[2] = (uint8)red_value;
		color.bytes[3] = (uint8)255;

		*bits++ = color.word;
	}
	bits = (uint32*)ramp2->Bits();
	for (int32 i=0;i<256;i++) {
		float red_value;
		float green_value;
		float blue_value;
		float temp_value = i*(max_value_at_2()-min_value_at_2())/255.0+min_value_at_2();
		hsv_to_rgb(h_value,temp_value,v_value,&red_value,&green_value,&blue_value);
		color.bytes[0] = (uint8)blue_value;
		color.bytes[1] = (uint8)green_value;
		color.bytes[2] = (uint8)red_value;
		color.bytes[3] = (uint8)255;

		*bits++ = color.word;
	}

	bits = (uint32*)ramp3->Bits();
	for (int32 i=0;i<256;i++) {
		float red_value;
		float green_value;
		float blue_value;
		float temp_value = i*(max_value_at_3()-min_value_at_3())/255.0+min_value_at_3();
		hsv_to_rgb(h_value,s_value,temp_value,&red_value,&green_value,&blue_value);
		color.bytes[0] = (uint8)blue_value;
		color.bytes[1] = (uint8)green_value;
		color.bytes[2] = (uint8)red_value;
		color.bytes[3] = (uint8)255;

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


void HSVControl::SetValue(int32 val)
{
	value.word = val;
	float red = value.bytes[2];
	float green = value.bytes[1];
	float blue = value.bytes[0];

	rgb_to_hsv(red,green,blue,&h_value,&s_value,&v_value);

	CalcRamps();
	Draw(Bounds());
}


void HSVControl::SetValue(rgb_color c)
{
	value.bytes[0] = c.blue;
	value.bytes[1] = c.green;
	value.bytes[2] = c.red;
	value.bytes[3] = c.alpha;

	float red = value.bytes[2];
	float green = value.bytes[1];
	float blue = value.bytes[0];

	rgb_to_hsv(red,green,blue,&h_value,&s_value,&v_value);

	CalcRamps();
	Draw(Bounds());
}
