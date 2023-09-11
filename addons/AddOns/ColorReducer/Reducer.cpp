/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Catalog.h>
#include <ClassInfo.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Window.h>
#include <stdio.h>
#include <string.h>

#include "AddOns.h"
#include "ManipulatorInformer.h"
#include "Reducer.h"
#include "Selection.h"
#include "color_mapper.h"
#include "palette_generator.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_ColorReducer"


#ifdef __cplusplus
extern "C" {
#endif
	char name[255] = B_TRANSLATE_MARK("Color reducer" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Reduces the number of used colors.");
	int32 add_on_api_version = ADD_ON_API_VERSION;
	add_on_types add_on_type = COLOR_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator*
instantiate_add_on(BBitmap* bm, ManipulatorInformer* i)
{
	delete i;
	return new ReducerManipulator(bm);
}


ReducerManipulator::ReducerManipulator(BBitmap* bm)
	:
	WindowGUIManipulator(),
	selection(NULL)
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;

	previous_settings.dither_mode = settings.dither_mode + 1;

	SetPreviewBitmap(bm);
}


ReducerManipulator::~ReducerManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap*
ReducerManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	ReducerManipulatorSettings* new_settings = cast_as(set, ReducerManipulatorSettings);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	BBitmap *source_bitmap, *target_bitmap;

	if (original == preview_bitmap) {
		if (*new_settings == previous_settings)
			return original;

		source_bitmap = copy_of_the_preview_bitmap;
		target_bitmap = original;
	} else {
		source_bitmap = original;
		target_bitmap = new BBitmap(original->Bounds(), B_RGB32, FALSE);
	}

	BScreen screen;
	const rgb_color* palette;
	if (new_settings->palette_mode == BEOS_PALETTE)
		palette = screen.ColorMap()->color_list;
	else if (new_settings->palette_mode == GLA_PALETTE)
		palette = gla_palette(source_bitmap, new_settings->palette_size);
	else
		palette = NULL;

	if (palette != NULL)
		do_dither(source_bitmap, target_bitmap, palette, new_settings->palette_size,
			new_settings->dither_mode);

	return target_bitmap;
}


int32
ReducerManipulator::PreviewBitmap(bool full_quality, BRegion* updated_region)
{
	if (settings == previous_settings)
		return 0;

	config_view->Window()->PostMessage(REDUCER_STARTED, config_view);

	previous_settings = settings;

	updated_region->Set(preview_bitmap->Bounds());

	BBitmap *source_bitmap, *target_bitmap;
	target_bitmap = preview_bitmap;
	source_bitmap = copy_of_the_preview_bitmap;

	BScreen screen;
	const rgb_color* palette;
	if (previous_settings.palette_mode == BEOS_PALETTE)
		palette = screen.ColorMap()->color_list;
	else if (previous_settings.palette_mode == GLA_PALETTE)
		palette = gla_palette(source_bitmap, previous_settings.palette_size);
	else
		palette = NULL;

	if (palette != NULL)
		do_dither(source_bitmap, target_bitmap, palette, previous_settings.palette_size,
			previous_settings.dither_mode);

	config_view->Window()->PostMessage(REDUCER_FINISHED, config_view);
	return 1;
}


void
ReducerManipulator::SetPreviewBitmap(BBitmap* bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm, 0);
		} else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}
}


void
ReducerManipulator::Reset()
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32* source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32* target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();

		memcpy(target, source, bits_length);
	}
}


BView*
ReducerManipulator::MakeConfigurationView(const BMessenger& target)
{
	if (config_view == NULL) {
		config_view = new ReducerManipulatorView(this, target);
		config_view->ChangeSettings(&settings);
	}

	return config_view;
}


ManipulatorSettings*
ReducerManipulator::ReturnSettings()
{
	return new ReducerManipulatorSettings(settings);
}


void
ReducerManipulator::ChangeSettings(ManipulatorSettings* s)
{
	ReducerManipulatorSettings* new_settings;
	new_settings = cast_as(s, ReducerManipulatorSettings);
	if (new_settings != NULL)
		settings = *new_settings;
}


const char*
ReducerManipulator::ReturnName()
{
	return B_TRANSLATE("Color reducer");
}


const char*
ReducerManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Reduces the number of used colors.");
}


void
ReducerManipulator::do_dither(
	BBitmap* source, BBitmap* target, const rgb_color* palette, int palette_size, int32 dither_mode)
{
	BBitmap* reduced_map;
	if (settings.dither_mode == NO_DITHER)
		reduced_map = nearest_color_mapper(source, palette, palette_size);
	else if (settings.dither_mode == PRESERVE_SOLIDS_DITHER)
		reduced_map = preserve_solids_fs_color_mapper(source, palette, palette_size);
	else if (settings.dither_mode == N_CANDIDATE_DITHER)
		reduced_map = n_candidate_color_mapper(source, palette, palette_size, 2);
	else
		reduced_map = floyd_steinberg_edd_color_mapper(source, palette, palette_size);

	uint8* reduced_bits = (uint8*)reduced_map->Bits();
	uint32 reduced_bpr = reduced_map->BytesPerRow();

	uint32* destination_bits = (uint32*)target->Bits();
	uint32 destination_bpr = target->BytesPerRow() / 4;

	uint32* source_bits = (uint32*)source->Bits();
	uint32 source_bpr = source->BytesPerRow() / 4;

	int32 width = target->Bounds().IntegerWidth();
	int32 height = target->Bounds().IntegerHeight();

	int32 reduced_padding = reduced_bpr - width - 1;
	int32 destination_padding = destination_bpr - width - 1;
	int32 source_padding = source_bpr - width - 1;

	// Use this union to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32, source_bgra32;

	for (int32 y = 0; y <= height; y++) {
		for (int32 x = 0; x <= width; x++) {
			rgb_color c = palette[*reduced_bits++];
			source_bgra32.word = *source_bits++;
			bgra32.bytes[0] = c.blue;
			bgra32.bytes[1] = c.green;
			bgra32.bytes[2] = c.red;
			bgra32.bytes[3] = source_bgra32.bytes[3];

			*destination_bits++ = bgra32.word;
		}
		destination_bits += destination_padding;
		reduced_bits += reduced_padding;
		source_bits += source_padding;
	}

	delete reduced_map;
}


ReducerManipulatorView::ReducerManipulatorView(ReducerManipulator* manip, const BMessenger& t)
	:
	WindowGUIManipulatorView()
{
	target = t;
	manipulator = manip;

	BMenu* dither_menu = new BPopUpMenu("SELECT");

	BMessage* message;
	message = new BMessage(DITHER_MODE_CHANGED);
	message->AddInt32("dither_mode", NO_DITHER);
	dither_menu->AddItem(new BMenuItem(B_TRANSLATE("No dithering"), message));

	message = new BMessage(DITHER_MODE_CHANGED);
	message->AddInt32("dither_mode", FLOYD_STEINBERG_EDD_DITHER);
	dither_menu->AddItem(new BMenuItem(B_TRANSLATE("Floyd-Steinberg EDD"), message));

	message = new BMessage(DITHER_MODE_CHANGED);
	message->AddInt32("dither_mode", PRESERVE_SOLIDS_DITHER);
	dither_menu->AddItem(new BMenuItem(B_TRANSLATE("Preserve solids FS"), message));

	message = new BMessage(DITHER_MODE_CHANGED);
	message->AddInt32("dither_mode", N_CANDIDATE_DITHER);
	dither_menu->AddItem(new BMenuItem(B_TRANSLATE("N-Candidate"), message));

	dither_mode_menu_field
		= new BMenuField("dither_mode_menu_field", B_TRANSLATE("Dither mode:"), dither_menu);

	BMenu* size_menu = new BPopUpMenu("SELECT"); // TODO: Find how initialized...

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 2);
	size_menu->AddItem(new BMenuItem("2", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 4);
	size_menu->AddItem(new BMenuItem("4", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 8);
	size_menu->AddItem(new BMenuItem("8", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 16);
	size_menu->AddItem(new BMenuItem("16", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 32);
	size_menu->AddItem(new BMenuItem("32", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 64);
	size_menu->AddItem(new BMenuItem("64", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 128);
	size_menu->AddItem(new BMenuItem("128", message));

	message = new BMessage(PALETTE_SIZE_CHANGED);
	message->AddInt32("palette_size", 256);
	size_menu->AddItem(new BMenuItem("256", message));

	palette_size_menu_field
		= new BMenuField("palette_size_menu_field", B_TRANSLATE("Palette size:"), size_menu);

	BMenu* mode_menu = new BPopUpMenu("SELECT");

	message = new BMessage(PALETTE_MODE_CHANGED);
	message->AddInt32("palette_mode", BEOS_PALETTE);
	mode_menu->AddItem(new BMenuItem(B_TRANSLATE("BeOS"), message));

	message = new BMessage(PALETTE_MODE_CHANGED);
	message->AddInt32("palette_mode", GLA_PALETTE);
	mode_menu->AddItem(new BMenuItem(B_TRANSLATE("Generalized Lloyd's Algorithm"), message));

	palette_mode_menu_field
		= new BMenuField("palette_mode_menu_field", B_TRANSLATE("Palette mode:"), mode_menu);

	dither_menu->ItemAt(settings.dither_mode)->SetMarked(true);
	size_menu->ItemAt(size_menu->CountItems() - 1)->SetMarked(true); // slight hack...
	mode_menu->ItemAt(settings.palette_mode)->SetMarked(true);

	busy = new BStringView("busy", B_TRANSLATE("Reducing in progress"));
	busy->SetHighColor(128, 0, 0);
	busy->SetViewColor(ViewColor());
	busy->Hide();

	BGridLayout* gridLayout = BLayoutBuilder::Grid<>(B_USE_DEFAULT_SPACING, B_USE_SMALL_SPACING)
		.Add(dither_mode_menu_field->CreateLabelLayoutItem(), 0, 0)
		.Add(dither_mode_menu_field->CreateMenuBarLayoutItem(), 1, 0)
		.Add(palette_size_menu_field->CreateLabelLayoutItem(), 0, 1)
		.Add(palette_size_menu_field->CreateMenuBarLayoutItem(), 1, 1)
		.Add(palette_mode_menu_field->CreateLabelLayoutItem(), 0, 2)
		.Add(palette_mode_menu_field->CreateMenuBarLayoutItem(), 1, 2);
	gridLayout->SetMinColumnWidth(1, StringWidth("-YES-THIS-IS-A-REALLY-LONG-STRING--"));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(gridLayout->View())
		.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.Add(busy)
			.AddGlue()
		.End()
		.SetInsets(B_USE_SMALL_INSETS)
	.End();
}


void
ReducerManipulatorView::AllAttached()
{
	dither_mode_menu_field->Menu()->SetTargetForItems(this);
	palette_size_menu_field->Menu()->SetTargetForItems(this);
	palette_mode_menu_field->Menu()->SetTargetForItems(this);
}


void
ReducerManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case DITHER_MODE_CHANGED:
		{
			int32 mode;
			if (message->FindInt32("dither_mode", &mode) == B_OK) {
				settings.dither_mode = mode;
				manipulator->ChangeSettings(&settings);
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			}
		} break;
		case PALETTE_SIZE_CHANGED:
		{
			int32 size;
			if (message->FindInt32("palette_size", &size) == B_OK) {
				settings.palette_size = size;
				manipulator->ChangeSettings(&settings);
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			}
		} break;
		case PALETTE_MODE_CHANGED:
		{
			int32 mode;
			if (message->FindInt32("palette_mode", &mode) == B_OK) {
				settings.palette_mode = mode;
				manipulator->ChangeSettings(&settings);
				target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			}
		} break;
		case REDUCER_STARTED:
		{
			busy->Show();
		} break;
		case REDUCER_FINISHED:
		{
			busy->Hide();
		} break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
ReducerManipulatorView::ChangeSettings(ManipulatorSettings* set)
{
	ReducerManipulatorSettings* new_settings = cast_as(set, ReducerManipulatorSettings);

	if (set != NULL) {
		settings = *new_settings;

		BWindow* window = Window();
		if (window != NULL) {
			window->Lock();
			window->Unlock();
		}
	}
}
