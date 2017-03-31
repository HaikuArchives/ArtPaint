/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Menu.h>
#include <MenuItem.h>
#include <stdio.h>

#include "Selection.h"
#include "ThresholdView.h"


ThresholdView::ThresholdView(BRect rect, BMessage *msg)
	:	BControl(rect,"threshold_view","Threshold",msg,
				B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW)
{
	histogramBitmap = NULL;
	for (int32 i=0;i<256;i++) {
		histogram[i] = 0;
	}

	isTracking = false;
	mode = HISTOGRAM_MODE_INTENSITY;
	histogramRect = BRect(4,4,259,103);
	histogramBitmap = new BBitmap(histogramRect,B_RGBA32);

	BMenu *a_menu = new BMenu("Based on:");
	a_menu->AddItem(new BMenuItem("Intensity",new BMessage(HISTOGRAM_MODE_INTENSITY)));
	a_menu->AddItem(new BMenuItem("Red",new BMessage(HISTOGRAM_MODE_RED)));
	a_menu->AddItem(new BMenuItem("Green",new BMessage(HISTOGRAM_MODE_GREEN)));
	a_menu->AddItem(new BMenuItem("Blue",new BMessage(HISTOGRAM_MODE_BLUE)));
	modeMenu = new BMenuField(BRect(0,0,0,0),"modeMenu","Mode",a_menu);

	AddChild(modeMenu);
	modeMenu->ResizeToPreferred();
	modeMenu->MoveTo(4,histogramRect.bottom+8);

	ResizeTo(histogramRect.right+4,modeMenu->Frame().bottom+34);
}


void ThresholdView::AttachedToWindow()
{
	if (Parent() != NULL) {
		SetViewColor(Parent()->ViewColor());
	}
	else {
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	modeMenu->Menu()->SetTargetForItems(this);
}

void ThresholdView::Draw(BRect)
{
	if (histogramBitmap != NULL) {
		DrawBitmap(histogramBitmap,histogramBitmap->Bounds(),histogramRect);
	}

	SetHighColor(255,0,0,255);

	StrokeLine(BPoint(histogramRect.left+threshold,histogramRect.top),
				BPoint(histogramRect.left+threshold,histogramRect.bottom));

	SetHighColor(0,0,0,255);

	StrokeLine(BPoint(Bounds().left,histogramRect.bottom+4),
				BPoint(Bounds().right,histogramRect.bottom+4));
}


status_t ThresholdView::Invoke(BMessage *msg)
{
	if (msg == NULL) {
		msg = Message();
	}
	if (msg != NULL) {
		if (msg->HasInt32("threshold") == false)
			msg->AddInt32("threshold",0);
		if (msg->HasInt32("mode") == false)
			msg->AddInt32("mode",0);

		msg->ReplaceInt32("threshold",threshold);
		msg->ReplaceInt32("mode",mode);
	}

	return BControl::Invoke(msg);
}

void ThresholdView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case HISTOGRAM_MODE_INTENSITY:
		case HISTOGRAM_MODE_RED:
		case HISTOGRAM_MODE_GREEN:
		case HISTOGRAM_MODE_BLUE:
			mode = msg->what;
			Invoke();
			CalculateHistogram();
			Draw(histogramRect);
			break;
		default:
			BControl::MessageReceived(msg);
			break;
	}
}
void ThresholdView::MouseDown(BPoint point)
{
	isTracking = true;
	threshold = (int32)min_c(255,max_c(0,point.x-histogramRect.left));
//	Invoke();
	Draw(histogramRect);
}


void ThresholdView::MouseMoved(BPoint point,uint32,const BMessage*)
{
	if (isTracking) {
		threshold = (int32)min_c(255,max_c(0,point.x-histogramRect.left));
//		Invoke();
		Draw(histogramRect);
	}
}


void ThresholdView::MouseUp(BPoint point)
{
	isTracking = false;
	threshold = (int32)min_c(255,max_c(0,point.x-histogramRect.left));
	Invoke();
	Draw(histogramRect);
}



void ThresholdView::SetBitmap(BBitmap *bitmap)
{
	analyzedBitmap = bitmap;
	CalculateHistogram();
}



void ThresholdView::CalculateHistogram()
{
	for (int32 i=0;i<256;i++) {
		histogram[i] = 0;
	}

	if (analyzedBitmap != NULL) {
		uint32 *bits = (uint32*)analyzedBitmap->Bits();
		int32 bits_length = analyzedBitmap->BitsLength()/4;

		union {
			uint8 bytes[4];
			uint32 word;
		} c;

		for (int32 i=0;i<bits_length;i++) {
			c.word = *bits++;
			if (mode == HISTOGRAM_MODE_INTENSITY) {
				histogram[(int32)(0.299*c.bytes[2] + 0.587*c.bytes[1] + 0.114*c.bytes[0])]++;
			}
			else if (mode == HISTOGRAM_MODE_RED) {
				histogram[c.bytes[2]]++;
			}
			else if (mode == HISTOGRAM_MODE_GREEN) {
				histogram[c.bytes[1]]++;
			}
			else if (mode == HISTOGRAM_MODE_BLUE) {
				histogram[c.bytes[0]]++;
			}

		}
	}


	if (histogramBitmap != NULL) {
		uint32 *bits = (uint32*)histogramBitmap->Bits();
		int32 bpr = histogramBitmap->BytesPerRow()/4;
		int32 bits_length = histogramBitmap->BitsLength()/4;

		union {
			uint8 bytes[4];
			uint32 word;
		} c;
		c.word = 0xFFFFFFFF;

		for (int32 i=0;i<bits_length;i++) {
			*bits++ = c.word;
		}
		bits = (uint32*)histogramBitmap->Bits();

		int32 max = 0;
		for (int32 i=0;i<256;i++)
			max = (int32)max_c(max,histogram[i]);

		if (max > 0) {
			int32 height = histogramBitmap->Bounds().IntegerHeight();
			c.word = 0x00000000;
			c.bytes[3] = 0xFF;
			for (int32 i=0;i<256;i++) {
				int32 top = height - (float)histogram[i]/(float)max * height;
				for (int32 y=top;y<=height;y++) {
					*(bits + y*bpr + i) = c.word;
				}
			}
		}
		else
			printf("Boo\n");
	}
}
