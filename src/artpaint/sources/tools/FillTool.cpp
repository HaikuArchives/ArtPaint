/* 

	Filename:	FillTool.cpp
	Contents:	FillTool-class definitions.	
	Author:		Heikki Suhonen
	
*/


#include <CheckBox.h>
#include <File.h>
#include <stdlib.h>

#include "ColorView.h"
#include "FillTool.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ColorPalette.h"
#include "StringServer.h"
#include "Cursors.h"

FillTool::FillTool()
	: DrawingTool(StringServer::ReturnString(FILL_TOOL_NAME_STRING),FILL_TOOL)
{
	// Set options here. The MODE_OPTION is used for determining if we do flood fill or some other type of fill.
	options = GRADIENT_ENABLED_OPTION | PREVIEW_ENABLED_OPTION | TOLERANCE_OPTION | MODE_OPTION;
	number_of_options = 4;
	binary_fill_map = NULL;
	
	SetOption(GRADIENT_ENABLED_OPTION,B_CONTROL_OFF);	// Initially disable the gradient.
	SetOption(PREVIEW_ENABLED_OPTION,B_CONTROL_ON);	// Initially enable the preview.
	SetOption(TOLERANCE_OPTION,0);
	SetOption(MODE_OPTION,B_CONTROL_ON);

	gradient_color1 = 0x00000000;
	gradient_color2 = 0xFFFFFFFF;
}


FillTool::~FillTool()
{

}


ToolScript* FillTool::UseTool(ImageView *view,uint32 buttons,BPoint point,BPoint view_point)
{
	ToolScript *the_script = new ToolScript(type,settings,((PaintApplication*)be_app)->GetColor(TRUE));
	the_script->AddPoint(point);
	Selection *selection = view->GetSelection();
	if (selection->IsEmpty() == TRUE) {
		if (settings.gradient_enabled == B_CONTROL_OFF) {
			// Do just a normal fill without gradient
			NormalFill(view,buttons,point);
		}	
		else {
			// Do the gradient.
			the_script->AddPoint(GradientFill(view,buttons,point,view_point));	
		}
	}
	else {
		if (settings.gradient_enabled == B_CONTROL_OFF) {
			// Do just a normal fill without gradient
			NormalFill(view,buttons,point,selection);
		}	
		else {
			// Do the gradient.
			the_script->AddPoint(GradientFill(view,buttons,point,view_point,selection));	
		}	
	}
	return the_script;
}


int32 FillTool::UseToolWithScript(ToolScript*,BBitmap*)
{
	return B_NO_ERROR;
}


BView* FillTool::makeConfigView()
{
	FillToolConfigView *target_view = new FillToolConfigView(BRect(0,0,150,0),this,gradient_color1,gradient_color2);

	return target_view;
}



status_t FillTool::NormalFill(ImageView *view,uint32 buttons,BPoint start,Selection *sel)
{
	// Get the necessary parameters
	BWindow *window = view->Window();
	if (window == NULL)
		return B_ERROR;

	uint32 tolerance = (uint32)((float)settings.tolerance/100.0 * 255);

	filled_bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer *drawer = new BitmapDrawer(filled_bitmap);
	BRect bitmap_bounds = filled_bitmap->Bounds();
	bitmap_bounds.OffsetTo(BPoint(0,0));
			
	// Get the color for the fill.	
	rgb_color c = ((PaintApplication*)be_app)->GetColor(TRUE);
	uint32 color = RGBColorToBGRA(c);
	
	// Get the old color.
	uint32 old_color = drawer->GetPixel(start);

	// If the old color is the same as new and the tolerance is 0, we should do nothing.
	if ( (old_color == color) && (tolerance == 0) ) {
		delete drawer;
		return B_OK;
	}

	// These are the edge coordinates of bitmap. It is still safe to
	// address a pixel at max_x,max_y or min_x,min_y.
	int32 min_x,min_y,max_x,max_y;
	min_x = (int32)bitmap_bounds.left;
	min_y = (int32)bitmap_bounds.top;
	max_x = (int32)bitmap_bounds.right;
	max_y = (int32)bitmap_bounds.bottom;

	if (bitmap_bounds.Contains(start) == TRUE) {
		if (settings.mode == B_CONTROL_ON) { 	// Do the flood fill
			// Create a stack for the points. This should be replaced by something
			// more reliable. Perhaps a STL-stack would be good...
			// This stack might overflow very easily. We should make it extendible
			// and probably give it a larger initial value.
				
			PointStack *stack = new PointStack(10000);
			stack->Push(start);
									
			// Here fill the area using drawer's SetPixel and GetPixel.
			// The algorithm uses 4-connected version of flood-fill.
			// The SetPixel and GetPixel functions are versions that 
			// do not check bounds so we have to be careful not to exceed
			// bitmap's bounds.
			if (tolerance != 0) {
				binary_fill_map = new BBitmap(filled_bitmap->Bounds(),B_MONOCHROME_1_BIT);
				// Clear the binary map.
				uchar *binary_bits = (uchar*)binary_fill_map->Bits();
				uint32 binary_bitslength = binary_fill_map->BitsLength();
				for (int32 i=0;i<binary_bitslength;i++)
					*binary_bits++ = 0x00;
			}		
			while (stack->IsEmpty() == FALSE) {
				BPoint span_start = stack->Pop();
				if ( (span_start.y == min_y) && (min_y != max_y) ) {
					// Only check the spans below this line
					CheckLowerSpans(span_start,drawer,*stack,min_x,max_x,color,old_color,tolerance,sel);
				}
				else if ( (span_start.y == max_y) && (min_y != max_y) ) {
					// Only check the spans above this line.
					CheckUpperSpans(span_start,drawer,*stack,min_x,max_x,color,old_color,tolerance,sel);
				}
				else if (min_y != max_y) {
					// Check the spans above and below this line.
					CheckBothSpans(span_start,drawer,*stack,min_x,max_x,color,old_color,tolerance,sel);
				}
				else {
					// The image is only one pixel high. Fill the only span.
					FillSpan(span_start,drawer,min_x,max_x,color,old_color,tolerance,sel);			
				}
			}		
			if (tolerance != 0) {
				delete binary_fill_map;
				binary_fill_map = NULL;
			}
			delete stack;
		}
		else {	// Fill all the pixels that are within the tolerance.
			if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
				for (int32 y=min_y;y<=max_y;y++) {
					for (int32 x=min_x;x<=max_x;x++) {
						if (compare_2_pixels_with_variance(old_color,drawer->GetPixel(x,y),tolerance)) {
							drawer->SetPixel(x,y,color);
						}			
					}
				}
			}
			else {
				for (int32 y=min_y;y<=max_y;y++) {
					for (int32 x=min_x;x<=max_x;x++) {
						if (sel->ContainsPoint(x,y) && compare_2_pixels_with_variance(old_color,drawer->GetPixel(x,y),tolerance)) {
							drawer->SetPixel(x,y,color);
						}			
					}
				}			
			}
		}
				
		last_updated_rect = filled_bitmap->Bounds();
		window->Lock();
		view->UpdateImage(last_updated_rect);
		view->Sync();
		window->Unlock();
	}	
	delete drawer;
	return B_OK;
}


void FillTool::CheckLowerSpans(BPoint span_start,BitmapDrawer *drawer,PointStack &stack,int32 min_x,int32 max_x,uint32 new_color,uint32 old_color,int32 tolerance,Selection *sel)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = FALSE;

	if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {	
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (drawer->GetPixel(x,y+1) != old_color) ) {
					inside_lower_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( drawer->GetPixel(start_x,y+1) == old_color );
			x = start_x + 1;
			while ( (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (drawer->GetPixel(x,y+1) != old_color) ) {
					inside_lower_span = FALSE;
				}
				x++;
			}
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
				drawer->SetPixel(x,y,new_color);
	//			*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					inside_lower_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),old_color,tolerance) );
			x = start_x + 1;
			while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
				drawer->SetPixel(x,y,new_color);
	//			*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					inside_lower_span = FALSE;
				}
				x++;
			}	
		}
	}
	else {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {	
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) && sel->ContainsPoint(x,y+1)) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && ((drawer->GetPixel(x,y+1) != old_color) || !sel->ContainsPoint(x,y+1))) {
					inside_lower_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( drawer->GetPixel(start_x,y+1) == old_color ) && sel->ContainsPoint(start_x,y+1);
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) && sel->ContainsPoint(x,y+1)) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && ((drawer->GetPixel(x,y+1) != old_color) || !sel->ContainsPoint(x,y+1))) {
					inside_lower_span = FALSE;
				}
				x++;
			}
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) && sel->ContainsPoint(x,y+1) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance) || !sel->ContainsPoint(x,y+1)) ) {
					inside_lower_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),old_color,tolerance) && sel->ContainsPoint(start_x,y+1));
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) && sel->ContainsPoint(x,y+1) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance) || !sel->ContainsPoint(x,y+1)) ) {
					inside_lower_span = FALSE;
				}
				x++;
			}	
		}
	}
}

void FillTool::CheckUpperSpans(BPoint span_start,BitmapDrawer *drawer,PointStack &stack,int32 min_x,int32 max_x,uint32 new_color,uint32 old_color,int32 tolerance,Selection *sel)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_upper_span = FALSE;

	if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {		
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (drawer->GetPixel(x,y-1) != old_color) ) {
					inside_upper_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_upper_span = ( drawer->GetPixel(start_x,y-1) == old_color );
			x = start_x + 1;
			while ( (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (drawer->GetPixel(x,y-1) != old_color) ) {
					inside_upper_span = FALSE;
				}
				x++;
			}		
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
	
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))  && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
				drawer->SetPixel(x,y,new_color);
	//			*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
					inside_upper_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1) , old_color,tolerance) );
			x = start_x + 1;
			while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
				drawer->SetPixel(x,y,new_color);
	//			*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
					inside_upper_span = FALSE;
				}
				x++;
			}	
		}
	}
	else {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {		
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) && sel->ContainsPoint(x,y-1) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && ((drawer->GetPixel(x,y-1) != old_color) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_upper_span = ( drawer->GetPixel(start_x,y-1) == old_color ) && sel->ContainsPoint(start_x,y-1);
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) && sel->ContainsPoint(x,y-1)) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && ((drawer->GetPixel(x,y-1) != old_color) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
				x++;
			}		
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
	
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))  && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) && sel->ContainsPoint(x,y-1) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1) , old_color,tolerance) && sel->ContainsPoint(start_x,y-1));
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) && sel->ContainsPoint(x,y-1) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
				x++;
			}	
		}
	}
}

void FillTool::CheckBothSpans(BPoint span_start,BitmapDrawer *drawer,PointStack &stack,int32 min_x,int32 max_x,uint32 new_color,uint32 old_color,int32 tolerance,Selection *sel)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = FALSE;
	bool inside_upper_span = FALSE;

	if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (drawer->GetPixel(x,y+1) != old_color) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (drawer->GetPixel(x,y-1) != old_color) ) {
					inside_upper_span = FALSE;
				}
		
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( drawer->GetPixel(start_x,y+1) == old_color );
			inside_upper_span = ( drawer->GetPixel(start_x,y-1) == old_color );	
			x = start_x + 1;
			while ( (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (drawer->GetPixel(x,y+1) != old_color) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (drawer->GetPixel(x,y-1) != old_color) ) {
					inside_upper_span = FALSE;
				}
				
				x++;
			}	
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
	
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
				drawer->SetPixel(x,y,new_color);
	//			*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
					inside_upper_span = FALSE;
				}
		
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),old_color,tolerance) );
			inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1), old_color,tolerance) );	
			x = start_x + 1;
			while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))  && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
				drawer->SetPixel(x,y,new_color);
	//			*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) ) {
					inside_upper_span = FALSE;
				}
				
				x++;
			}				
		}
	}
	else {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) && sel->ContainsPoint(x,y+1)) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && ((drawer->GetPixel(x,y+1) != old_color) || !sel->ContainsPoint(x,y+1)) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) && sel->ContainsPoint(x,y-1)) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && ((drawer->GetPixel(x,y-1) != old_color) || !sel->ContainsPoint(x,y-1))) {
					inside_upper_span = FALSE;
				}
		
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( drawer->GetPixel(start_x,y+1) == old_color ) && sel->ContainsPoint(x,y+1);
			inside_upper_span = ( drawer->GetPixel(start_x,y-1) == old_color ) && sel->ContainsPoint(x,y-1);	
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				
				if ( (inside_lower_span == FALSE) && (drawer->GetPixel(x,y+1) == old_color) && sel->ContainsPoint(x,y+1)) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && ((drawer->GetPixel(x,y+1) != old_color) || !sel->ContainsPoint(x,y+1)) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (drawer->GetPixel(x,y-1) == old_color) && sel->ContainsPoint(x,y-1)) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && ((drawer->GetPixel(x,y-1) != old_color) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
				
				x++;
			}	
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
	
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00)  ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) && sel->ContainsPoint(x,y+1) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance) || !sel->ContainsPoint(x,y+1)) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance)) && sel->ContainsPoint(x,y-1) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
		
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			// We might already be inside a lower span
			inside_lower_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y+1),old_color,tolerance) && sel->ContainsPoint(start_x,y+1));
			inside_upper_span = ( compare_2_pixels_with_variance(drawer->GetPixel(start_x,y-1),old_color,tolerance) && sel->ContainsPoint(start_x,y-1));	
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance))  && ((*(binary_bits + y*binary_bpr + (x/8))&(0x01 << (7-x%8))) == 0x00) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) |= (0x01 << (7 - x%8));
				
				if ( (inside_lower_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance)) && sel->ContainsPoint(x,y+1) ) {
					stack.Push(BPoint(x,y+1));
					inside_lower_span = TRUE;
				} 	
				else if ( (inside_lower_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y+1),old_color,tolerance) || !sel->ContainsPoint(x,y+1)) ) {
					inside_lower_span = FALSE;
				}
				
				if ( (inside_upper_span == FALSE) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y-1), old_color,tolerance)) && sel->ContainsPoint(x,y-1) ) {
					stack.Push(BPoint(x,y-1));
					inside_upper_span = TRUE;
				} 	
				else if ( (inside_upper_span == TRUE) && (!compare_2_pixels_with_variance(drawer->GetPixel(x,y-1),old_color,tolerance) || !sel->ContainsPoint(x,y-1)) ) {
					inside_upper_span = FALSE;
				}
				
				x++;
			}				
		}
	}
}


void FillTool::FillSpan(BPoint span_start,BitmapDrawer *drawer,int32 min_x, int32 max_x, uint32 new_color, uint32 old_color,int32 tolerance,Selection *sel)
{
	// First get the vital data.
	int32 x,start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;

	if ((sel == NULL) || (sel->IsEmpty() == TRUE)) {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			x = start_x + 1;
			while ( (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				x++;
			}	
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
	
			// Then go from start towards the left side of the bitmap.
			while ( (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			x = start_x + 1;
			while ( (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				x++;
			}	
		
		}
	}
	else {
		if ((tolerance == 0) && (binary_fill_map == NULL)) {
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (drawer->GetPixel(x,y) == old_color) ) {
				drawer->SetPixel(x,y,new_color);
				x++;
			}	
		}
		else {
			// This is the case that takes the variance into account. We must use a
			// binary bitmap to see what parts have already been filled.
			uint32 binary_bpr = binary_fill_map->BytesPerRow();
			uchar *binary_bits = (uchar*)binary_fill_map->Bits();	
	
			// Then go from start towards the left side of the bitmap.
			while (sel->ContainsPoint(x,y) && (x >= min_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y), old_color,tolerance)) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				x--;
			}
			
			// Then go from start_x+1 towards the right side of the bitmap.
			x = start_x + 1;
			while (sel->ContainsPoint(x,y) && (x <= max_x) && (compare_2_pixels_with_variance(drawer->GetPixel(x,y),old_color,tolerance)) ) {
				drawer->SetPixel(x,y,new_color);
				*(binary_bits + y*binary_bpr + (x/8)) = *(binary_bits + y*binary_bpr + (x/8)) | (0x01 << (7 - x%8));
				x++;
			}	
		
		}
	}
}


BPoint FillTool::GradientFill(ImageView *view,uint32 buttons,BPoint start,BPoint orig_view_point,Selection *sel)
{
	// First calculate points that are to be included in the fill to 
	// a separate binary mask. Then go through the filled areas bounds
	// rectangle and fill only those pixels that the mask tells to.
	// The color of pixel is to be calculated from the original mousedown point
	// and the last mousedown point and from color values for mouse-button
	// and gradien color. If preview is enabled we should also update in real time
	// whenever possible.
	
	// Get the necessary parameters
	// Wait for the last_updated_region to become empty
	while (last_updated_rect.IsValid() == TRUE)
		snooze(50 * 1000);

	BWindow *window = view->Window();
	if (window == NULL)
		return BPoint(-1,-1);
	
	BBitmap *bitmap = view->ReturnImage()->ReturnActiveBitmap();
	BitmapDrawer *drawer = new BitmapDrawer(bitmap);
	BRect bitmap_bounds = bitmap->Bounds();
	bitmap_bounds.OffsetTo(BPoint(0,0));
			
	window->Lock();
	drawing_mode old_mode = view->DrawingMode();
	view->SetDrawingMode(B_OP_INVERT);
	window->Unlock();	
	BPoint new_point = start;

	if (bitmap_bounds.Contains(start) == TRUE) {	
	
		// These are the edge coordinates of bitmap. It is still safe to
		// address a pixel at max_x,max_y or min_x,min_y.
		int32 min_x,min_y,max_x,max_y;
		min_x = (int32)bitmap_bounds.left;
		min_y = (int32)bitmap_bounds.top;
		max_x = (int32)bitmap_bounds.right;
		max_y = (int32)bitmap_bounds.bottom;
	
		// At this point we should take some action if min_x == max_x or min_y == max_y	

		// Get the color for the fill.	
//		rgb_color c = ((PaintApplication*)be_app)->getColor(buttons);
//		uint32 color = RGBColorToBGRA(c);
	
//		// Get the gradient color.
//		uint32 gradient_color = settings.gradient_color;
		// Get the old color.
		uint32 old_color = drawer->GetPixel(start);
	
		uint32 gradient_color = gradient_color1;
		uint32 color = gradient_color2;

		// Here calculate the binary bitmap for the purpose of doing the gradient.
		BBitmap *binary_map;
		if (settings.mode == B_CONTROL_OFF)	// Not flood-mode
			binary_map = MakeBinaryMap(drawer,min_x,max_x,min_y,max_y,old_color,sel);
		else	// Flood-mode
			binary_map = MakeFloodBinaryMap(drawer,min_x,max_x,min_y,max_y,old_color,start,sel);
		

		// Here calculate the bounding rectangle of the filled area and
		// change the min and max coordinates to those edges of the rect.
		BRect filled_area_bounds = calcBinaryMapBounds(binary_map);
//		min_x = filled_area_bounds.left;
//		max_x = filled_area_bounds.right;
//		min_y = filled_area_bounds.top;
//		max_y = filled_area_bounds.bottom;

		BRect ellipse_rect = BRect(orig_view_point-BPoint(3,3),orig_view_point+BPoint(3,3));		
		BPoint new_view_point = orig_view_point;
		BPoint prev_view_point = new_view_point;
		window->Lock();
		view->StrokeEllipse(ellipse_rect);
		window->Unlock();	
		if (settings.preview_enabled == B_CONTROL_OFF) {
			// Do not do the preview. Just read the coordinates.
			while (buttons) {
				if (new_view_point != prev_view_point) {
					window->Lock();
					view->StrokeEllipse(ellipse_rect);
					window->Unlock();					
					prev_view_point = new_view_point;
				}
				window->Lock();
				view->getCoords(&new_point,&buttons,&new_view_point);
				window->Unlock();
				snooze(20 * 1000);
			}
		}
		else {
			// Display also preview of the gradient
			while (buttons) {
				if (new_view_point != prev_view_point) {
					int32 dx = (int32)(start.x - new_point.x);
					int32 dy = (int32)(start.y - new_point.y);
					// There should actually be a separate function (and maybe even a thread) that calculates
					// the preview in real time for some bitmap that is quite small (about 200x200 pixels maximum).
					// Then the gradient would be copied to bitmap using some specialized function.
					FillGradientPreview(drawer,binary_map,dx,dy,min_x,max_x,min_y,max_y,color,gradient_color);
					view->ReturnImage()->RenderPreview(bitmap->Bounds(),2);
					window->Lock();
					view->Draw(view->convertBitmapRectToView(filled_area_bounds));
					view->StrokeEllipse(ellipse_rect);
					window->Unlock();					
					prev_view_point = new_view_point;
				}
				window->Lock();
				view->getCoords(&new_point,&buttons,&new_view_point);
				window->Unlock();
				snooze(20 * 1000);
			}
		}		
		// Here calculate the final gradient.
		int32 dx = (int32)(start.x - new_point.x);
		int32 dy = (int32)(start.y - new_point.y);
	
		FillGradient(drawer,binary_map,dx,dy,min_x,max_x,min_y,max_y,color,gradient_color);
	
		// Update the image-view.
		view->ReturnImage()->Render();
		window->Lock();
		view->SetDrawingMode(old_mode);
		view->Draw(view->convertBitmapRectToView(filled_area_bounds) | ellipse_rect);
		window->Unlock();
		delete binary_map;
		last_updated_rect = filled_area_bounds;	
	}
	delete drawer;
	return new_point;
}



BBitmap* FillTool::MakeBinaryMap(BitmapDrawer *drawer,int32 min_x,int32 max_x,int32 min_y,int32 max_y,uint32 old_color,Selection *sel)
{
	// This function makes a binary bitmap that has ones where the
	// color of original bitmap is same as old_color, and zeroes elsewhere.
	BBitmap *binary_map = new BBitmap(BRect(min_x,min_y,max_x,max_y),B_MONOCHROME_1_BIT);
	uchar *binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	if ((sel == NULL) || (sel->IsEmpty() == TRUE)) { 
		if (settings.tolerance == 0) {
			// Always collect eight pixels from the bitmap and then move that data to the binary bitmap.
			uchar next_value = 0x00;
			for (int32 y=min_y;y<=max_y;y++) {
				int32 bytes_advanced = 0;
				for (int32 x=min_x;x<=max_x;x++) {
					if ( ((x % 8) == 0) && (x != 0) ) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00; 	
					}		
					next_value |= ( (old_color == drawer->GetPixel(x,y)) ? ( 0x01 << (7 - (x%8) )) : 0x00 );
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00; 	
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		}
		else {
			// Always collect eight pixels from the bitmap and then move that data to the binary bitmap.
			uchar next_value = 0x00;
			uint32 tolerance = (uint32)((float)settings.tolerance/100.0 * 255);
			for (int32 y=min_y;y<=max_y;y++) {
				int32 bytes_advanced = 0;
				for (int32 x=min_x;x<=max_x;x++) {
					if ( ((x % 8) == 0) && (x != 0) ) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00; 	
					}		
					next_value |= ( (compare_2_pixels_with_variance(old_color,drawer->GetPixel(x,y),tolerance)) ? ( 0x01 << (7 - (x%8) )) : 0x00 );
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00; 	
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		}
	}
	else {
		if (settings.tolerance == 0) {
			// Always collect eight pixels from the bitmap and then move that data to the binary bitmap.
			uchar next_value = 0x00;
			for (int32 y=min_y;y<=max_y;y++) {
				int32 bytes_advanced = 0;
				for (int32 x=min_x;x<=max_x;x++) {
					if ( ((x % 8) == 0) && (x != 0) ) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00; 	
					}		
					next_value |= ( ((old_color == drawer->GetPixel(x,y)) && sel->ContainsPoint(x,y)) ? ( 0x01 << (7 - (x%8) )) : 0x00 );
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00; 	
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		}
		else {
			// Always collect eight pixels from the bitmap and then move that data to the binary bitmap.
			uchar next_value = 0x00;
			uint32 tolerance = (uint32)((float)settings.tolerance/100.0 * 255);
			for (int32 y=min_y;y<=max_y;y++) {
				int32 bytes_advanced = 0;
				for (int32 x=min_x;x<=max_x;x++) {
					if ( ((x % 8) == 0) && (x != 0) ) {
						*binary_bits++ = next_value;
						bytes_advanced++;
						next_value = 0x00; 	
					}		
					next_value |= ( (compare_2_pixels_with_variance(old_color,drawer->GetPixel(x,y),tolerance) && sel->ContainsPoint(x,y)) ? ( 0x01 << (7 - (x%8) )) : 0x00 );
				}
				if ((max_x % 8) != 0) {
					*binary_bits++ = next_value;
					bytes_advanced++;
					next_value = 0x00; 	
				}
				binary_bits += binary_bpr - bytes_advanced;
			}
		}
	}
	return binary_map;
}

BBitmap* FillTool::MakeFloodBinaryMap(BitmapDrawer *drawer,int32 min_x,int32 max_x,int32 min_y,int32 max_y,uint32 old_color, BPoint start,Selection *sel)
{
	// This function makes a binary bitmap of the image. It contains ones where the
	// flood fill should fill and zeroes elsewhere.
	BBitmap *binary_map;
	binary_map = binary_fill_map = new BBitmap(BRect(min_x,min_y,max_x,max_y),B_MONOCHROME_1_BIT);
	uchar *binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bitslength = binary_map->BitsLength();
	// Clear the binary map.
	for (int32 i=0;i<binary_bitslength;i++)
		*binary_bits++ = 0x00;

	binary_bits = (uchar*)binary_map->Bits();

	// We can use the functions CheckLowerSpans, CheckUpperSpans and CheckBothSpans
	// to calculate the binary_fill_map and then return it.

	// Here we proceed just like in the case of a normal fill, except that we do not
	// show the intermediate fill to the user.
	// Create a stack for the points. This should be replaced by something
	// more reliable. Perhaps a STL-stack would be good...
	// This stack might overflow very easily. We should make it extendible
	// and probably give it a larger initial value.
		
	PointStack *stack = new PointStack(10000);
	stack->Push(start);
				
	// Here fill the area using drawer's SetPixel and GetPixel.
	// The algorithm uses 4-connected version of flood-fill.
	// The SetPixel and GetPixel functions are versions that 
	// do not check bounds so we have to be careful not to exceed
	// bitmap's bounds.
	uint32 color = 0xFFFFFFFF;	// This is the temporary color that will be used
								// to fill the bitmap.
	uint32 tolerance = (uint32)((float)settings.tolerance/100.0 * 255);
	while (stack->IsEmpty() == FALSE) {
		BPoint span_start = stack->Pop();
		if ( (span_start.y == min_y) && (min_y != max_y) ) {
			// Only check the spans below this line
			CheckLowerSpans(span_start,drawer,*stack,min_x,max_x,color,old_color,tolerance,sel);
		}
		else if ( (span_start.y == max_y) && (min_y != max_y) ) {
			// Only check the spans above this line.
			CheckUpperSpans(span_start,drawer,*stack,min_x,max_x,color,old_color,tolerance,sel);
		}
		else if (min_y != max_y) {
			// Check the spans above and below this line.
			CheckBothSpans(span_start,drawer,*stack,min_x,max_x,color,old_color,tolerance,sel);
		}
		else {
			// The image is only one pixel high. Fill the only span.
			FillSpan(span_start,drawer,min_x,max_x,color,old_color,tolerance,sel);			
		}
	}		
	delete stack;
		
	// Remember to NULL the attribute binary_fill_map
	binary_fill_map = NULL;
	return binary_map;
}


void FillTool::FillGradient(BitmapDrawer *drawer,BBitmap *binary_map,int32 dx,int32 dy,int32 min_x,int32 max_x,int32 min_y,int32 max_y,uint32 new_color,uint32 gradient_color)
{
	uchar *binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	uint32 x_source,x_target;
	uint32 y_source,y_target;
	
	
	if (dx<0) {
		x_target = gradient_color;
	}
	else {
		x_target = new_color;
	}	
	if (dy<0) {
		y_target = gradient_color;
		y_source = new_color;
	}
	else {
		y_target = new_color;
		y_source = gradient_color;
	}
	uint32 y_gradient;
	uint32 x_gradient;

	// These might also be negative
	int16 x_red_diff,x_green_diff,x_blue_diff,x_alpha_diff;
	int16 y_red_diff,y_green_diff,y_blue_diff,y_alpha_diff;
	
	float x_accumulation;	
	float y_accumulation = 0;
	
	y_gradient = y_source;	
	uchar next_value = 0;

	y_red_diff = (int16)((y_target>>8) & 0xFF) - (int16)((y_source>>8) & 0xFF);	
	y_green_diff = (int16)((y_target>>16) & 0xFF) - (int16)((y_source>>16) & 0xFF);
	y_blue_diff = (int16)((y_target>>24) & 0xFF) - (int16)((y_source>>24) & 0xFF);
	y_alpha_diff = (int16)((y_target) & 0xFF) - (int16)((y_source) & 0xFF);
		
	
	for (int32 y=min_y;y<=max_y;y++) {
		int32 bytes_advanced = 0;
		// Here we should move y-gradient 1 step closer to y-target
		// and put x-source to y-gradient. We should also recalculate
		// the x-gradient step.
		if (dy<0) {
			y_accumulation = (float)(abs(dy))/50.0*(float)(max_y-y)/(float)(max_y - min_y);
			if (y_accumulation < 1.0) {
				y_gradient = y_source 	+ (((int32)(y_blue_diff * (1.0-y_accumulation))) << 24)
													+ (((int32)(y_green_diff * (1.0-y_accumulation))) << 16)
													+ (((int32)(y_red_diff * (1.0-y_accumulation))) << 8)
													+ (((int32)(y_alpha_diff * (1.0-y_accumulation))));	
			}
			else 
				y_gradient = y_source;
		}
		else {
			y_accumulation = (float)dy/50.0*(float)y/(float)(max_y - min_y);
			if (y_accumulation < 1.0) {
				y_gradient = y_source 	+ (((int32)(y_blue_diff * y_accumulation)) << 24)
													+ (((int32)(y_green_diff * y_accumulation)) << 16)
													+ (((int32)(y_red_diff * y_accumulation)) << 8)
													+ (((int32)(y_alpha_diff * y_accumulation)));	
			}
			else 
				y_gradient = y_target;
		}		

		if (dx<0) {
			x_source = new_color;
			x_target = y_gradient;
		}
		else {
			x_source = y_gradient;
			x_target = new_color;
		}
		x_red_diff = (int16)((x_target>>8) & 0xFF) - (int16)((x_source>>8) & 0xFF);
		x_green_diff = (int16)((x_target>>16) & 0xFF) - (int16)((x_source>>16) & 0xFF);
		x_blue_diff = (int16)((x_target>>24) & 0xFF) - (int16)((x_source>>24) & 0xFF);
		x_alpha_diff = (int16)((x_target) & 0xFF) - (int16)((x_source) & 0xFF);
		
		x_accumulation = 0;		
		for (int32 x=min_x;x<=max_x;x++) {
			// This might crash if max_x == min_x
			if (dx < 0)
				x_accumulation = (float)abs(dx)/50.0*(float)(max_x-x)/(float)(max_x - min_x);
			else
				x_accumulation = (float)dx/50.0*(float)x/(float)(max_x - min_x);
				
			if ( (x % 8) == 0 ) {
				next_value = *binary_bits++;
				bytes_advanced++;
			}		
			if ( next_value & (0x01 << (7-(x%8))) ) {
				// Here we should put a new value into position x,y
				if (dx<0) {
					if (x_accumulation < 1.0) {
						x_gradient = x_source 	+ (((int32)(x_blue_diff * (1.0-x_accumulation))) << 24)
															+ (((int32)(x_green_diff * (1.0-x_accumulation))) << 16)
															+ (((int32)(x_red_diff * (1.0-x_accumulation))) << 8)
															+ (((int32)(x_alpha_diff * (1.0-x_accumulation))));
						drawer->SetPixel(x,y,x_gradient);
					}
					else {
						drawer->SetPixel(x,y,x_source);
					}					
				}
				else {
					if (x_accumulation < 1.0) {
						x_gradient = x_source 	+ (((int32)(x_blue_diff * x_accumulation)) << 24)
															+ (((int32)(x_green_diff * x_accumulation)) << 16)
															+ (((int32)(x_red_diff * x_accumulation)) << 8)
															+ (((int32)(x_alpha_diff * x_accumulation)));
						drawer->SetPixel(x,y,x_gradient);
					}
					else {
						drawer->SetPixel(x,y,x_target);
					}	
				}
			} 
		}
		binary_bits += binary_bpr - bytes_advanced;
	}	
}


void FillTool::FillGradientPreview(BitmapDrawer *drawer,BBitmap *binary_map,int32 dx,int32 dy,int32 min_x,int32 max_x,int32 min_y,int32 max_y,uint32 new_color,uint32 gradient_color)
{
	// This preview fill calculates only every other pixel.
	uchar *binary_bits = (uchar*)binary_map->Bits();
	int32 binary_bpr = binary_map->BytesPerRow();

	uint32 x_source,x_target;
	uint32 y_source,y_target;
	
	
	if (dx<0) {
		x_target = gradient_color;
	}
	else {
		x_target = new_color;
	}	
	if (dy<0) {
		y_target = gradient_color;
		y_source = new_color;
	}
	else {
		y_target = new_color;
		y_source = gradient_color;
	}
	uint32 y_gradient;
	uint32 x_gradient;

	// These might also be negative
	int16 x_red_diff,x_green_diff,x_blue_diff,x_alpha_diff;
	int16 y_red_diff,y_green_diff,y_blue_diff,y_alpha_diff;
	
	float x_accumulation;	
	float y_accumulation = 0;
	
	y_gradient = y_source;	
	uchar next_value = 0;
	uchar next_rows_value = 0;
	
	y_red_diff = (int16)((y_target>>8) & 0xFF) - (int16)((y_source>>8) & 0xFF);	
	y_green_diff = (int16)((y_target>>16) & 0xFF) - (int16)((y_source>>16) & 0xFF);
	y_blue_diff = (int16)((y_target>>24) & 0xFF) - (int16)((y_source>>24) & 0xFF);
	y_alpha_diff = (int16)((y_target) & 0xFF) - (int16)((y_source) & 0xFF);
		
	
	for (int32 y=min_y;y<=max_y-1;y+=2) {
		int32 bytes_advanced = 0;
		// Here we should move y-gradient 1 step closer to y-target
		// and put x-source to y-gradient. We should also recalculate
		// the x-gradient step.
		if (dy<0) {
			y_accumulation = (float)(abs(dy))/50.0*(float)(max_y-y)/(float)(max_y - min_y);
			if (y_accumulation < 1.0) {
				y_gradient = y_source 	+ (((int32)(y_blue_diff * (1.0-y_accumulation))) << 24)
													+ (((int32)(y_green_diff * (1.0-y_accumulation))) << 16)
													+ (((int32)(y_red_diff * (1.0-y_accumulation))) << 8)
													+ (((int32)(y_alpha_diff * (1.0-y_accumulation))));	
			}
			else 
				y_gradient = y_source;
		}
		else {
			y_accumulation = (float)dy/50.0*(float)y/(float)(max_y - min_y);
			if (y_accumulation < 1.0) {
				y_gradient = y_source 	+ (((int32)(y_blue_diff * y_accumulation)) << 24)
													+ (((int32)(y_green_diff * y_accumulation)) << 16)
													+ (((int32)(y_red_diff * y_accumulation)) << 8)
													+ (((int32)(y_alpha_diff * y_accumulation)));	
			}
			else 
				y_gradient = y_target;
		}		

		if (dx<0) {
			x_source = new_color;
			x_target = y_gradient;
		}
		else {
			x_source = y_gradient;
			x_target = new_color;
		}
		x_red_diff = (int16)((x_target>>8) & 0xFF) - (int16)((x_source>>8) & 0xFF);
		x_green_diff = (int16)((x_target>>16) & 0xFF) - (int16)((x_source>>16) & 0xFF);
		x_blue_diff = (int16)((x_target>>24) & 0xFF) - (int16)((x_source>>24) & 0xFF);
		x_alpha_diff = (int16)((x_target) & 0xFF) - (int16)((x_source) & 0xFF);
		
		x_accumulation = 0;		
		for (int32 x=min_x;x<=max_x-1;x+=2) {
			// This might crash if max_x == min_x
			if (dx < 0)
				x_accumulation = (float)abs(dx)/50.0*(float)(max_x-x)/(float)(max_x - min_x);
			else
				x_accumulation = (float)dx/50.0*(float)x/(float)(max_x - min_x);
				
			if ( (x % 8) == 0 ) {
				next_rows_value = *(binary_bits+binary_bpr);
				next_value = *binary_bits++;
				bytes_advanced++;
			}		
			if ( next_value & (0x01 << (7-(x%8))) ) {
				// Here we should put a new value into position x,y
				if (dx<0) {
					if (x_accumulation < 1.0) {
						x_gradient = x_source 	+ (((int32)(x_blue_diff * (1.0-x_accumulation))) << 24)
															+ (((int32)(x_green_diff * (1.0-x_accumulation))) << 16)
															+ (((int32)(x_red_diff * (1.0-x_accumulation))) << 8)
															+ (((int32)(x_alpha_diff * (1.0-x_accumulation))));
						drawer->SetPixel(x,y,x_gradient);
						if (next_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y,x_gradient);
						}
						if (next_rows_value & (0x01 << (7-(x%8)))) {
							drawer->SetPixel(x,y+1,x_gradient);
						}
						if (next_rows_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y+1,x_gradient);
						}
					}
					else {
						drawer->SetPixel(x,y,x_source);
						if (next_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y,x_source);
						}
						if (next_rows_value & (0x01 << (7-(x%8)))) {
							drawer->SetPixel(x,y+1,x_source);
						}
						if (next_rows_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y+1,x_source);
						}

					}					
				}
				else {
					if (x_accumulation < 1.0) {
						x_gradient = x_source 	+ (((int32)(x_blue_diff * x_accumulation)) << 24)
															+ (((int32)(x_green_diff * x_accumulation)) << 16)
															+ (((int32)(x_red_diff * x_accumulation)) << 8)
															+ (((int32)(x_alpha_diff * x_accumulation)));
						drawer->SetPixel(x,y,x_gradient);
						if (next_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y,x_gradient);
						}
						if (next_rows_value & (0x01 << (7-(x%8)))) {
							drawer->SetPixel(x,y+1,x_gradient);
						}
						if (next_rows_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y+1,x_gradient);
						}
					
					}
					else {
						drawer->SetPixel(x,y,x_target);
						if (next_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y,x_target);
						}
						if (next_rows_value & (0x01 << (7-(x%8)))) {
							drawer->SetPixel(x,y+1,x_target);
						}
						if (next_rows_value & (0x01 << (7-((x+1)%8)))) {
							drawer->SetPixel(x+1,y+1,x_target);
						}
					
					}	
				}
			} 
		}
		binary_bits += binary_bpr - bytes_advanced+binary_bpr;
	}	
}

BRect FillTool::calcBinaryMapBounds(BBitmap *boolean_map)
{
	// it seems like the monochrome-bitmap is aligned at 16-bit boundary instead of
	// 32-bit as the BeBook claims
	char *bits = (char*)boolean_map->Bits();
	int32 bpr = boolean_map->BytesPerRow();
	bool selected_line;

	// this is an invalid rect	
	BRect rc = BRect(100000,100000,-100000,-100000);
	
	int32 height = boolean_map->Bounds().IntegerHeight();
	for (int32 y=0;y<=height;y++) {
		selected_line = FALSE;
		for (int32 i=0;i<bpr;i++) {
			selected_line |= *bits != 0x00;
			if (*bits != 0x00) {
				for (register int32 b=0;b<8;b++) {
					rc.left = min_c(rc.left,i*8+(((*bits>>(7-b))& 0x00000001) ? b : 100000));
					rc.right = max_c(rc.right,i*8+(((*bits>>(7-b))& 0x00000001) ? b : -100000));	
				}
			}
			++bits;
		}
		if (selected_line) {
			rc.top = min_c(rc.top,y);
			rc.bottom = max_c(rc.bottom,y);
		}
	}
	
	return rc & boolean_map->Bounds();
}



status_t FillTool::readSettings(BFile &file,bool is_little_endian)
{
	if (DrawingTool::readSettings(file,is_little_endian) != B_NO_ERROR)
		return B_ERROR;
			
	if (file.Read(&gradient_color1,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;
	
	if (file.Read(&gradient_color2,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;
		
	if (is_little_endian) {
		gradient_color1 = B_LENDIAN_TO_HOST_INT32(gradient_color1);
		gradient_color2 = B_LENDIAN_TO_HOST_INT32(gradient_color2);
	}
	else {
		gradient_color1 = B_BENDIAN_TO_HOST_INT32(gradient_color1);
		gradient_color2 = B_BENDIAN_TO_HOST_INT32(gradient_color2);	
	}
	return B_NO_ERROR;;
}

status_t FillTool::writeSettings(BFile &file)
{
	if (DrawingTool::writeSettings(file) != B_NO_ERROR)
		return B_ERROR;
			
	if (file.Write(&gradient_color1,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;
	
	if (file.Write(&gradient_color2,sizeof(uint32)) != sizeof(uint32))
		return B_ERROR;
		
	return B_NO_ERROR;;

}

const void* FillTool::ReturnToolCursor()
{
	return HS_FILL_CURSOR;
}

const char* FillTool::ReturnHelpString(bool is_in_use)
{
	if (!is_in_use)
		return StringServer::ReturnString(FILL_TOOL_READY_STRING);
	else
		return StringServer::ReturnString(FILL_TOOL_IN_USE_STRING);
}


FillToolConfigView::FillToolConfigView(BRect rect, DrawingTool *t,uint32 c1, uint32 c2)
	: DrawingToolConfigView(rect,t)
{
	// BControl's and classes derived from it do not copy the message
	// when they are created.
	BMessage *control_message;
		
	BRect controller_frame = BRect(EXTRA_EDGE,EXTRA_EDGE,150+EXTRA_EDGE,EXTRA_EDGE);
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",MODE_OPTION);
	control_message->AddInt32("value",0x00000000);
	flood_checkbox = new BCheckBox(controller_frame,"enable flood fill",StringServer::ReturnString(FLOOD_FILL_STRING),control_message);
	AddChild(flood_checkbox);

	if (tool->GetCurrentValue(MODE_OPTION) == B_CONTROL_ON)
		flood_checkbox->SetValue(B_CONTROL_ON);
	else
		flood_checkbox->SetValue(B_CONTROL_OFF);
		
	controller_frame.OffsetBy(0,flood_checkbox->Frame().Height()+EXTRA_EDGE);	

	BBox *container;
	container = new BBox(controller_frame,"gradient box");
	AddChild(container);
	

	// First add a checkbox for enabling the gradient.	
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",GRADIENT_ENABLED_OPTION);
	control_message->AddInt32("value",0x00000000);
	gradient_checkbox = new BCheckBox(BRect(EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE,EXTRA_EDGE),"enable gradient box",StringServer::ReturnString(ENABLE_GRADIENT_STRING),control_message);	
	container->AddChild(gradient_checkbox);
	gradient_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(GRADIENT_ENABLED_OPTION) != B_CONTROL_OFF) {
		gradient_checkbox->SetValue(B_CONTROL_ON);
	}
	
	// Then add a checkbox for enabling the preview.
	controller_frame = gradient_checkbox->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",PREVIEW_ENABLED_OPTION);
	control_message->AddInt32("value",0x00000000);	
	preview_checkbox = new BCheckBox(controller_frame,"enable preview box",StringServer::ReturnString(ENABLE_PREVIEW_STRING),control_message);
	container->AddChild(preview_checkbox);
	preview_checkbox->ResizeToPreferred();
	if (tool->GetCurrentValue(PREVIEW_ENABLED_OPTION) != B_CONTROL_OFF) {
		preview_checkbox->SetValue(B_CONTROL_ON);
	}
	
	container->ResizeTo(max_c(container->Frame().Width(),max_c(gradient_checkbox->Frame().Width(),preview_checkbox->Frame().Width())+2*EXTRA_EDGE),0);

	// Then add a control-view for the gradient colors.	
	controller_frame = preview_checkbox->Frame();
	controller_frame.OffsetBy(0,preview_checkbox->Bounds().Height() + EXTRA_EDGE-1);	
	controller_frame.right = container->Bounds().Width()-EXTRA_EDGE;
	gradient_view = new GradientView(controller_frame,c1,c2);
	container->AddChild(gradient_view);
	container->ResizeBy(0,controller_frame.bottom+EXTRA_EDGE);
	
	controller_frame = container->Frame();
	controller_frame.OffsetBy(0,controller_frame.Height()+EXTRA_EDGE);	
	control_message = new BMessage(OPTION_CHANGED);
	control_message->AddInt32("option",TOLERANCE_OPTION);
	control_message->AddInt32("value",tool->GetCurrentValue(TOLERANCE_OPTION));
	tolerance_slider = new ControlSliderBox(controller_frame,"fill tolerance",StringServer::ReturnString(TOLERANCE_STRING),"0",control_message,0,100);	 
	AddChild(tolerance_slider);
	tolerance_slider->setValue(tool->GetCurrentValue(TOLERANCE_OPTION));
	
	ResizeTo(tolerance_slider->Bounds().Width()+2*EXTRA_EDGE,tolerance_slider->Frame().bottom + EXTRA_EDGE);	
}



void FillToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();
	flood_checkbox->SetTarget(BMessenger(this));
	gradient_checkbox->SetTarget(BMessenger(this));
	preview_checkbox->SetTarget(BMessenger(this));
//	color_view->SetTarget(BMessenger(this));
	gradient_view->SetTarget(BMessenger(this));
	tolerance_slider->SetTarget(new BMessenger(this));
}


void FillToolConfigView::MessageReceived(BMessage *message)
{
	uint32 color1;
	uint32 color2;
	
	switch (message->what) {
		case GRADIENT_ADJUSTED:
			message->FindInt32("color1",(int32*)&color1);
			message->FindInt32("color2",(int32*)&color2);
			((FillTool*)tool)->SetGradient(color1,color2);
			break;
		default:
			DrawingToolConfigView::MessageReceived(message);
			break;
	}
}



GradientView::GradientView(BRect rect,uint32 c1,uint32 c2)
	: BControl(rect,"gradient_control","Gradient",new BMessage(GRADIENT_ADJUSTED),B_FOLLOW_NONE,B_WILL_DRAW)
{
	rect.bottom = rect.top;
	rect.InsetBy(1,0);
	rect.OffsetTo(0,0);
	gradient_map = new BBitmap(rect,B_RGB32);
	
	color1 = BGRAColorToRGB(c1);
	color2 = BGRAColorToRGB(c2);	

	CalculateGradient();
	Message()->AddInt32("color1",RGBColorToBGRA(color1));
	Message()->AddInt32("color2",RGBColorToBGRA(color2));
} 



GradientView::~GradientView()
{
	delete gradient_map;
}

void GradientView::AttachedToWindow()
{
	BControl::AttachedToWindow();
}

void GradientView::Draw(BRect rect)
{
	BControl::Draw(rect);
	SetHighColor(0,0,0,255);
	StrokeRect(Bounds());
	BRect bounds = Bounds();
	bounds.InsetBy(1,1);
	DrawBitmap(gradient_map,gradient_map->Bounds(),bounds);
}



void GradientView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_PASTE:
			if (message->WasDropped()) {
				BPoint drop_point = ConvertFromScreen(message->DropPoint());
				rgb_color *new_color = new rgb_color();
				int32 color_size = sizeof(rgb_color);
				if (message->FindData("RGBColor",B_RGB_COLOR_TYPE,(const void**)&new_color,&color_size) == B_OK) {
					float half = Bounds().left + (Bounds().right-Bounds().left)/2.0;
					if (drop_point.x < half)
						color1 = *new_color;
					else
						color2 = *new_color;
					
					Message()->ReplaceInt32("color1",RGBColorToBGRA(color1));
					Message()->ReplaceInt32("color2",RGBColorToBGRA(color2));
					CalculateGradient();
					Draw(Bounds());
					Invoke();
				}
			}
			break;
		default:
			BControl::MessageReceived(message);
			break;			
	}
}



void GradientView::MouseDown(BPoint point)
{
	BControl::MouseDown(point);

	// Open the palette-view.
	float half = Bounds().left + (Bounds().right-Bounds().left)/2.0;
	rgb_color color;
	if (point.x < half)
		color = color1;
	else
		color = color2;
		
	ColorPaletteWindow::showPaletteWindow();
	ColorPaletteWindow::ChangePaletteColor(color);		
}



void GradientView::CalculateGradient()
{
	uint32 c1 = RGBColorToBGRA(color1);
	uint32 c2 = RGBColorToBGRA(color2);
	
	uint32 *bits = (uint32*)gradient_map->Bits();

	int32 width = gradient_map->BytesPerRow()/4;
	union {
		uint8 bytes[4];
		uint32 word;
	} c;
	
	for (int32 x=0;x<width;x++) {
		c.word = mix_2_pixels(c1,c2,1-(float)x/(float)(width+1));
		float coeff = c.bytes[3] / 255.0;
		if ((x % 2)	== 0) {
			c.bytes[0] = (uint8)(c.bytes[0]*coeff);
			c.bytes[1] = (uint8)(c.bytes[1]*coeff);
			c.bytes[2] = (uint8)(c.bytes[2]*coeff);
		}
		else {
			c.bytes[0] = (uint8)(c.bytes[0]*coeff + (1-coeff)*255);
			c.bytes[1] = (uint8)(c.bytes[1]*coeff + (1-coeff)*255);
			c.bytes[2] = (uint8)(c.bytes[2]*coeff + (1-coeff)*255);
		}
		*bits++ = c.word;
	}
}