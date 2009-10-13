/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "TextManipulator.h"

#include "NumberSliderControl.h"
#include "HSPolygon.h"
#include "MessageConstants.h"
#include "Selection.h"
#include "StringServer.h"
#include "UtilityClasses.h"
#include "PaletteWindowClient.h"


#include <Bitmap.h>
#include <CheckBox.h>
#include <File.h>
#include <GridLayoutBuilder.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>


#include <math.h>
#include <stdlib.h>
#include <string.h>


#define TEXT_SETTINGS_VERSION	0x03


#define TEXT_CHANGED							'Txch'
#define	FONT_STYLE_CHANGED						'Fsch'
#define FONT_SIZE_CHANGED						'Fsic'
#define FONT_ROTATION_CHANGED					'Froc'
#define	FONT_SHEAR_CHANGED						'Fshc'
#define FONT_ANTI_ALIAS_CHANGED					'Faac'


using ArtPaint::Interface::NumberSliderControl;


TextManipulator::TextManipulator(BBitmap *bm)
	:	WindowGUIManipulator()
{
	preview_bitmap = NULL;
	copy_of_the_preview_bitmap = NULL;
	config_view = NULL;
	SetPreviewBitmap(bm);

	last_used_quality = 1;
	lowest_allowed_quality = 1; // This must be 1.
}

TextManipulator::~TextManipulator()
{
	delete copy_of_the_preview_bitmap;

	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}
}

void TextManipulator::MouseDown(BPoint point,uint32 buttons,BView*,bool first_click)
{
	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		if (first_click == FALSE) {
			fSettings.starting_point = fSettings.starting_point + point-origo;
			origo = point;
		}
		else
			origo = point;
	}
	else {
		if (first_click == FALSE) {
			float dy = point.y - fSettings.starting_point.y;
			float dx = point.x - fSettings.starting_point.x;
			float new_angle = atan2(dy,dx);
			new_angle = new_angle / M_PI *180;
			fSettings.font.SetRotation(-new_angle);
		}
		else
			origo = point;
	}


	if (config_view != NULL) {
		config_view->ChangeSettings(&fSettings);
	}
}


BRegion TextManipulator::Draw(BView *view,float)
{
//	BRect rect = CalculateBoundingBox(settings);
//	view->StrokeRect(rect,B_MIXED_COLORS);
//	BRegion region;
//	region.Include(rect);
//	rect.InsetBy(1,1);
//	region.Exclude(rect);
//	return region;
//	return BRegion();
/*	printf("test0\n");
	BRect *rect_array;
	int32 rect_count = strlen(fSettings.text);
	printf("test0.5\n");
	rect_array = new BRect[rect_count];
	for (int32 i=0;i<rect_count;i++)
		rect_array[i] = BRect(0,0,0,0);

	escapement_delta *deltas = new escapement_delta[rect_count];
	fSettings.font.GetBoundingBoxesAsString(fSettings.text,rect_count,B_SCREEN_METRIC,deltas,rect_array);
	printf("test1\n");
	BRect rect ;//= rect_array[0];
	printf("test2, %d\n",rect_count);

	for (int32 i=0;i<rect_count;i++) {
		rect_array[i].OffsetBy(fSettings.starting_point);
		view->StrokeRect(rect_array[i],B_MIXED_COLORS);
		rect = rect | rect_array[i];
		rect_array[i].PrintToStream();
	}
	printf("test3\n");

//	delete [] rect_array;
//	delete [] deltas;
	printf("test4\n");

	BRegion region;
	printf("test5\n");
	region.Set(rect);
	printf("test6\n");
	return region;
*/
	return BRegion();
}


BBitmap* TextManipulator::ManipulateBitmap(ManipulatorSettings *set,
	BBitmap *original, Selection *selection, BStatusBar*)
{
	TextManipulatorSettings *new_settings = dynamic_cast<TextManipulatorSettings*> (set);

	if (new_settings == NULL || original == NULL)
		return NULL;

	// If the original bitmap is the same as preview-bitmap
	// we should first reset the preview_bitmap to its original state.
	if (original == preview_bitmap) {
		BRect frame = previously_updated_region.Frame() & original->Bounds();
		int32 left = (int32)frame.left;
		int32 right = (int32)frame.right;
		int32 top = (int32)frame.top;
		int32 bottom = (int32)frame.bottom;

		uint32 *target_bits = (uint32*)preview_bitmap->Bits();
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 target_bpr = preview_bitmap->BytesPerRow()/4;
		int32 source_bpr = copy_of_the_preview_bitmap->BytesPerRow()/4;
		for (int32 y=top;y<=bottom;y++) {
			for (int32 x=left;x<=right;x++) {
				*(target_bits + x + y*target_bpr) = *(source_bits + x + y*source_bpr);
			}
		}
	}

	// new_bitmap should be deleted at the end of this function.
	// This was not done properly before and it caused quite a big memory-leak.
	BBitmap *new_bitmap = DuplicateBitmap(original,0,TRUE);
	BView *new_view = new BView(new_bitmap->Bounds(),"new_view",B_FOLLOW_NONE,B_WILL_DRAW);
	new_bitmap->AddChild(new_view);
	new_bitmap->Lock();
	new_view->SetFont(&(new_settings->font));
	new_view->SetDrawingMode(B_OP_OVER);
	new_view->MovePenTo(new_settings->starting_point);
	font_height fHeight;
	new_settings->font.GetHeight(&fHeight);
	BPoint height_vector(0,0);
	height_vector.y = fHeight.ascent + fHeight.descent + fHeight.leading;
	float alpha = new_settings->font.Rotation()/180*M_PI;
	height_vector.x = -sin(-alpha)*height_vector.y;
	height_vector.y = cos(-alpha)*height_vector.y;
	int32 line_number = 0;

	int32 length = new_settings->text ? strlen(new_settings->text) : 0;
	for (int32 i = 0; i < length; ++i) {
		if (new_settings->text[i] == '\n') {
			// Move to next line
			line_number++;
			new_view->MovePenTo(new_settings->starting_point +
				BPoint(height_vector.x * line_number, height_vector.y * line_number));
		} else if (new_settings->text[i] == '\t') {
			// Replace tabs with four spaces
			new_view->DrawChar(' ');
			new_view->DrawChar(' ');
			new_view->DrawChar(' ');
			new_view->DrawChar(' ');
		} else {
			// Draw the next character from the string with
			// correct color.
			new_view->SetHighColor(new_settings->text_color_array[i]);
			if ((new_settings->text[i] & 0x80) == 0x00)
				new_view->DrawChar(new_settings->text[i]);
			else {
				int32 length = 0;
				int32 j=0;
				while ((j<8) && (((new_settings->text[i] << j) & 0x80) != 0x00)) {
					j++;
					length++;
				}
				new_view->DrawString(&new_settings->text[i],length);
				i += (length - 1);
			}
		}
	}

	new_view->Sync();
	new_bitmap->Unlock();

	// Here copy the bits back to original.
	uint32 *target_bits = (uint32*)original->Bits();
	uint32 *source_bits = (uint32*)new_bitmap->Bits();
	int32 target_bpr = original->BytesPerRow()/4;
	int32 source_bpr = new_bitmap->BytesPerRow()/4;

	BRect bounds = selection->GetBoundingRect();
	int32 left = (int32)bounds.left;
	int32 right = (int32)bounds.right;
	int32 top = (int32)bounds.top;
	int32 bottom = (int32)bounds.bottom;

	for (int32 y=top;y<=bottom;y++) {
		for (int32 x=left;x<=right;x++) {
			if (selection->ContainsPoint(x,y))
				*(target_bits + x + y*target_bpr) = *(source_bits + x + y*source_bpr);
		}
	}

	// delete the new_bitmap
	delete new_bitmap;

	return original;
}



int32 TextManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	// First decide the resolution of the bitmap
	if (previous_settings == fSettings) {
		if (last_used_quality <= 1) {
			last_used_quality = 0;
			return DRAW_ONLY_GUI;
		}
		else
			last_used_quality = last_used_quality / 2;
	}
	else if (full_quality == TRUE)
		last_used_quality = 1;
	else
		last_used_quality = lowest_allowed_quality;


	BRect updated_rect;

	// This is very dangerous. The manipulator-view might be changing the
	// settings at this very moment. This might even lead to a crash.
	TextManipulatorSettings current_settings = fSettings;

	// First reset the preview_bitmap for the part that previous settings did draw.
	uint32 *preview_bits = (uint32*)preview_bitmap->Bits();
	uint32 *copy_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
	int32 preview_bpr = preview_bitmap->BytesPerRow()/4;
	int32 copy_bpr = copy_of_the_preview_bitmap->BytesPerRow()/4;

	if (previously_updated_region.Frame().IsValid() == TRUE) {
		BRegion frame_region;
		frame_region.Set(preview_bitmap->Bounds());
		previously_updated_region.IntersectWith(&frame_region);


		for (int32 i=0;i<previously_updated_region.CountRects();i++) {
			int32 left = (int32)previously_updated_region.RectAt(i).left;
			int32 top = (int32)previously_updated_region.RectAt(i).top;
			int32 right = (int32)previously_updated_region.RectAt(i).right;
			int32 bottom = (int32)previously_updated_region.RectAt(i).bottom;
			for (int32 y=top;y<=bottom;y++) {
				for (int32 x=left;x<=right;x++) {
					*(preview_bits + x + y*preview_bpr) = *(copy_bits + x + y*copy_bpr);
				}
			}
		}
	}

	if (previously_updated_region.Frame().IsValid())
		updated_rect = previously_updated_region.Frame();

	previously_updated_region.MakeEmpty();

	// Here draw the text in the copy_of_the_preview_bitmap.
	copy_of_the_preview_bitmap->Lock();
	BView *view = copy_of_the_preview_bitmap->ChildAt(0);

	if (view != NULL) {
		BRect character_bounds[4];
		escapement_delta deltas[4];
		BPoint pen_location;
		view->SetFont(&(fSettings.font));
		view->SetDrawingMode(B_OP_OVER);
		view->MovePenTo(current_settings.starting_point);
		font_height fHeight;
		current_settings.font.GetHeight(&fHeight);
		BPoint height_vector(0,0);
		height_vector.y = fHeight.ascent + fHeight.descent + fHeight.leading;
		float alpha = current_settings.font.Rotation()/180*M_PI;
		height_vector.x = -sin(-alpha)*height_vector.y;
		height_vector.y = cos(-alpha)*height_vector.y;
		int32 line_number = 0;

		int32 len = current_settings.text ? strlen(current_settings.text) : 0;
		for (int32 i = 0; i < len; ++i) {
			pen_location = view->PenLocation();
			if (fSettings.text[i] == '\n') {
				// Move to next line
				line_number++;
				view->MovePenTo(current_settings.starting_point+BPoint(height_vector.x*line_number,height_vector.y*line_number));
			}
			else if (fSettings.text[i] == '\t') {
				// Replace tabs with four spaces
				view->DrawChar(' ');
				view->DrawChar(' ');
				view->DrawChar(' ');
				view->DrawChar(' ');
			}
			else {
				// Draw the next character from the string with
				// correct color.
				view->SetHighColor(current_settings.text_color_array[i]);
				if ((current_settings.text[i] & 0x80) == 0x00)
					view->DrawChar(current_settings.text[i]);
				else {
//					int32 length = (current_settings.text[i] >> 8) + ((current_settings.text[i]>>7) & 0x01) +
//								((current_settings.text[i]>>6) & 0x01) + ((current_settings.text[i]>>7) & 0x01);
					int32 length = 0;
					int32 j=0;
					while ((j<8) && (((current_settings.text[i] << j) & 0x80) != 0x00)) {
						j++;
						length++;
					}
					view->DrawString(&current_settings.text[i],length);
					i += (length - 1);
				}
				current_settings.font.GetBoundingBoxesAsString(&(current_settings.text[i]),1,B_SCREEN_METRIC,deltas,character_bounds);
				character_bounds[0].OffsetBy(pen_location);
				character_bounds[0].top = floor(character_bounds[0].top);
				character_bounds[0].bottom = ceil(character_bounds[0].bottom);
				character_bounds[0].left = floor(character_bounds[0].left);
				character_bounds[0].right = ceil(character_bounds[0].right);

				if (updated_rect.IsValid())
					updated_rect = updated_rect | character_bounds[0];
				else
					updated_rect = character_bounds[0];

			}
		}
		view->Sync();
	}


	copy_of_the_preview_bitmap->Unlock();

	updated_rect = updated_rect & preview_bitmap->Bounds();
	updated_region->Set(updated_rect);
	previously_updated_region.Set(updated_rect);

	previous_settings = current_settings;
	// Then swap the bits in preview_bitmap and copy_of_the_preview_bitmap for
	// the part that the new settings decide. If a selection is active only pixels
	// that are inside the selection are updated to preview_bitmap.
	uint32 spare_value;
	if (selection->IsEmpty() == TRUE) {
		for (int32 i=0;i<updated_region->CountRects();i++) {
			preview_bits = (uint32*)preview_bitmap->Bits();
			copy_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
			int32 left = (int32)updated_region->RectAt(i).left;
			int32 right = (int32)updated_region->RectAt(i).right;
			int32 top = (int32)updated_region->RectAt(i).top;
			int32 bottom = (int32)updated_region->RectAt(i).bottom;

			preview_bits += top*preview_bpr;
			copy_bits += top*copy_bpr;
			for (int32 y=top;y<=bottom;y++) {
				preview_bits += left;
				copy_bits += left;
				for (int32 x=left;x<=right;x++) {
					spare_value = *copy_bits;
					*copy_bits++ = *preview_bits;
					*preview_bits++ = spare_value;
				}
				preview_bits += (preview_bpr - right - 1);
				copy_bits += (copy_bpr - right - 1);
			}
		}
	}
	else {
		for (int32 i=0;i<updated_region->CountRects();i++) {
			preview_bits = (uint32*)preview_bitmap->Bits();
			copy_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
			int32 left = (int32)updated_region->RectAt(i).left;
			int32 right = (int32)updated_region->RectAt(i).right;
			int32 top = (int32)updated_region->RectAt(i).top;
			int32 bottom = (int32)updated_region->RectAt(i).bottom;

			preview_bits += top*preview_bpr;
			copy_bits += top*copy_bpr;
			for (int32 y=top;y<=bottom;y++) {
				preview_bits += left;
				copy_bits += left;
				for (int32 x=left;x<=right;x++) {
					if (selection->ContainsPoint(x,y)) {
						spare_value = *copy_bits;
						*copy_bits++ = *preview_bits;
						*preview_bits++ = spare_value;
					}
					else {
						*copy_bits++ = *preview_bits++;
					}
				}
				preview_bits += (preview_bpr - right - 1);
				copy_bits += (copy_bpr - right - 1);
			}
		}
	}

	return last_used_quality;
}

//BRect TextManipulator::CalculateBoundingBox(const TextManipulatorSettings &s)
//{
////	BRect b_box;
////	font_height fHeight;
////	s.font.GetHeight(&fHeight);
////
////	// Calculate the new bounding rect
////	b_box.left = s.starting_point.x-10;
////	b_box.top = s.starting_point.y - ceil(fHeight.ascent+fHeight.leading);
////	b_box.bottom = s.starting_point.y + ceil(fHeight.descent+fHeight.leading);
////	b_box.right = s.starting_point.x + s.font.StringWidth(s.text)+10;
////
////	// Here take the shear into account.
////	float shear = s.font.Shear();
////	shear = shear-90;
////	if (shear < 0)
////		b_box.left += shear/45.0*fHeight.ascent;
////	else
////		b_box.right += shear/45.0*fHeight.ascent;
////
////	// Here take the rotation into account.
////	HSPolygon *poly;
////	BPoint point_list[4];
////	point_list[0] = b_box.LeftTop();
////	point_list[1] = b_box.RightTop();
////	point_list[2] = b_box.RightBottom();
////	point_list[3] = b_box.LeftBottom();
////	poly = new HSPolygon(point_list,4);
////	poly->Rotate(s.starting_point,-s.font.Rotation());
////	b_box = poly->BoundingBox();
////	delete poly;
////
////	b_box.left = floor(b_box.left);
////	b_box.right = ceil(b_box.right);
////	b_box.top = floor(b_box.top);
////	b_box.bottom = ceil(b_box.bottom);
////
////	return b_box;
//
//
//	BRect rect(0,0,0,0);
//
//	font_height fHeight;
//	s.font.GetHeight(&fHeight);
//	float line_height = ceil(fHeight.ascent + fHeight.descent + fHeight.leading);
//	float line_length = 0;
//	edge_info edge[1];
//
//	rect.bottom += line_height;
//	BFont font = s.font;
//	font.SetRotation(0);
//	int32 line_count = 0;
//	int32 char_number = 0;
//	int32 i=0;
//	int32 text_length = strlen(s.text);
//	char *line = new char[text_length+1];
//	while (i<text_length) {
//		int32 j=0;
//		int32 number_of_additional_spaces = 0;
//		while ((i<text_length) && (s.text[i] != '\n')) {
//			if (s.text[i] == '\t')
//				number_of_additional_spaces += 3;
//
//			line[j++] = s.text[i++];
//		}
//		i++;
//		line[j] = '\0';
//		line_count++;
//
//		if (j>0) {
//			float width = font.StringWidth(line);
//			for (int32 space=0;space<number_of_additional_spaces;space++)
//				width += font.StringWidth(" ");
//
//			font.GetEdges(line,1,edge);
//			rect.left = min_c(rect.left,edge[0].left * font.Size());
//
//			font.GetEdges(line+j-1,1,edge);
//			rect.right = max_c(rect.right,max_c(width,width+edge[0].right*font.Size()));
//		}
//	}
//	delete [] line;
//	rect.bottom = line_count*line_height;
//
//	rect.OffsetBy(0,-fHeight.ascent);
//	rect.OffsetBy(s.starting_point);
//
//	HSPolygon *poly;
//	BPoint point_list[4];
//	point_list[0] = rect.LeftTop();
//	point_list[1] = rect.RightTop();
//	point_list[2] = rect.RightBottom();
//	point_list[3] = rect.LeftBottom();
//	poly = new HSPolygon(point_list,4);
//	poly->Rotate(s.starting_point,-s.font.Rotation());
//	rect = poly->BoundingBox();
//	delete poly;
//
//	return rect;
//}


void TextManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (bm != preview_bitmap) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0,TRUE);
			BView *a_view = new BView(copy_of_the_preview_bitmap->Bounds(),"a_view",B_FOLLOW_NONE,B_WILL_DRAW);
			copy_of_the_preview_bitmap->AddChild(a_view);
			if (preview_bitmap->Bounds().Contains(fSettings.starting_point) == FALSE) {
				fSettings.starting_point.x = preview_bitmap->Bounds().left + 20;
				fSettings.starting_point.y = preview_bitmap->Bounds().top + (preview_bitmap->Bounds().bottom - preview_bitmap->Bounds().top) / 2;;
			}
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}
}


void
TextManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap) {
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i = 0; i < bits_length; ++i)
			*target_bits++ = *source_bits++;
	}
}


ManipulatorSettings*
TextManipulator::ReturnSettings()
{
	return new (std::nothrow) TextManipulatorSettings(fSettings);
}


BView*
TextManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new (std::nothrow) TextManipulatorView(this, target);
	if (config_view)
		config_view->ChangeSettings(&fSettings);
	return config_view;
}


void
TextManipulator::ChangeSettings(ManipulatorSettings* settings)
{
	TextManipulatorSettings* newSettings =
		dynamic_cast<TextManipulatorSettings*> (settings);
	if (newSettings)
		fSettings = *newSettings;
}


status_t
TextManipulator::Save(BMessage& settings) const
{
	settings.MakeEmpty();

	status_t status = settings.AddString("text", fSettings.text);
	status |= settings.AddPoint("starting_point", fSettings.starting_point);
	status |= settings.AddInt32("text_array_length", fSettings.text_array_length);
	status |= settings.AddData("font", B_RAW_TYPE, (const void*)&fSettings.font,
		sizeof(BFont));

	int32 length = fSettings.text ? strlen(fSettings.text) : 0;
	for (int32 i = 0; i < length; ++i) {
		status |= settings.AddData("text_color_array", B_RGB_COLOR_TYPE,
			(const void*)&fSettings.text_color_array[i], sizeof(rgb_color));
	}

	return status;
}


status_t
TextManipulator::Restore(const BMessage& settings)
{
	delete [] fSettings.text;
	delete [] fSettings.text_color_array;

	const char* dummy;
	status_t status = settings.FindString("text", &dummy);
	status |= settings.FindPoint("starting_point", &fSettings.starting_point);
	status |= settings.FindInt32("text_array_length", &fSettings.text_array_length);

	fSettings.text = new char[fSettings.text_array_length];
	strcpy(fSettings.text, dummy);

	const void* data;
	ssize_t dataSize;
	status |= settings.FindData("font", B_RAW_TYPE, &data, &dataSize);
	if (status == B_OK && dataSize == sizeof(BFont))
		memcpy(&fSettings.font, data, sizeof(BFont));

	int32 i = 0;
	fSettings.text_color_array = new rgb_color[fSettings.text_array_length];
	while ((status |= settings.FindData("text_color_array", B_RGB_COLOR_TYPE, i,
		&data, &dataSize)) == B_OK) {
		if (dataSize == sizeof(rgb_color))
			memcpy(&fSettings.text_color_array[i], data, sizeof(rgb_color));
		i++;
	}

	return status;
}


const char*
TextManipulator::ReturnName()
{
	return StringServer::ReturnString(TEXT_TOOL_NAME_STRING);
}


const char*
TextManipulator::ReturnHelpString()
{
	return StringServer::ReturnString(TEXT_TOOL_IN_USE_STRING);
}


// #pragma mark -- TextManipulatorView


TextManipulatorView::TextManipulatorView(TextManipulator* manipulator,
		const BMessenger& target)
	: WindowGUIManipulatorView()
	, fTarget(target)
	, fTracking(false)
	, fManipulator(manipulator)
{
	fTextView = new TextEditor();
	fTextView->SetMessage(new BMessage(TEXT_CHANGED));
	fTextView->SetExplicitMinSize(BSize(250.0, 100.0));

	fFontMenu = new BMenu("font menu");

	const int32 numFamilies = count_font_families();
	font_family* families = new font_family[numFamilies];
	for (int32 i = 0; i < numFamilies; ++i) {
		uint32 flags;
		get_font_family(i, &families[i], &flags);
	}

	typedef int (*FP) (const void*, const void*);
	qsort(families, numFamilies, sizeof(font_family), reinterpret_cast<FP>(&strcmp));

	char family_and_style_name[256];
	for (int32 i = 0; i < numFamilies; ++i) {
		BFont font;
		font.SetFamilyAndStyle(families[i], NULL);
		int32 numStyles = count_font_styles(families[i]);
		if (numStyles > 1) {
			BMenu* subMenu = new BMenu(families[i]);
			for (int32 j = 0; j < numStyles; ++j) {
				font_style style;
				uint32 flags;
				if (get_font_style(families[i], j, &style, &flags) == B_OK) {
					font.SetFamilyAndStyle(families[i],style);
					BMessage* message = new BMessage(FONT_STYLE_CHANGED);
					message->AddUInt32("font_code", font.FamilyAndStyle());
					subMenu->AddItem(new BMenuItem(style, message));
				}
			}
			BMenuItem *controlling_item = new BMenuItem(subMenu);
			fFontMenu->AddItem(controlling_item);
		} else {
			font_style style;
			uint32 flags;
			if (get_font_style(families[i], 0, &style, &flags) == B_OK) {
				sprintf(family_and_style_name, "%s %s", families[i], style);
				font.SetFamilyAndStyle(NULL, style);
				BMessage* message = new BMessage(FONT_STYLE_CHANGED);
				message->AddUInt32("font_code", font.FamilyAndStyle());
				fFontMenu->AddItem(new BMenuItem(family_and_style_name, message));
			}
		}
	}
	fFontMenuField = new BMenuField(StringServer::ReturnString(FONT_STRING),
		fFontMenu);

	BMessage *message = new BMessage(FONT_SIZE_CHANGED);
	message->AddBool("final", false);
	fSizeControl =
		new NumberSliderControl(StringServer::ReturnString(SIZE_STRING), "0",
		message, 5, 500, false);
	AddChild(fSizeControl);

	message = new BMessage(FONT_ROTATION_CHANGED);
	message->AddBool("final", false);
	fRotationControl =
		new NumberSliderControl(StringServer::ReturnString(ROTATION_STRING),
		"0", message, -180, 180, false);
	AddChild(fRotationControl);

	message = new BMessage(FONT_SHEAR_CHANGED);
	message->AddBool("final", false);
	fShearControl =
		new NumberSliderControl(StringServer::ReturnString(SHEAR_STRING),
		"45", message, 45, 135, false);
	AddChild(fShearControl);

	fAntiAliasing =
		new BCheckBox(StringServer::ReturnString(ENABLE_ANTI_ALIASING_STRING),
		new BMessage(FONT_ANTI_ALIAS_CHANGED));

	BGridLayout* layout = BGridLayoutBuilder(5.0, 5.0)
		.Add(fSizeControl->LabelLayoutItem(), 0, 0)
		.Add(fSizeControl->TextViewLayoutItem(), 1, 0)
		.Add(fSizeControl->Slider(), 2, 0)
		.Add(fRotationControl->LabelLayoutItem(), 0, 1)
		.Add(fRotationControl->TextViewLayoutItem(), 1, 1)
		.Add(fRotationControl->Slider(), 2, 1)
		.Add(fShearControl->LabelLayoutItem(), 0, 2)
		.Add(fShearControl->TextViewLayoutItem(), 1, 2)
		.Add(fShearControl->Slider(), 2, 2);
	layout->SetMaxColumnWidth(1, fTextView->StringWidth("1000"));

	SetLayout(new BGroupLayout(B_VERTICAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL, 5.0)
		.Add(new BBox(B_FANCY_BORDER, fTextView))
		.Add(fFontMenuField)
		.Add(layout->View())
		.Add(fAntiAliasing)
	);
}


void
TextManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

	fFontMenu->SetTargetForItems(BMessenger(this));
	for (int32 i = 0; i < fFontMenu->CountItems(); ++i) {
		if (BMenu* subMenu = fFontMenu->SubmenuAt(i))
			subMenu->SetTargetForItems(this);
	}

	fSizeControl->SetTarget(this);
	fRotationControl->SetTarget(this);
	fShearControl->SetTarget(this);
	fAntiAliasing->SetTarget(this);
}


void
TextManipulatorView::AllAttached()
{
	WindowGUIManipulatorView::AllAttached();

	fTextView->MakeFocus(true);
	fTextView->SetText(fSettings.text);
	fTextView->SetTarget(BMessenger(this, Window()));

	int32 length = fSettings.text ? strlen(fSettings.text) : 0;
	for (int32 i = 0; i < length; ++i) {
		fTextView->SetFontAndColor(i, i + 1, NULL, B_FONT_ALL,
			&fSettings.text_color_array[i]);
	}

	fSizeControl->SetValue(int32(fSettings.font.Size()));
	fRotationControl->SetValue(int32(fSettings.font.Rotation()));
	fShearControl->SetValue(int32(fSettings.font.Shear()));

	_FontFamilyAndStyleChanged(fSettings.font.FamilyAndStyle());

	fAntiAliasing->SetValue(B_CONTROL_ON);
	if (fSettings.font.Flags() & B_DISABLE_ANTIALIASING)
		fAntiAliasing->SetValue(B_CONTROL_OFF);
}


void
TextManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case TEXT_CHANGED: {
			const int32 textLength = fTextView->TextLength();
			if (textLength > fSettings.text_array_length) {
				fSettings.text_array_length = 2 * textLength;
				delete [] fSettings.text;
				delete [] fSettings.text_color_array;
				fSettings.text = new char[fSettings.text_array_length];
				fSettings.text_color_array = new rgb_color[fSettings.text_array_length];
			}
			strcpy(fSettings.text, fTextView->Text());

			for (int32 i = 0; i < textLength; ++i) {
				BFont font;
				fTextView->GetFontAndColor(i, &font,
					&fSettings.text_color_array[i]);
			}

			fManipulator->ChangeSettings(&fSettings);
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;

		case FONT_STYLE_CHANGED: {
			uint32 fontCode;
			if (message->FindUInt32("font_code", &fontCode) == B_OK) {
				fSettings.font.SetFamilyAndStyle(fontCode);
				fManipulator->ChangeSettings(&fSettings);
				fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);

				_FontFamilyAndStyleChanged(fontCode);
			}
		}	break;

		case FONT_SIZE_CHANGED: {
		case FONT_SHEAR_CHANGED:
		case FONT_ROTATION_CHANGED:
			bool final;
			int32 value;
			if ((message->FindBool("final", &final) == B_OK)
				&& (message->FindInt32("value", &value) == B_OK)) {
				const uint32 what = message->what;
				if (what == FONT_SIZE_CHANGED)
					fSettings.font.SetSize(value);

				if (what == FONT_SHEAR_CHANGED)
					fSettings.font.SetShear(value);

				if (what == FONT_ROTATION_CHANGED)
					fSettings.font.SetRotation(value);

				fManipulator->ChangeSettings(&fSettings);
				if (final == false) {
					if (!fTracking) {
						fTracking = true;
						fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
					}
				} else {
					fTracking = false;
					fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				}
			}
		}	break;

		case FONT_ANTI_ALIAS_CHANGED: {
			if (fAntiAliasing->Value() == B_CONTROL_ON)
				fSettings.font.SetFlags(fSettings.font.Flags() & ~B_DISABLE_ANTIALIASING);
			else
				fSettings.font.SetFlags(fSettings.font.Flags() | B_DISABLE_ANTIALIASING);
			fManipulator->ChangeSettings(&fSettings);
			fTarget.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		}	break;

		default: {
			WindowGUIManipulatorView::MessageReceived(message);
		}	break;
	}
}


void
TextManipulatorView::_FontFamilyAndStyleChanged(uint32 fontCode)
{
	BFont font;
	font.SetFamilyAndStyle(fontCode);

	font_family family;
	font_style style;
	font.GetFamilyAndStyle(&family, &style);

	char family_and_style_name[256];
	sprintf(family_and_style_name, "%s %s", family, style);

	if (BMenuItem* item = fFontMenu->FindMarked()) {
		item->SetMarked(false);
		if (BMenu* subMenu = item->Submenu()) {
			if ((item = subMenu->FindMarked()))
				item->SetMarked(false);
		}
	}

	BMenuItem* item = fFontMenu->FindItem(family);
	if (!item)
		item = fFontMenu->FindItem(family_and_style_name);

	if (item) {
		item->SetMarked(true);
		if (BMenu* subMenu = item->Submenu()) {
			if ((item = subMenu->FindItem(style)))
				item->SetMarked(true);
		}
	}

	if (BMenuItem* superItem = fFontMenu->Superitem())
		superItem->SetLabel(family_and_style_name);
}


void
TextManipulatorView::ChangeSettings(TextManipulatorSettings* s)
{
	if (fSettings.font.FamilyAndStyle() != s->font.FamilyAndStyle())
		_FontFamilyAndStyleChanged(s->font.FamilyAndStyle());

	if (Window() && Window()->Lock()) {
		if (strcmp(fSettings.text,s->text) != 0) {
			strcpy(fSettings.text, s->text);
			fTextView->SetText(fSettings.text);

			int32 length = fSettings.text ? strlen(fSettings.text) : 0;
			for (int32 i = 0; i < length; ++i) {
				fTextView->SetFontAndColor(i, i + 1, NULL, B_FONT_ALL,
					&fSettings.text_color_array[i]);
			}
		} else {
			;// Here we should set the text-colors if needed.
		}

		if (fSettings.font.Size() != s->font.Size())
			fSizeControl->SetValue(int32(s->font.Size()));

		if (fSettings.font.Shear() != s->font.Shear())
			fShearControl->SetValue(int32(s->font.Shear()));

		if (fSettings.font.Rotation() != s->font.Rotation())
			fRotationControl->SetValue(int32(s->font.Rotation()));

		if (fSettings.font.Flags() != s->font.Flags()) {
			if (fSettings.font.Flags() & B_DISABLE_ANTIALIASING)
				fAntiAliasing->SetValue(B_CONTROL_OFF);
			else
				fAntiAliasing->SetValue(B_CONTROL_ON);
		}
		Window()->Unlock();
	}

	fSettings = *s;
}


// #pragma mark - TextEditor


TextEditor::TextEditor()
	: BTextView("text view", B_WILL_DRAW)
	, PaletteWindowClient()
	, fMessage(NULL)
{
	SetStylable(true);
	SetWordWrap(false);
	SetTextRect(Bounds());
}


TextEditor::~TextEditor()
{
	delete fMessage;
	ColorPaletteWindow::RemovePaletteWindowClient(this);
}


void
TextEditor::InsertText(const char *text, int32 length, int32 offset,
	const text_run_array *runs)
{
	BTextView::InsertText(text,length,offset,runs);

	_SendMessage();
}


void
TextEditor::DeleteText(int32 start, int32 finish)
{
	BTextView::DeleteText(start,finish);

	_SendMessage();
}


void
TextEditor::PaletteColorChanged(const rgb_color& color)
{
	if (Window() && Window()->Lock()) {
		int32 start, finish;
		GetSelection(&start, &finish);

		if (start != finish)
			SetFontAndColor(NULL, B_FONT_ALL, &color);
		else
			SetFontAndColor(0, TextLength(), NULL, B_FONT_ALL, &color);

		_SendMessage();

		Window()->Unlock();
	}
}


void
TextEditor::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
}


void
TextEditor::SetTarget(const BMessenger& target)
{
	fTarget = target;
}


void
TextEditor::_SendMessage()
{
	fTarget.SendMessage(fMessage);
}


// #pragma mark - TextManipulatorSettings


TextManipulatorSettings::TextManipulatorSettings()
	: ManipulatorSettings()
{
	text_array_length = 256;
	text = new char[text_array_length];
	text_color_array = new rgb_color[text_array_length];
	strcpy(text,"Text!");

	rgb_color text_color;
	text_color.red = 0;
	text_color.blue = 0;
	text_color.green = 0;
	text_color.alpha = 255;
	for (int32 i=0;i<text_array_length;i++) {
		text_color_array[i] = text_color;
	}
	starting_point = BPoint(-1,-1);
	font.SetSize(25);
}


TextManipulatorSettings::TextManipulatorSettings(const TextManipulatorSettings& s)
{
	text_array_length = s.text_array_length;
	text = new char[text_array_length];
	text_color_array = new rgb_color[text_array_length];

	strcpy(text,s.text);

	for (int32 i=0;i<text_array_length;i++)
		text_color_array[i] = s.text_color_array[i];

	starting_point = s.starting_point;
	font = s.font;
}


TextManipulatorSettings::~TextManipulatorSettings()
{
	delete [] text;
	delete [] text_color_array;
}


bool
TextManipulatorSettings::operator==(const TextManipulatorSettings& s)
{
	bool same = TRUE;
	same = same && (strcmp(s.text,text)==0);
	same = same && (s.text_array_length == text_array_length);
	same = same && (s.starting_point == starting_point);
	same = same && (s.font == font);

	if (same == TRUE) {
		for (int32 i=0;i<min_c(s.text_array_length,text_array_length);i++) {
			same = same	&& (s.text_color_array[i].red == text_color_array[i].red)
				&& (s.text_color_array[i].green == text_color_array[i].green)
				&& (s.text_color_array[i].blue == text_color_array[i].blue)
				&& (s.text_color_array[i].alpha == text_color_array[i].alpha);
		}
	}

	return same;
}


bool
TextManipulatorSettings::operator!=(const TextManipulatorSettings& settings)
{
	return (*this != settings);
}


TextManipulatorSettings&
TextManipulatorSettings::operator=(const TextManipulatorSettings& settings)
{
	delete [] text;
	delete [] text_color_array;

	text_array_length = settings.text_array_length;
	text = new char[text_array_length];
	text_color_array = new rgb_color[text_array_length];

	strcpy(text, settings.text);

	for (int32 i=0;i<text_array_length;i++)
		text_color_array[i] = settings.text_color_array[i];

	starting_point = settings.starting_point;
	font = settings.font;

	return *this;
}
