/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "ColorSlider.h"


#include "BitmapUtilities.h"
#include "PixelOperations.h"
#include "SettingsServer.h"
#include "UtilityClasses.h"


#define RES 128


ColorSlider::ColorSlider(BRect frame, const char* name, const char* label, BMessage* message,
	int32 minValue, int32 maxValue, thumb_style thumbType, uint32 resizingMode, uint32 flags)
	:
	BSlider(frame, name, label, message, minValue, maxValue, thumbType, resizingMode, flags)
{
	gradient = new BBitmap(BRect(BPoint(0, 0), BSize(RES, 0)), B_RGBA32);
	rgb_color start = {255, 0, 0, 255};
	rgb_color end = {0, 255, 0, 255};

	SetColors(start, end);
}


ColorSlider::ColorSlider(BRect frame, const char* name, const char* label, BMessage* message,
	int32 minValue, int32 maxValue, orientation posture, thumb_style thumbType, uint32 resizingMode,
	uint32 flags)
	:
	BSlider(
		frame, name, label, message, minValue, maxValue, posture, thumbType, resizingMode, flags)
{
	gradient = new BBitmap(BRect(BPoint(0, 0), BSize(RES, 0)), B_RGBA32);
	rgb_color start = {255, 0, 0, 255};
	rgb_color end = {0, 255, 0, 255};

	SetColors(start, end);
}


ColorSlider::ColorSlider(const char* name, const char* label, BMessage* message, int32 minValue,
	int32 maxValue, orientation posture, thumb_style thumbType, uint32 flags)
	:
	BSlider(name, label, message, minValue, maxValue, posture, thumbType, flags)
{
	gradient = new BBitmap(BRect(BPoint(0, 0), BSize(RES, 0)), B_RGBA32);
	rgb_color start = {255, 0, 0, 255};
	rgb_color end = {0, 255, 0, 255};

	SetColors(start, end);
}


ColorSlider::ColorSlider(BMessage* archive)
	:
	BSlider(archive)
{
}


ColorSlider::~ColorSlider()
{
	if (gradient != NULL)
		delete gradient;
}


void
ColorSlider::SetColors(rgb_color start, rgb_color end)
{
	fColorStart = start;
	fColorEnd = end;

	union color_conversion pixel;
	pixel.word = RGBColorToBGRA(start);

	uint32* bits = (uint32*)gradient->Bits();

	int32 rowLen = gradient->BytesPerRow() / 4;

	float r_delta = (float)(end.red - start.red) / (float)(RES);
	float g_delta = (float)(end.green - start.green) / (float)(RES);
	float b_delta = (float)(end.blue - start.blue) / (float)(RES);
	float a_delta = (float)(end.alpha - start.alpha) / (float)(RES);

	float r = (float)pixel.bytes[2];
	float g = (float)pixel.bytes[1];
	float b = (float)pixel.bytes[0];
	float a = (float)pixel.bytes[3];

	for (int i = 0; i < rowLen; ++i) {
		*bits++ = pixel.word;

		r += r_delta;
		g += g_delta;
		b += b_delta;
		a += a_delta;

		pixel.bytes[0] = (uint8)min_c(255, max_c(0, b));
		pixel.bytes[1] = (uint8)min_c(255, max_c(0, g));
		pixel.bytes[2] = (uint8)min_c(255, max_c(0, r));
		pixel.bytes[3] = (uint8)min_c(255, max_c(0, a));
	}
}


void
ColorSlider::SetColors(BList* colors)
{
	union color_conversion pixel, start, end;

	uint32* bits = (uint32*)gradient->Bits();

	uint32 numColors = colors->CountItems();
	uint32 step = ceil((float)RES / (numColors - 1));

	for (uint32 i = 0; i < numColors - 1; ++i) {
		start.word = *((uint32*)colors->ItemAt(i));
		end.word = *((uint32*)colors->ItemAt(i + 1));

		pixel.word = start.word;

		float r_delta = (float)(end.bytes[2] - start.bytes[2]) / (float)step;
		float g_delta = (float)(end.bytes[1] - start.bytes[1]) / (float)step;
		float b_delta = (float)(end.bytes[0] - start.bytes[0]) / (float)step;
		float a_delta = (float)(end.bytes[3] - start.bytes[3]) / (float)step;

		for (uint32 j = 0; j < step; ++j) {
			*bits++ = pixel.word;

			pixel.bytes[0] = min_c(255, pixel.bytes[0] + ceil(b_delta));
			pixel.bytes[1] = min_c(255, pixel.bytes[1] + ceil(g_delta));
			pixel.bytes[2] = min_c(255, pixel.bytes[2] + ceil(r_delta));
			pixel.bytes[3] = min_c(255, pixel.bytes[3] + ceil(a_delta));
		}
	}
}


void
ColorSlider::DrawBar()
{
	BRect frame = BarFrame();
	BView* view = OffscreenView();

	uint32 color1, color2;
	rgb_color rgb1, rgb2;
	rgb1.red = rgb1.green = rgb1.blue = 0xBB;
	rgb2.red = rgb2.green = rgb2.blue = 0x99;
	rgb1.alpha = rgb2.alpha = 0xFF;
	color1 = RGBColorToBGRA(rgb1);
	color2 = RGBColorToBGRA(rgb2);

	if (SettingsServer* server = SettingsServer::Instance()) {
		BMessage settings;
		server->GetApplicationSettings(&settings);

		color1 = settings.GetUInt32(skBgColor1, color1);
		color2 = settings.GetUInt32(skBgColor2, color2);
	}

	BBitmap* tmpBitmap = new BBitmap(gradient);
	BitmapUtilities::CheckerBitmap(gradient, color1, color2, 4);
	BitmapUtilities::CompositeBitmapOnSource(gradient, gradient, tmpBitmap, gradient->Bounds());

	view->DrawBitmap(gradient, gradient->Bounds(), frame);
	view->StrokeRect(frame);
}
