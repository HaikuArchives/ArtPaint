/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License
 *
 * Authors:
 *      Dale Cieslak <dcieslak@yahoo.com>
 *
 */

 #include "YUVColorControl.h"


 #include "ColorUtilities.h"


 YUVColorControl::YUVColorControl(rgb_color c)
 	: MultichannelColorControl(c, "Y", "U", "V", "A")
 {
	slider1->SetResolution(1);
	slider2->SetResolution(1);
	slider3->SetResolution(1);
	slider4->SetResolution(1);
 }


 void
 YUVColorControl::SetValue(rgb_color c)
 {
 	float y, u, v;

 	rgb2yuv((float)c.red, (float)c.green, (float)c.blue, y, u, v);

 	slider1->SetValue(y);
 	slider2->SetValue(u);
 	slider3->SetValue(v);
 	slider4->SetValue(c.alpha);

 	rgb_color color1s, color1e;
 	rgb_color color2s, color2e;
 	rgb_color color3s, color3e;
	rgb_color color4s, color4e;

	float r, g, b;
	yuv2rgb(0.0, u, v, r, g, b);
	color1s = {(uint8)r, (uint8)g, (uint8)b, 255};
	yuv2rgb(y, 0.0, v, r, g, b);
	color2s = {(uint8)r, (uint8)g, (uint8)b, 255};
	yuv2rgb(y, u, 0.0, r, g, b);
	color3s = {(uint8)r, (uint8)g, (uint8)b, 255};
	color4s = {c.red, c.green, c.blue, 0};
	yuv2rgb(255., u, v, r, g, b);
	color1e = {(uint8)r, (uint8)g, (uint8)b, 255};
	yuv2rgb(y, 255., v, r, g, b);
	color2e = {(uint8)r, (uint8)g, (uint8)b, 255};
	yuv2rgb(y, u, 255., r, g, b);
	color3e = {(uint8)r, (uint8)g, (uint8)b, 255};
	color4e = {c.red, c.green, c.blue, 255};

	slider1->Slider()->SetColors(color1s, color1e);
	slider2->Slider()->SetColors(color2s, color2e);
	slider3->Slider()->SetColors(color3s, color3e);
	slider4->Slider()->SetColors(color4s, color4e);
 }


 void
 YUVColorControl::_SetColor(float one, float two,
 	float three, float four)
 {
 	float r, g, b;

 	yuv2rgb(one, two, three, r, g, b);

 	value.bytes[0] = (uint8)r;
 	value.bytes[1] = (uint8)g;
 	value.bytes[2] = (uint8)b;
 	value.bytes[3] = (uint8)four;
 }
