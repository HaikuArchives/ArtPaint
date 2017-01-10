/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <CheckBox.h>
#include <ClassInfo.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <StatusBar.h>
#include <string.h>
#include <Window.h>

#include "AddOns.h"
#include "ColorSeparator.h"
#include "ColorConverter.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = "ColorSeparator…";
	char menu_help_string[255] = "Starts adjusting the image saturation.";
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator* instantiate_add_on(BBitmap *bm,ManipulatorInformer *i)
{
	delete i;
	return new ColorSeparatorManipulator(bm);
}



ColorSeparatorManipulator::ColorSeparatorManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;

	SetPreviewBitmap(bm);
}


ColorSeparatorManipulator::~ColorSeparatorManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap* ColorSeparatorManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	ColorSeparatorManipulatorSettings *new_settings = cast_as(set,ColorSeparatorManipulatorSettings);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (original == preview_bitmap) {
		if (*new_settings == previous_settings)
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	}
	else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(),B_RGB32,FALSE);
	}


	settings = *new_settings;

	separate_colors();

	return target_bitmap;
}

int32 ColorSeparatorManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	updated_region->Set(preview_bitmap->Bounds());

	target_bitmap = preview_bitmap;
	source_bitmap = copy_of_the_preview_bitmap;

	separate_colors();

	return 1;
}


void ColorSeparatorManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in ColorSeparatorManipulator.
}


void ColorSeparatorManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;

		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}
}


void ColorSeparatorManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target,source,bits_length);
	}
}


void ColorSeparatorManipulator::separate_colors()
{
	uint32 *source_bits = (uint32*)source_bitmap->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();

	int32 bits_length = source_bitmap->BitsLength()/4;

	ColorConverter converter;

	for (int32 i=0;i<bits_length;i++) {
		converter.SetColor(*source_bits++);
		cmyk_color cmyk = converter.ReturnColorAsCMYK();

		if (settings.mode == SHOW_CYAN) {
			cmyk.magenta = 0;
			cmyk.yellow = 0;
			cmyk.black = 0;
		}
		else if (settings.mode == SHOW_MAGENTA) {
			cmyk.cyan = 0;
			cmyk.yellow = 0;
			cmyk.black = 0;
		}
		else if (settings.mode == SHOW_YELLOW) {
			cmyk.magenta = 0;
			cmyk.cyan = 0;
			cmyk.black = 0;
		}
		else {
			cmyk.magenta = 0;
			cmyk.yellow = 0;
			cmyk.cyan = 0;
		}
		converter.SetColor(cmyk);

		*target_bits++ = converter.ReturnColorAsBGRA();
	}
}

BView* ColorSeparatorManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new ColorSeparatorManipulatorView(this,target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings* ColorSeparatorManipulator::ReturnSettings()
{
	return new ColorSeparatorManipulatorSettings(settings);
}

void ColorSeparatorManipulator::ChangeSettings(ManipulatorSettings *s)
{
	ColorSeparatorManipulatorSettings *new_settings;
	new_settings = cast_as(s,ColorSeparatorManipulatorSettings);
	if (new_settings != NULL) {
		settings = *new_settings;
	}
}

char* ColorSeparatorManipulator::ReturnName()
{
	return "ColorSeparator";
}

char* ColorSeparatorManipulator::ReturnHelpString()
{
	return "Use the slider to set the image saturation.";
}




// -------------------------------------
ColorSeparatorManipulatorView::ColorSeparatorManipulatorView(ColorSeparatorManipulator *manip,
		const BMessenger& t)
	: WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;

	BMenu *cmyk_menu = new BMenu("cmyk_menu");

	BMessage *message;
	message = new BMessage(MENU_ENTRY_CHANGED);
	message->AddInt32("value",SHOW_CYAN);
	cmyk_menu->AddItem(new BMenuItem("C",message));

	message = new BMessage(MENU_ENTRY_CHANGED);
	message->AddInt32("value",SHOW_MAGENTA);
	cmyk_menu->AddItem(new BMenuItem("M",message));

	message = new BMessage(MENU_ENTRY_CHANGED);
	message->AddInt32("value",SHOW_YELLOW);
	cmyk_menu->AddItem(new BMenuItem("Y",message));

	message = new BMessage(MENU_ENTRY_CHANGED);
	message->AddInt32("value",SHOW_BLACK);
	cmyk_menu->AddItem(new BMenuItem("K",message));

	cmyk_menu_field = new BMenuField(BRect(4,4,204,24),"cmyk_menu_field","CMYK",cmyk_menu);
	AddChild(cmyk_menu_field);
	cmyk_menu_field->ResizeToPreferred();

	ResizeTo(cmyk_menu_field->Bounds().Width()+8,cmyk_menu_field->Frame().bottom+4);
}


ColorSeparatorManipulatorView::~ColorSeparatorManipulatorView()
{
}


void ColorSeparatorManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	cmyk_menu_field->Menu()->SetTargetForItems(BMessenger(this));
}

void ColorSeparatorManipulatorView::AllAttached()
{
}

void ColorSeparatorManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MENU_ENTRY_CHANGED:
			int32 new_value;
				if (message->FindInt32("value",&new_value) == B_OK) {
				settings.mode = new_value;
				manipulator->ChangeSettings(&settings);
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				started_adjusting = TRUE;
			}
			break;

		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}

void ColorSeparatorManipulatorView::ChangeSettings(ManipulatorSettings *set)
{
	ColorSeparatorManipulatorSettings *new_settings = cast_as(set,ColorSeparatorManipulatorSettings);

	if (set != NULL) {
		settings = *new_settings;

		BWindow *window = Window();
		if (window != NULL) {
			window->Lock();
			window->Unlock();
		}
	}
}
