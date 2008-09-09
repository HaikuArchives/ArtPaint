/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <CheckBox.h>
#include <ClassInfo.h>
#include <File.h>
#include <math.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <stdlib.h>
#include <string.h>

#include "TextManipulator.h"
#include "UtilityClasses.h"
#include "HSPolygon.h"
#include "MessageConstants.h"
#include "StringServer.h"
#include "PaletteWindowClient.h"


#define PI M_PI

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
			settings.starting_point = settings.starting_point + point-origo;
			origo = point;
		}
		else
			origo = point;
	}
	else {
		if (first_click == FALSE) {
			float dy = point.y - settings.starting_point.y;
			float dx = point.x - settings.starting_point.x;
			float new_angle = atan2(dy,dx);
			new_angle = new_angle / PI *180;
			settings.font.SetRotation(-new_angle);
		}
		else
			origo = point;
	}


	if (config_view != NULL) {
		config_view->ChangeSettings(&settings);
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
	int32 rect_count = strlen(settings.text);
	printf("test0.5\n");
	rect_array = new BRect[rect_count];
	for (int32 i=0;i<rect_count;i++)
		rect_array[i] = BRect(0,0,0,0);

	escapement_delta *deltas = new escapement_delta[rect_count];
	settings.font.GetBoundingBoxesAsString(settings.text,rect_count,B_SCREEN_METRIC,deltas,rect_array);
	printf("test1\n");
	BRect rect ;//= rect_array[0];
	printf("test2, %d\n",rect_count);

	for (int32 i=0;i<rect_count;i++) {
		rect_array[i].OffsetBy(settings.starting_point);
		view->StrokeRect(rect_array[i],B_MIXED_COLORS);
		rect = rect | rect_array[i];
		rect_array[i].PrintToStream();
	}
	printf("test3\n");

//	delete[] rect_array;
//	delete[] deltas;
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
	TextManipulatorSettings *new_settings = cast_as(set, TextManipulatorSettings);

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
	float alpha = new_settings->font.Rotation()/180*PI;
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
	if (previous_settings == settings) {
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
	TextManipulatorSettings current_settings = settings;

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
		view->SetFont(&(settings.font));
		view->SetDrawingMode(B_OP_OVER);
		view->MovePenTo(current_settings.starting_point);
		font_height fHeight;
		current_settings.font.GetHeight(&fHeight);
		BPoint height_vector(0,0);
		height_vector.y = fHeight.ascent + fHeight.descent + fHeight.leading;
		float alpha = current_settings.font.Rotation()/180*PI;
		height_vector.x = -sin(-alpha)*height_vector.y;
		height_vector.y = cos(-alpha)*height_vector.y;
		int32 line_number = 0;

		int32 len = current_settings.text ? strlen(current_settings.text) : 0;
		for (int32 i = 0; i < len; ++i) {
			pen_location = view->PenLocation();
			if (settings.text[i] == '\n') {
				// Move to next line
				line_number++;
				view->MovePenTo(current_settings.starting_point+BPoint(height_vector.x*line_number,height_vector.y*line_number));
			}
			else if (settings.text[i] == '\t') {
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
//	delete[] line;
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
			if (preview_bitmap->Bounds().Contains(settings.starting_point) == FALSE) {
				settings.starting_point.x = preview_bitmap->Bounds().left + 20;
				settings.starting_point.y = preview_bitmap->Bounds().top + (preview_bitmap->Bounds().bottom - preview_bitmap->Bounds().top) / 2;;
			}
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}
}

void TextManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		uint32 *target_bits = (uint32*)preview_bitmap->Bits();
		uint32 *source_bits = (uint32*)copy_of_the_preview_bitmap->Bits();
		int32 bits_length = preview_bitmap->BitsLength()/4;

		for (int32 i=0;i<bits_length;i++)
			*target_bits++ = *source_bits++;
	}
}

ManipulatorSettings* TextManipulator::ReturnSettings()
{
	return new TextManipulatorSettings(settings);
}

BView* TextManipulator::MakeConfigurationView(BMessenger *target)
{
	config_view = new TextManipulatorView(BRect(0,0,0,0),this,target);
	config_view->ChangeSettings(&settings);
	return config_view;
}


void TextManipulator::ChangeSettings(ManipulatorSettings *s)
{
	TextManipulatorSettings *new_settings = cast_as(s,TextManipulatorSettings);
	if (new_settings != NULL)
		settings = *new_settings;
}

status_t TextManipulator::ReadSettings(BNode *node)
{
	BFile *file = cast_as(node,BFile);
	if (file != NULL) {
		int32 version;
		file->Read(&version,sizeof(int32));
		if (version == TEXT_SETTINGS_VERSION) {
			int32 length;
			file->Read(&length,sizeof(int32));
			settings.text_array_length = length;
			delete[] settings.text;
			delete[] settings.text_color_array;
			settings.text = new char[settings.text_array_length];
			settings.text_color_array = new rgb_color[settings.text_array_length];

			file->Read(settings.text,settings.text_array_length);
			file->Read(settings.text_color_array,settings.text_array_length*sizeof(int32));
			file->Read(&(settings.font),sizeof(BFont));
			file->Read(&settings.starting_point,sizeof(BPoint));
		}
	}

	return B_OK;
}


status_t TextManipulator::WriteSettings(BNode *node)
{
	BFile *file = cast_as(node,BFile);
	if (file != NULL) {
		int32 version = TEXT_SETTINGS_VERSION;
		file->Write(&version,sizeof(int32));
		file->Write(&settings.text_array_length,sizeof(int32));
		file->Write(settings.text,settings.text_array_length);
		file->Write(settings.text_color_array,settings.text_array_length*sizeof(int32));
		file->Write(&settings.font,sizeof(BFont));
		file->Write(&settings.starting_point,sizeof(BPoint));
	}

	return B_OK;
}



const char* TextManipulator::ReturnName()
{
	return StringServer::ReturnString(TEXT_TOOL_NAME_STRING);
}


const char* TextManipulator::ReturnHelpString()
{
	return StringServer::ReturnString(TEXT_TOOL_IN_USE_STRING);
}

TextManipulatorView::TextManipulatorView(BRect rect,TextManipulator *manip,BMessenger *t)
	: WindowGUIManipulatorView(rect)
{
	preview_started = FALSE;
	manipulator = manip;
	target = new BMessenger(*t);

	char string[256];
	sprintf(string,"%s:",StringServer::ReturnString(TEXT_STRING));
	float divider;

	BBox *text_view_box = new BBox(BRect(0,0,200,100),"text_box",B_FOLLOW_LEFT_RIGHT|B_FOLLOW_TOP);
	AddChild(text_view_box);
	BRect frame = text_view_box->Bounds();
	frame.InsetBy(2,2);
	text_view = new TextEditor(frame);
	text_view->SetMessage(new BMessage(TEXT_CHANGED));
	text_view_box->AddChild(text_view);
	text_view_box->MoveTo(4,4);

	font_menu = new BMenu("font_menu");
	int32 numFamilies = count_font_families();
	font_family *families = new font_family[numFamilies];
	for (int32 i=0;i<numFamilies;i++) {
		uint32 flags;
		get_font_family(i,&families[i],&flags);
	}
	typedef int (*FP) (const void*,const void*);
	qsort(families,numFamilies,sizeof(font_family),reinterpret_cast<FP>(&strcmp));

	BFont a_font;
	char family_and_style_name[256];
	for ( int32 i = 0; i < numFamilies; i++ ) {
		a_font.SetFamilyAndStyle(families[i],NULL);
		int32 numStyles = count_font_styles(families[i]);
		if (numStyles > 1) {
			BMenu *sub_menu = new BMenu(families[i]);
			for ( int32 j = 0; j < numStyles; j++ ) {
				font_style style;
				uint32 flags;
				if (get_font_style(families[i], j, &style, &flags) == B_OK) {
					a_font.SetFamilyAndStyle(families[i],style);
					BMessage *font_message = new BMessage(FONT_STYLE_CHANGED);
					font_message->AddInt32("font_code",(int32)a_font.FamilyAndStyle());
					sub_menu->AddItem(new BMenuItem(style,font_message));
				}
			}
			BMenuItem *controlling_item = new BMenuItem(sub_menu);
			font_menu->AddItem(controlling_item);
		}
		else {
			font_style style;
			uint32 flags;
			if (get_font_style(families[i], 0, &style, &flags) == B_OK) {
				sprintf(family_and_style_name,"%s %s",families[i],style);
				a_font.SetFamilyAndStyle(NULL,style);
				BMessage *font_message = new BMessage(FONT_STYLE_CHANGED);
				font_message->AddInt32("font_code",a_font.FamilyAndStyle());
				font_menu->AddItem(new BMenuItem(family_and_style_name,font_message));
			}
		}
	}
	frame = text_view_box->Frame();
	frame.OffsetBy(0,frame.Height()+4);
	frame.bottom = frame.top;
	sprintf(string,"%s:",StringServer::ReturnString(FONT_STRING));
	font_menu_field = new BMenuField(frame,"menu_field",string,font_menu);
//	font_menu_field->SetDivider(divider);
	AddChild(font_menu_field);
	font_menu_field->ResizeToPreferred();
	font_menu_field->SetDivider(font_menu_field->StringWidth(string)+5);
	frame = text_view_box->Frame();
	frame.OffsetBy(0,frame.Height()+4);

	BMessage *message = new BMessage(FONT_SIZE_CHANGED);
	sprintf(string,"%s",StringServer::ReturnString(SIZE_STRING));
	message->AddBool("final",FALSE);
	message->AddInt32("value",0);
	size_slider = new ControlSliderBox(frame,"size_slider",string,"0",message,5,500);
	AddChild(size_slider);
	size_slider->MoveBy(0,size_slider->Frame().Height());	// This is because font_menu_field seems to be of height 0 at this point.

	message = new BMessage(FONT_ROTATION_CHANGED);
	message->AddBool("final",FALSE);
	message->AddInt32("value",0);
	frame = size_slider->Frame();
	frame.OffsetBy(0,frame.Height()+4);
	sprintf(string,"%s",StringServer::ReturnString(ROTATION_STRING));
	rotation_slider = new ControlSliderBox(frame,"rotation_slider",string,"0",message,-180,180);
	rotation_slider->ResizeToPreferred();
	AddChild(rotation_slider);

	message = new BMessage(FONT_SHEAR_CHANGED);
	message->AddBool("final",FALSE);
	message->AddInt32("value",0);
	frame = rotation_slider->Frame();
	frame.OffsetBy(0,frame.Height()+4);
	sprintf(string,"%s",StringServer::ReturnString(SHEAR_STRING));
	shear_slider = new ControlSliderBox(frame,"shear_slider",string,"45",message,45,135);
	shear_slider->ResizeToPreferred();
	AddChild(shear_slider);

	divider = max_c(rotation_slider->Divider(),max_c(size_slider->Divider(),shear_slider->Divider()));
	size_slider->SetDivider(divider);
	rotation_slider->SetDivider(divider);
	shear_slider->SetDivider(divider);


	frame = shear_slider->Frame();
	frame.OffsetBy(0,frame.Height()+4);

	sprintf(string,"%s",StringServer::ReturnString(ENABLE_ANTI_ALIASING_STRING));
	anti_aliasing_box = new BCheckBox(frame,"anti_aliasing_box",string,new BMessage(FONT_ANTI_ALIAS_CHANGED));
	anti_aliasing_box->ResizeToPreferred();
	AddChild(anti_aliasing_box);

	ResizeTo(shear_slider->Frame().Width()+8,anti_aliasing_box->Frame().bottom+4);
}


void TextManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();

	font_menu->SetTargetForItems(BMessenger(this));

	for (int32 i=0;i<font_menu->CountItems();i++) {
		BMenu *sub_menu = font_menu->SubmenuAt(i);
		if (sub_menu != NULL) {
			sub_menu->SetTargetForItems(this);
		}
	}

	size_slider->SetTarget(new BMessenger(this));
	rotation_slider->SetTarget(new BMessenger(this));
	shear_slider->SetTarget(new BMessenger(this));
	anti_aliasing_box->SetTarget(this);
}


void TextManipulatorView::AllAttached()
{
	WindowGUIManipulatorView::AllAttached();

	text_view->MakeFocus(true);
	text_view->SetText(settings.text);
	text_view->SetTarget(BMessenger(this,Window()));
	int32 length = settings.text ? strlen(settings.text) : 0;
	for (int32 i = 0; i < length; ++i) {
		text_view->SetFontAndColor(i, i + 1, NULL, B_FONT_ALL,
			&settings.text_color_array[i]);
	}

	size_slider->setValue(int32(settings.font.Size()));
	rotation_slider->setValue(int32(settings.font.Rotation()));
	shear_slider->setValue(int32(settings.font.Shear()));

	FontFamilyAndStyleChanged(settings.font.FamilyAndStyle());

	if (settings.font.Flags() & B_DISABLE_ANTIALIASING)
		anti_aliasing_box->SetValue(B_CONTROL_OFF);
	else
		anti_aliasing_box->SetValue(B_CONTROL_ON);
}


void TextManipulatorView::MessageReceived(BMessage *message)
{
	uint32 font_code;
	bool final;
	int32 value;
	BFont f;

	switch (message->what) {
		case TEXT_CHANGED:
			if (text_view->TextLength() > settings.text_array_length) {
				settings.text_array_length = 2*text_view->TextLength();
				delete[] settings.text;
				delete[] settings.text_color_array;
				settings.text = new char[settings.text_array_length];
				settings.text_color_array = new rgb_color[settings.text_array_length];
			}
			strcpy(settings.text,text_view->Text());
			for (int32 i=0;i<text_view->TextLength();i++) {
				text_view->GetFontAndColor(i,&f,&settings.text_color_array[i]);
			}
			manipulator->ChangeSettings(&settings);
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
			break;


		case FONT_STYLE_CHANGED:
			if (message->FindInt32("font_code",(int32*)&font_code) == B_OK) {
				font_family family;
				font_style style;
				settings.font.SetFamilyAndStyle(font_code);
				manipulator->ChangeSettings(&settings);
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				settings.font.GetFamilyAndStyle(&family,&style);
				FontFamilyAndStyleChanged(font_code);
			}
			break;



		case FONT_SHEAR_CHANGED:
			if ((message->FindBool("final",&final) == B_OK) && (message->FindInt32("value",&value) == B_OK)) {
				settings.font.SetShear(value);
				manipulator->ChangeSettings(&settings);
				if (final == FALSE) {
					if (preview_started == FALSE) {
						preview_started = TRUE;
						target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
					}
				}
				else {
					preview_started = FALSE;
					target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				}
			}
			break;

		case FONT_ROTATION_CHANGED:
			if ((message->FindBool("final",&final) == B_OK) && (message->FindInt32("value",&value) == B_OK)) {
				settings.font.SetRotation(value);
				manipulator->ChangeSettings(&settings);
				if (final == FALSE) {
					if (preview_started == FALSE) {
						preview_started = TRUE;
						target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
					}
				}
				else {
					preview_started = FALSE;
					target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				}
			}
			break;

		case FONT_SIZE_CHANGED:
			if ((message->FindBool("final",&final) == B_OK) && (message->FindInt32("value",&value) == B_OK)) {
				settings.font.SetSize(value);
				manipulator->ChangeSettings(&settings);
				if (final == FALSE) {
					if (preview_started == FALSE) {
						preview_started = TRUE;
						target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
					}
				}
				else {
					preview_started = FALSE;
					target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				}
			}
			break;

		case FONT_ANTI_ALIAS_CHANGED:
			{
				bool anti_alias = anti_aliasing_box->Value() == B_CONTROL_ON;
				if (anti_alias == TRUE) {
					settings.font.SetFlags(settings.font.Flags() & ~B_DISABLE_ANTIALIASING);
				}
				else {
					settings.font.SetFlags(settings.font.Flags() | B_DISABLE_ANTIALIASING);
				}
				manipulator->ChangeSettings(&settings);
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
				break;
			}
		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}



void TextManipulatorView::FontFamilyAndStyleChanged(uint32 font_code)
{
	BFont a_font;
	a_font.SetFamilyAndStyle(font_code);

	font_family family;
	font_style style;
	a_font.GetFamilyAndStyle(&family,&style);
	char family_and_style_name[256];
	sprintf(family_and_style_name,"%s %s",family,style);

	BMenuItem *item = font_menu->FindMarked();

	if (item != NULL) {
		item->SetMarked(FALSE);
		BMenu *sub_menu = item->Submenu();
		if (sub_menu != NULL) {
			item = sub_menu->FindMarked();
			if (item != NULL) {
				item->SetMarked(FALSE);
			}
		}
	}


	item = font_menu->FindItem(family);
	if (item == NULL)
		item = font_menu->FindItem(family_and_style_name);

	if (item != NULL) {
		item->SetMarked(TRUE);
		BMenu *sub_menu = item->Submenu();
		if (sub_menu != NULL) {
			item = sub_menu->FindItem(style);
			if (item != NULL) {
				item->SetMarked(TRUE);
			}
		}
	}

	BMenuItem *controlling_item = font_menu->Superitem();
	if (controlling_item != NULL)
		controlling_item->SetLabel(family_and_style_name);
}


void TextManipulatorView::ChangeSettings(TextManipulatorSettings *s)
{
	if (settings.font.FamilyAndStyle() != s->font.FamilyAndStyle())
		FontFamilyAndStyleChanged(s->font.FamilyAndStyle());

	if (Window() && Window()->Lock()) {
		if (strcmp(settings.text,s->text) != 0) {
			strcpy(settings.text,s->text);
			text_view->SetText(settings.text);

			int32 length = settings.text ? strlen(settings.text) : 0;
			for (int32 i = 0; i < length; ++i) {
				text_view->SetFontAndColor(i, i + 1, NULL, B_FONT_ALL,
					&settings.text_color_array[i]);
			}
		} else {
			;// Here we should set the text-colors if needed.
		}

		if (settings.font.Size() != s->font.Size())
			size_slider->setValue(int32(s->font.Size()));

		if (settings.font.Shear() != s->font.Shear())
			shear_slider->setValue(int32(s->font.Shear()));

		if (settings.font.Rotation() != s->font.Rotation())
			rotation_slider->setValue(int32(s->font.Rotation()));

		if (settings.font.Flags() != s->font.Flags()) {
			if (settings.font.Flags() & B_DISABLE_ANTIALIASING)
				anti_aliasing_box->SetValue(B_CONTROL_OFF);
			else
				anti_aliasing_box->SetValue(B_CONTROL_ON);
		}
		Window()->Unlock();
	}

	settings = *s;
}


// #pragma mark - TextEditor


TextEditor::TextEditor(BRect rect)
	: BTextView(rect, "text_view", BRect(0.0, 0.0, rect.Width(), rect.Height()),
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW),
	  PaletteWindowClient(),
	  fMessage(NULL)
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


TextManipulatorSettings::TextManipulatorSettings(const TextManipulatorSettings &s)
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
	delete[] text;
	delete[] text_color_array;
}


bool TextManipulatorSettings::operator==(const TextManipulatorSettings &s)
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

bool TextManipulatorSettings::operator!=(const TextManipulatorSettings &s)
{
	return !(*this == s);
}

TextManipulatorSettings& TextManipulatorSettings::operator=(const TextManipulatorSettings &s)
{
	delete[] text;
	delete[] text_color_array;

	text_array_length = s.text_array_length;
	text = new char[text_array_length];
	text_color_array = new rgb_color[text_array_length];

	strcpy(text,s.text);

	for (int32 i=0;i<text_array_length;i++)
		text_color_array[i] = s.text_color_array[i];

	starting_point = s.starting_point;
	font = s.font;

	return *this;
}
