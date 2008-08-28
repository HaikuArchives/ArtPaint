/* 

	Filename:	YUVControl.cpp
	Contents:	YUVControl-class definitions		
	Author:		Heikki Suhonen
	
*/

#include <Bitmap.h>

#include "YUVControl.h"
#include "UtilityClasses.h"
#include "PixelOperations.h"

YUVControl::YUVControl(BPoint position, rgb_color c)
	:	VisualColorControl(position,c,"Y","U","V","A")
{
//	float red = (value>>8)&0xFF;
//	float green = (value>>16)&0xFF;
//	float blue = (value>>24)&0xFF;
	float red = value.bytes[2];
	float green = value.bytes[1];
	float blue = value.bytes[0];
	
	y_value = (0.299*red+0.587*green+0.114*blue);
	u_value = (-0.147*red-0.289*green+0.437*blue);
	v_value = (0.615*red-0.515*green-0.100*blue);
}

int32 YUVControl::value_at_1()
{
	return (int32)y_value;
}

int32 YUVControl::value_at_2()
{
	return (int32)u_value;
}

int32 YUVControl::value_at_3()
{
	return (int32)v_value;
}


void YUVControl::MouseDown(BPoint point)
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
		float orig_y = y_value;
		float orig_u = u_value;
		float orig_v = v_value;
		int32 orig_x = (int32)point.x;
		if (modifiers() & B_LEFT_SHIFT_KEY) {
			float prev_y = -500;
			float prev_u = -500;
			float prev_v = -500;
			while (buttons) {
				// Change all the values linearily				
				y_value = ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_1()-min_value_at_1());
				y_value = min_c(y_value,max_value_at_1());
				y_value = max_c(y_value,min_value_at_1());

				u_value = min_value_at_2() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_2()-min_value_at_2());
				u_value = min_c(u_value,max_value_at_2());
				u_value = max_c(u_value,min_value_at_2());

				v_value = min_value_at_3() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_3()-min_value_at_3());
				v_value = min_c(v_value,max_value_at_3());
				v_value = max_c(v_value,min_value_at_3());
		
				float red_value = max_c(0,min_c(255,(1*y_value + 0*u_value + 1.140*v_value)));
				float green_value = max_c(0,min_c(255,(1*y_value - 0.394*u_value - 0.581*v_value)));
				float blue_value = max_c(0,min_c(255,(1*y_value + 2.028*u_value + 0*v_value)));
	
				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;

				if ((y_value != prev_y) || (u_value != prev_u) || (v_value != prev_v)) {
					prev_y = y_value;
					prev_u = u_value;
					prev_v = v_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		else if (modifiers() & B_LEFT_CONTROL_KEY) {
			float prev_y = -500;
			float prev_u = -500;
			float prev_v = -500;
			while (buttons) {
				// Change all the values proportionally
				y_value = orig_y - (orig_x-point.x);
				y_value = min_c(y_value,max_value_at_1());
				y_value = max_c(y_value,min_value_at_1());

				u_value = orig_u - (orig_x-point.x);
				u_value = min_c(u_value,max_value_at_2());
				u_value = max_c(u_value,min_value_at_2());

				v_value = orig_v - (orig_x-point.x);
				v_value = min_c(v_value,max_value_at_3());
				v_value = max_c(v_value,min_value_at_3());
				
				float red_value = max_c(0,min_c(255,(1*y_value + 0*u_value + 1.140*v_value)));
				float green_value = max_c(0,min_c(255,(1*y_value - 0.394*u_value - 0.581*v_value)));
				float blue_value = max_c(0,min_c(255,(1*y_value + 2.028*u_value + 0*v_value)));

				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;

				if ((y_value != prev_y) || (u_value != prev_u) || (v_value != prev_v)) {
					prev_y = y_value;
					prev_u = u_value;
					prev_v = v_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		
		if (((int32)(point.y / COLOR_HEIGHT)) == 0) {
			// Here we change the Y-value
			while (buttons) {
				// make sure the value does not exceed limits
				y_value = ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_1()-min_value_at_1());
				y_value = min_c(y_value,max_value_at_1());
				y_value = max_c(y_value,min_value_at_1());
		
				float red_value = max_c(0,min_c(255,(1*y_value + 0*u_value + 1.140*v_value)));
				float green_value = max_c(0,min_c(255,(1*y_value - 0.394*u_value - 0.581*v_value)));
				float blue_value = max_c(0,min_c(255,(1*y_value + 2.028*u_value + 0*v_value)));
				
	//			value = (int32)blue_value << 24 | (int32)green_value<<16 | (int32)red_value<<8 |value & 0xFF;
				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;
							 			
										
				if (y_value != previous_value) {
					previous_value = y_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		
		else if	(((int32)(point.y / COLOR_HEIGHT)) == 1) {
			// Here we change the I-value
			while (buttons) {
				// make sure the value does not exceed limits
				u_value = min_value_at_2() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_2()-min_value_at_2());
				u_value = min_c(u_value,max_value_at_2());
				u_value = max_c(u_value,min_value_at_2());
		
				float red_value = max_c(0,min_c(255,(1*y_value + 0*u_value + 1.140*v_value)));
				float green_value = max_c(0,min_c(255,(1*y_value - 0.394*u_value - 0.581*v_value)));
				float blue_value = max_c(0,min_c(255,(1*y_value + 2.028*u_value + 0*v_value)));
				
	//			value = (int32)blue_value << 24 | (int32)green_value<<16 | (int32)red_value<<8 |value & 0xFF;
				value.bytes[0] = (uint8)blue_value;
				value.bytes[1] = (uint8)green_value;
				value.bytes[2] = (uint8)red_value;
													
				if (u_value != previous_value) {
					previous_value = u_value;
					CalcRamps();
					Draw(Bounds());
				}
				GetMouse(&point,&buttons);
			}
		}
		
		else if	(((int32)(point.y / COLOR_HEIGHT)) == 2){
			// Here we change the Q-value
			while (buttons) {
				// make sure the value does not exceed limits
				v_value = min_value_at_3() + ((point.x-4-ramp_left_edge)/RAMP_WIDTH)*(max_value_at_3()-min_value_at_3());
				v_value = min_c(v_value,max_value_at_3());
				v_value = max_c(v_value,min_value_at_3());
		
				float red_value = max_c(0,min_c(255,(1*y_value + 0*u_value + 1.140*v_value)));
				float green_value = max_c(0,min_c(255,(1*y_value - 0.394*u_value - 0.581*v_value)));
				float blue_value = max_c(0,min_c(255,(1*y_value + 2.028*u_value + 0*v_value)));
	
	//			value = (int32)blue_value << 24 | (int32)green_value<<16 | (int32)red_value<<8 |value & 0xFF;
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


void YUVControl::CalcRamps()
{
	// all the ramps are 256 pixels in width and 1 pixel in height
	// and 32-bit mode

	// the order of colors in a bitmap is BGRA
	uint32 *bits = (uint32*)ramp1->Bits();	
	union {
		uint8 bytes[4];
		uint32 word;
	} color,white,black;
	
	for (int32 i=0;i<256;i++) {
		float red_value = max_c(0,min_c(255,(i + 0*u_value + 1.140*v_value)));
		float green_value = max_c(0,min_c(255,(i - 0.394*u_value - 0.581*v_value)));
		float blue_value = max_c(0,min_c(255,(i + 2.028*u_value + 0*v_value)));
		color.bytes[0] = (uint8)blue_value;
		color.bytes[1] = (uint8)green_value;
		color.bytes[2] = (uint8)red_value;
		color.bytes[3] = value.bytes[3];
//		*bits++ = ((int32)blue_value<<24) + ((int32)green_value <<16) + ((int32)red_value<<8);		
		*bits++ = color.word;
	}	
	bits = (uint32*)ramp2->Bits();	
	for (int32 i=0;i<256;i++) {
		float temp_value = min_value_at_2() + (max_value_at_2() - min_value_at_2())/256 * i;
		float red_value = max_c(0,min_c(255,(y_value + 0*temp_value + 1.140*v_value)));
		float green_value = max_c(0,min_c(255,(y_value - 0.394*temp_value - 0.581*v_value)));
		float blue_value = max_c(0,min_c(255,(y_value + 2.028*temp_value + 0*v_value)));
		color.bytes[0] = (uint8)blue_value;
		color.bytes[1] = (uint8)green_value;
		color.bytes[2] = (uint8)red_value;
		color.bytes[3] = value.bytes[3];
//		*bits++ = ((int32)blue_value<<24) + ((int32)green_value <<16) + ((int32)red_value<<8);		
		*bits++ = color.word;
	}
	
	bits = (uint32*)ramp3->Bits();	
	for (int32 i=0;i<256;i++) {
		float temp_value = min_value_at_3() + (max_value_at_3() - min_value_at_3())/256 * i;
		float red_value = max_c(0,min_c(255,(y_value + 0*u_value + 1.140*temp_value)));
		float green_value = max_c(0,min_c(255,(y_value - 0.394*u_value - 0.581*temp_value)));
		float blue_value = max_c(0,min_c(255,(y_value + 2.028*u_value + 0*temp_value)));
		color.bytes[0] = (uint8)blue_value;
		color.bytes[1] = (uint8)green_value;
		color.bytes[2] = (uint8)red_value;
		color.bytes[3] = value.bytes[3];
//		*bits++ = ((int32)blue_value<<24) + ((int32)green_value <<16) + ((int32)red_value<<8);		
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

void YUVControl::SetValue(int32 val)
{
	value.word = val;
	float red = value.bytes[2];
	float green = value.bytes[1];
	float blue = value.bytes[0];
	
	y_value = (0.299*red+0.587*green+0.114*blue);
	u_value = (-0.147*red-0.289*green+0.437*blue);
	v_value = (0.615*red-0.515*green-0.100*blue);

	CalcRamps();
	Draw(Bounds());
}

void YUVControl::SetValue(rgb_color c)
{
//	value = ( 	((c.blue<<24) & 0xFF000000) | ((c.green<<16) & 0x00FF0000)
//				| ((c.red<<8) &0x0000FF00) | (c.alpha & 0x000000FF) );
	value.bytes[0] = c.blue;
	value.bytes[1] = c.green;
	value.bytes[2] = c.red;
	value.bytes[3] = c.alpha;
	
	float red = value.bytes[2];
	float green = value.bytes[1];
	float blue = value.bytes[0];
	
	y_value = (0.299*red+0.587*green+0.114*blue);
	u_value = (-0.147*red-0.289*green+0.437*blue);
	v_value = (0.615*red-0.515*green-0.100*blue);

	CalcRamps();
	Draw(Bounds());
}
