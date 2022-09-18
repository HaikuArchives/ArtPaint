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


#define RES 	128


ColorSlider::ColorSlider(BRect frame, const char* name,
	const char* label, BMessage* message,
	int32 minValue, int32 maxValue,
	thumb_style thumbType,
	uint32 resizingMode,
	uint32 flags)
	: BSlider(frame, name, label, message, minValue, maxValue,
		thumbType, resizingMode, flags)
{
	gradient = new BBitmap(BRect(BPoint(0, 0), BSize(RES, 0)), B_RGBA32);
	rgb_color start = {255, 0, 0, 255};
	rgb_color end = {0, 255, 0, 255};

	SetColors(start, end);
}


ColorSlider::ColorSlider(BRect frame, const char* name,
	const char* label, BMessage* message,
	int32 minValue, int32 maxValue,
	orientation posture,
	thumb_style thumbType,
	uint32 resizingMode,
	uint32 flags)
	: BSlider(frame, name, label, message, minValue, maxValue,
		posture, thumbType, resizingMode, flags)
{
	gradient = new BBitmap(BRect(BPoint(0, 0), BSize(RES, 0)), B_RGBA32);
	rgb_color start = {255, 0, 0, 255};
	rgb_color end = {0, 255, 0, 255};

	SetColors(start, end);
}


ColorSlider::ColorSlider(const char* name, const char* label,
	BMessage* message, int32 minValue,
	int32 maxValue, orientation posture,
	thumb_style thumbType,
	uint32 flags)
	: BSlider(name, label, message, minValue, maxValue,
	posture, thumbType, flags)
{
	gradient = new BBitmap(BRect(BPoint(0, 0), BSize(RES, 0)), B_RGBA32);
	rgb_color start = {255, 0, 0, 255};
	rgb_color end = {0, 255, 0, 255};

	SetColors(start, end);
}


ColorSlider::ColorSlider(BMessage* archive)
	: BSlider(archive)
{
}


ColorSlider::~ColorSlider()
{
	if (gradient != NULL)
		delete gradient;
}

void
ColorSlider::WindowActivated(bool state)
{
	BSlider::WindowActivated(state);
}


void
ColorSlider::AttachedToWindow()
{
 	BSlider::AttachedToWindow();
}


void
ColorSlider::AllAttached()
{
 	BSlider::AllAttached();
}


void
ColorSlider::AllDetached()
{
 	BSlider::AllDetached();
}


void
ColorSlider::DetachedFromWindow()
{
	BSlider::DetachedFromWindow();
}


void
ColorSlider::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BSlider::MessageReceived(message);
	}
}


void
ColorSlider::FrameMoved(BPoint newPosition)
{
	BSlider::FrameMoved(newPosition);
}


void
ColorSlider::FrameResized(float width, float height)
{
	BSlider::FrameResized(width, height);
}


void
ColorSlider::KeyDown(const char* bytes, int32 numBytes)
{
	BSlider::KeyDown(bytes, numBytes);
}


void
ColorSlider::KeyUp(const char* bytes, int32 numBytes)
{
	BSlider::KeyUp(bytes, numBytes);
}


void
ColorSlider::MouseDown(BPoint point)
{
	BSlider::MouseDown(point);
}


void
ColorSlider::MouseUp(BPoint point)
{
	BSlider::MouseUp(point);
}


void
ColorSlider::MouseMoved(BPoint point, uint32 transit,
	const BMessage* dragMessage)
{
	BSlider::MouseMoved(point, transit, dragMessage);
}


void
ColorSlider::Pulse()
{
	BSlider::Pulse();
}


void
ColorSlider::SetLabel(const char* label)
{
	BSlider::SetLabel(label);
}


void
ColorSlider::SetLimitLabels(const char* minLabel,
	const char* maxLabel)
{
	BSlider::SetLimitLabels(minLabel, maxLabel);
}


void
ColorSlider::SetValue(int32 value)
{
	BSlider::SetValue(value);
}


void
ColorSlider::SetPosition(float position)
{
	BSlider::SetPosition(position);
}


void
ColorSlider::SetEnabled(bool on)
{
	BSlider::SetEnabled(on);
}


void
ColorSlider::Draw(BRect updateRect)
{
	BSlider::Draw(updateRect);
}


void
ColorSlider::DrawSlider()
{
	BSlider::DrawSlider();
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

	int32 rowLen = gradient->BytesPerRow() / 4;
	uint32 numColors = colors->CountItems();
	uint32 step = ceil((float)RES / (numColors - 1));

	for (int i = 0; i < numColors - 1; ++i)
	{
		start.word = *((uint32*)colors->ItemAt(i));
		end.word = *((uint32*)colors->ItemAt(i + 1));

		pixel.word = start.word;

		float r_delta = (float)(end.bytes[2] - start.bytes[2]) / (float)step;
		float g_delta = (float)(end.bytes[1] - start.bytes[1]) / (float)step;
		float b_delta = (float)(end.bytes[0] - start.bytes[0]) / (float)step;
		float a_delta = (float)(end.bytes[3] - start.bytes[3]) / (float)step;

		for (int j = 0; j < step; ++j)
		{
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
	BitmapUtilities::CompositeBitmapOnSource(gradient, gradient,
		tmpBitmap, gradient->Bounds());

	view->DrawBitmap(gradient, gradient->Bounds(), frame);
	view->StrokeRect(frame);
}


void
ColorSlider::DrawHashMarks()
{
	BSlider::DrawHashMarks();
}


void
ColorSlider::DrawThumb()
{
	BSlider::DrawThumb();
}


void
ColorSlider::DrawFocusMark()
{
	BSlider::DrawFocusMark();

}


void
ColorSlider::DrawText()
{
	BSlider::DrawText();
}


void
ColorSlider::SetFlags(uint32 flags)
{
	BSlider::SetFlags(flags);
}


void
ColorSlider::SetResizingMode(uint32 mode)
{
	BSlider::SetResizingMode(mode);
}


void
ColorSlider::GetPreferredSize(float* _width,
	float* _height)
{
	BSlider::GetPreferredSize(_width, _height);
}


void
ColorSlider::ResizeToPreferred()
{
	BSlider::ResizeToPreferred();
}


void
ColorSlider::SetModificationMessage(BMessage* message)
{
	BSlider::SetModificationMessage(message);
}


void
ColorSlider::SetSnoozeAmount(int32 microSeconds)
{
	BSlider::SetSnoozeAmount(microSeconds);
}


void
ColorSlider::SetKeyIncrementValue(int32 value)
{
	BSlider::SetKeyIncrementValue(value);
}


void
ColorSlider::SetHashMarkCount(int32 count)
{
	BSlider::SetHashMarkCount(count);
}


void
ColorSlider::SetHashMarks(hash_mark_location where)
{
	BSlider::SetHashMarks(where);
}


void
ColorSlider::SetStyle(thumb_style style)
{
	BSlider::SetStyle(style);
}


void
ColorSlider::SetBarColor(rgb_color color)
{
	BSlider::SetBarColor(color);
}


void
ColorSlider::UseFillColor(bool useFill,
	const rgb_color* color)
{
	BSlider::UseFillColor(useFill, color);
}


void
ColorSlider::SetOrientation(orientation posture)
{
	BSlider::SetOrientation(posture);
}


void
ColorSlider::SetBarThickness(float thickness)
{
	BSlider::SetBarThickness(thickness);
}


void
ColorSlider::SetFont(const BFont* font,
	uint32 properties)
{
	BSlider::SetFont(font, properties);
}


void
ColorSlider::SetLimits(int32 minimum, int32 maximum)
{
	BSlider::SetLimits(minimum, maximum);
}


void
ColorSlider::LayoutInvalidated(bool descendants)
{
	BSlider::LayoutInvalidated(descendants);
}
