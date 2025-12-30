/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "SelectorTool.h"

#include "BitmapDrawer.h"
#include "BitmapUtilities.h"
#include "Cursors.h"
#include "HSPolygon.h"
#include "Image.h"
#include "ImageUpdater.h"
#include "ImageView.h"
#include "IntelligentPathFinder.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Patterns.h"
#include "PixelOperations.h"
#include "Selection.h"
#include "ToolScript.h"
#include "UtilityClasses.h"


#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <RadioButton.h>
#include <SeparatorView.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


SelectorTool::SelectorTool()
	:
	DrawingTool(B_TRANSLATE("Selection tool"), "s", SELECTOR_TOOL),
	ToolEventAdapter()
{
	// the value for mode will be either B_OP_ADD or B_OP_SUBTRACT
	fOptions = MODE_OPTION | SHAPE_OPTION | TOLERANCE_OPTION;

	SetOption(MODE_OPTION, B_OP_ADD);
	SetOption(SHAPE_OPTION, HS_RECTANGLE);
	SetOption(TOLERANCE_OPTION, 10);

	selectionPoints = NULL;
	numPoints = 0;
	activePoints = NULL;
	numActivePoints = 0;
}


SelectorTool::~SelectorTool()
{
	if (selectionPoints != NULL)
		delete[] selectionPoints;

	if (activePoints != NULL)
		delete[] activePoints;
}


ToolScript*
SelectorTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint view_point)
{
	// these are used when drawing the preview to the view
	BPoint original_point = point;
	BPoint view_original_point = view_point;

	Selection* selection = view->GetSelection();

	BWindow* window = view->Window();
	if (window == NULL)
		return NULL;

	ToolScript* the_script
		= new ToolScript(Type(), fToolSettings, ((PaintApplication*)be_app)->Color(true));

	bool addSelection = true;

	window->Lock();
	view->SetHighColor(255, 255, 255, 255);
	view->SetLowColor(0, 0, 0, 255);
	view->SetPenSize(1);
	view->SetDrawingMode(B_OP_COPY);
	view->MovePenTo(view_point);
	if (modifiers() & B_OPTION_KEY)
		addSelection = false;
	else
		addSelection = true;
	BBitmap* buffer = view->ReturnImage()->ReturnActiveBitmap();
	window->Unlock();

	ImageUpdater* imageUpdater = new ImageUpdater(view, 2000);

	if (fToolSettings.shape == HS_INTELLIGENT_SCISSORS) {
		if (buffer->Bounds().Contains(point) == FALSE) {
			delete the_script;
			return NULL;
		}

		IntelligentPathFinder* path_finder
			= new IntelligentPathFinder(view->ReturnImage()->ReturnActiveBitmap());
		HSPolygon* the_polygon = new HSPolygon(&point, 1);
		BPolygon* active_view_polygon = NULL;

		path_finder->SetSeedPoint(int32(point.x), int32(point.y));
		bool finished = false;
		BPoint prev_point = point;
		BPoint seed_point = point;

		BRect old_rect;
		BRect old_bitmap_rect;

		int32 size = 100;
		numPoints = 0;
		selectionPoints = new BPoint[size];

		while (finished == false) {
			// Add support for different zoom-levels. Currently it does not support
			// changing the zoom-level while the tool is working.
			BRect view_rect;
			BRect bitmap_rect = view->ReturnImage()->ReturnActiveBitmap()->Bounds();
			if (is_clicks_data_valid) {
				is_clicks_data_valid = false;
				if (last_click_clicks >= 2) {
					if (modifiers() & B_OPTION_KEY)
						addSelection = false;
					finished = true;
					delete active_view_polygon;
					active_view_polygon = NULL;
				} else if (point != seed_point && bitmap_rect.Contains(point)) {
					// Here we should add a section from seed_point to
					// point (or last_click_bitmap_location) to the_polygon,
					// and respective changes to view_polygon too. If the
					// route between these two points is not yet calculated
					// we should wait until it becomes calculated by the
					// dynamic programming. Then the distance-map should be
					// cleared and new round of dynamic programming should be
					// started with last_click_bitmap_location as a seed_point.
					if (numActivePoints > 0 && activePoints != NULL) {
						delete[] activePoints;
						activePoints = NULL;
						numActivePoints = 0;
					}
					activePoints
						= path_finder->ReturnPath(int32(point.x), int32(point.y), &numActivePoints);
					while (activePoints == NULL) {
						activePoints
							= path_finder->ReturnPath(int32(point.x), int32(point.y), &numActivePoints);
						snooze(50 * 1000);
					}
					the_polygon->AddPoints(activePoints, numActivePoints, true);
					if (numPoints + numActivePoints > size) {
						size = numPoints + numActivePoints + 1;
						BPoint* tmpPoints = new BPoint[size];
						for (int i = 0; i < numPoints; ++i) {
							tmpPoints[i] = selectionPoints[i];
						}
						delete[] selectionPoints;
						selectionPoints = tmpPoints;
					}

					for (int i = 0; i < numActivePoints; ++i)
						selectionPoints[numPoints + i] = activePoints[numActivePoints - i - 1];

					numPoints += numActivePoints;
					delete activePoints;
					activePoints = NULL;
					numActivePoints = 0;

					BRect updatedRect = the_polygon->BoundingBox();
					imageUpdater->AddRect(updatedRect);
					imageUpdater->ForceUpdate();

					path_finder->SetSeedPoint(int32(point.x), int32(point.y));
					seed_point = point;
					if (numActivePoints > 0 && activePoints != NULL) {
						delete[] activePoints;
						activePoints = NULL;
						numActivePoints = 0;
					}
				}
			} else {
				// We should draw the active segment to the view if necessary
				// (i.e. point has changed)
				if (point != prev_point) {
					if (active_view_polygon != NULL) {
						bitmap_rect = active_view_polygon->Frame();
						imageUpdater->AddRect(bitmap_rect);
						imageUpdater->ForceUpdate();
						delete active_view_polygon;
					}
					if (numActivePoints > 0 && activePoints != NULL) {
						delete[] activePoints;
						activePoints = NULL;
						numActivePoints = 0;
					}
					activePoints
						= path_finder->ReturnPath(int32(point.x), int32(point.y), &numActivePoints);

					if (activePoints == NULL) {
						activePoints = new BPoint[3];
						activePoints[0] = seed_point;
						activePoints[1] = point;
						activePoints[2] = point;
						numActivePoints = 3;
					}
					active_view_polygon = new BPolygon(activePoints, numActivePoints);
					BRect updatedRect = active_view_polygon->Frame();
					imageUpdater->AddRect(updatedRect);
					imageUpdater->ForceUpdate();

					prev_point = point;
				}
			}
			if (view->LockLooper()) {
				view->getCoords(&point, &buttons, &view_point);
				view->UnlockLooper();
			}
			snooze(50 * 1000);
		}

		delete path_finder;
		selection->AddSelection(the_polygon, addSelection);
	} else if (fToolSettings.shape == HS_FREE_LINE) {
		int32 size = 100;
		numPoints = 0;
		selectionPoints = new BPoint[size];

		while (buttons) {
			if (modifiers() & B_OPTION_KEY)
				addSelection = false;
			else
				addSelection = true;

			selectionPoints[numPoints++] = point;
			the_script->AddPoint(point);
			if (numPoints == size) {
				BPoint* new_list = new BPoint[2 * size];
				for (int32 i = 0; i < numPoints; i++)
					new_list[i] = selectionPoints[i];

				delete[] selectionPoints;
				selectionPoints = new_list;
				size = 2 * size;
			}

			view->Window()->Lock();
			view->getCoords(&point, &buttons, &view_point);
			view->Window()->Unlock();

			BRect updatedRect = MakeRectFromPoints(selectionPoints[numPoints-1], selectionPoints[numPoints-2]);
			imageUpdater->AddRect(updatedRect.InsetByCopy(-5, -5));
			imageUpdater->ForceUpdate();

			snooze(20 * 1000);

		}
		HSPolygon* poly = new HSPolygon(selectionPoints, numPoints, HS_POLYGON_CLOCKWISE);
		selection->AddSelection(poly, addSelection);
	} else if (fToolSettings.shape == HS_MAGIC_WAND) {
		BBitmap* original_bitmap = view->ReturnImage()->ReturnActiveBitmap();
		BBitmap* selection_map;
		// We use a fill-tool to select the area:
		BitmapDrawer* drawer = new BitmapDrawer(original_bitmap);
		selection_map = MakeFloodBinaryMap(drawer, 0, int32(original_bitmap->Bounds().right), 0,
			int32(original_bitmap->Bounds().bottom), drawer->GetPixel(original_point),
			original_point);
		if (modifiers() & B_OPTION_KEY)
			addSelection = false;
		else
			addSelection = true;

		selection->AddSelection(selection_map, addSelection);
		delete drawer;
		delete selection_map;
	} else if (fToolSettings.shape == HS_CIRCLE || fToolSettings.shape == HS_RECTANGLE) {
		BRect old_rect, new_rect;
		old_rect = new_rect = BRect(point, point);
		float left, top, right, bottom;

		the_script->AddPoint(point);
		selectionPoints = new BPoint[2];

		while (buttons) {
			if (modifiers() & B_OPTION_KEY)
				addSelection = false;
			else
				addSelection = true;

			if (old_rect != new_rect)
				old_rect = new_rect;

			view->Window()->Lock();
			view->getCoords(&point, &buttons, &view_point);
			view->Window()->Unlock();

			BRect bitmap_rect = MakeRectFromPoints(original_point, point);
			if (modifiers() & B_SHIFT_KEY) {
				// Make the rectangle square.
				float max_distance = max_c(bitmap_rect.Height(), bitmap_rect.Width());
				if (original_point.x == bitmap_rect.left)
					bitmap_rect.right = bitmap_rect.left + max_distance;
				else
					bitmap_rect.left = bitmap_rect.right - max_distance;

				if (original_point.y == bitmap_rect.top)
					bitmap_rect.bottom = bitmap_rect.top + max_distance;
				else
					bitmap_rect.top = bitmap_rect.bottom - max_distance;
			}
			if (modifiers() & B_COMMAND_KEY) {
				// Make the the rectangle original corner be at the center of
				// new rectangle.
				float y_distance = bitmap_rect.Height();
				float x_distance = bitmap_rect.Width();

				if (bitmap_rect.left == original_point.x)
					bitmap_rect.left = bitmap_rect.left - x_distance;
				else
					bitmap_rect.right = bitmap_rect.right + x_distance;

				if (bitmap_rect.top == original_point.y)
					bitmap_rect.top = bitmap_rect.top - y_distance;
				else
					bitmap_rect.bottom = bitmap_rect.bottom + y_distance;
			}

			selectionPoints[0] = bitmap_rect.LeftTop();
			selectionPoints[1] = bitmap_rect.RightBottom();
			numPoints = 2;

			new_rect = bitmap_rect;
			imageUpdater->AddRect(old_rect.InsetByCopy(-1, -1));

			imageUpdater->ForceUpdate();

			snooze(20 * 1000);
		}
		the_script->AddPoint(point);

		if (fToolSettings.shape == HS_CIRCLE) {
			BBitmap* selection_map;
			BBitmap* draw_map
				= new BBitmap(view->ReturnImage()->ReturnActiveBitmap()->Bounds(), B_RGBA32);
			memset(draw_map->Bits(), 0x00, draw_map->BitsLength());
			// We use a fill-tool to select the area:
			BitmapDrawer* drawer = new BitmapDrawer(draw_map);
			drawer->DrawEllipse(new_rect, 0xFFFFFFFF, TRUE, FALSE);
			selection_map = BitmapUtilities::ConvertColorSpace(draw_map, B_GRAY8);
			selection->AddSelection(selection_map, addSelection);
			delete drawer;
			delete selection_map;
		} else {
			BPoint point_list[4];
			point_list[0] = old_rect.LeftTop();
			point_list[1] = old_rect.RightTop();
			point_list[2] = old_rect.RightBottom();
			point_list[3] = old_rect.LeftBottom();
			HSPolygon* poly = new HSPolygon(point_list, 4, HS_POLYGON_CLOCKWISE);
			selection->AddSelection(poly, addSelection);
		}
	}

	if (numPoints > 0 && selectionPoints != NULL) {
		delete[] selectionPoints;
		selectionPoints = NULL;
		numPoints = 0;
	}
	if (numActivePoints > 0 && activePoints != NULL) {
		delete[] activePoints;
		activePoints = NULL;
		numActivePoints = 0;
	}
	delete imageUpdater;

	view->Window()->Lock();
	view->Invalidate();
	view->Window()->Unlock();

	return the_script;
}


BView*
SelectorTool::ConfigView()
{
	return new SelectorToolConfigView(this);
}


const void*
SelectorTool::ToolCursor() const
{
	if (modifiers() & B_OPTION_KEY)
		return HS_SELECTOR_SUBTRACT_CURSOR;

	return HS_SELECTOR_CURSOR;
}


const char*
SelectorTool::HelpString(bool isInUse) const
{
	return (isInUse
			? B_TRANSLATE("Making a selection.")
			: B_TRANSLATE(
				"Selection tool: SHIFT for square/circle, ALT centers selection, OPT subtracts"));
}


void
SelectorTool::CheckSpans(BPoint span_start, BitmapDrawer* drawer, PointStack& stack, int32 min_x,
	int32 max_x, uint32 old_color, int32 tolerance, BBitmap* binary_fill_map, span_type spans)
{
	// First get the vital data.
	int32 x, start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;
	bool inside_lower_span = false;
	bool inside_upper_span = false;

	// This is the case that takes the variance into account. We must use a
	// binary bitmap to see what parts have already been filled.
	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar* binary_bits = (uchar*)binary_fill_map->Bits();

	if (y > binary_fill_map->Bounds().Height() || y < 0)
		return;

	// Then go from start towards the left side of the bitmap.
	while (x >= min_x && x <= max_x
		&& compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance)
		&& (*(binary_bits + y * binary_bpr + x) & 0x01) == 0x00) {

		*(binary_bits + y * binary_bpr + x) = 0xFF;

		if (spans == BOTH || spans == LOWER) {
			uint32 pixel_color = drawer->GetPixel(x, y + 1);
			bool match = compare_2_pixels_with_variance(pixel_color, old_color, tolerance);
			if (inside_lower_span == false && match == true)
				stack.Push(BPoint(x, y + 1));
			inside_lower_span = match;
		}

		if (spans == BOTH || spans == UPPER) {
			uint32 pixel_color = drawer->GetPixel(x, y - 1);
			bool match = compare_2_pixels_with_variance(pixel_color, old_color, tolerance);
			if (inside_upper_span == false && match == true)
				stack.Push(BPoint(x, y - 1));
			inside_upper_span = match;
		}
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	// We might already be inside a lower span
	inside_lower_span
		= compare_2_pixels_with_variance(drawer->GetPixel(start_x, y + 1), old_color, tolerance);
	inside_upper_span
		= compare_2_pixels_with_variance(drawer->GetPixel(start_x, y - 1), old_color, tolerance);
	x = start_x + 1;
	while (x >= min_x && x <= max_x
		&& compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance)
		&& (*(binary_bits + y * binary_bpr + x) & 0x01) == 0x00) {
		*(binary_bits + y * binary_bpr + x) = 0xFF;

		if (spans == BOTH || spans == LOWER) {
			uint32 pixel_color = drawer->GetPixel(x, y + 1);
			bool match = compare_2_pixels_with_variance(pixel_color, old_color, tolerance);
			if (inside_lower_span == false && match == true)
				stack.Push(BPoint(x, y + 1));
			inside_lower_span = match;
		}

		if (spans == BOTH || spans == UPPER) {
			uint32 pixel_color = drawer->GetPixel(x, y - 1);
			bool match = compare_2_pixels_with_variance(pixel_color, old_color, tolerance);
			if (inside_upper_span == false && match == true)
				stack.Push(BPoint(x, y - 1));
			inside_upper_span = match;
		}
		x++;
	}
}


void
SelectorTool::FillSpan(BPoint span_start, BitmapDrawer* drawer, int32 min_x, int32 max_x,
	uint32 old_color, int32 tolerance, BBitmap* binary_fill_map)
{
	// First get the vital data.
	int32 x, start_x;
	int32 y = (int32)span_start.y;
	x = start_x = (int32)span_start.x;

	// This is the case that takes the variance into account. We must use a
	// binary bitmap to see what parts have already been filled.
	uint32 binary_bpr = binary_fill_map->BytesPerRow();
	uchar* binary_bits = (uchar*)binary_fill_map->Bits();

	// Then go from start towards the left side of the bitmap.
	while (x >= min_x
		&& compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance)) {
		*(binary_bits + y * binary_bpr + x) = 0xFF;
		x--;
	}

	// Then go from start_x+1 towards the right side of the bitmap.
	x = start_x + 1;
	while (x <= max_x
		&& compare_2_pixels_with_variance(drawer->GetPixel(x, y), old_color, tolerance)) {
		*(binary_bits + y * binary_bpr + x) = 0xFF;
		x++;
	}
}


BBitmap*
SelectorTool::MakeFloodBinaryMap(BitmapDrawer* drawer, int32 min_x, int32 max_x, int32 min_y,
	int32 max_y, uint32 old_color, BPoint start)
{
	// This function makes a binary bitmap of the image. It contains 255 where
	// the flood fill should fill and zeroes elsewhere.
	BBitmap* fill_map = new BBitmap(BRect(min_x, min_y, max_x, max_y), B_GRAY8);
	memset(fill_map->Bits(), 0x00, fill_map->BitsLength());

	// Here fill the area using drawer's SetPixel and GetPixel.
	// The algorithm uses 4-connected version of flood-fill.
	// The SetPixel and GetPixel functions are versions that
	// do not check bounds so we have to be careful not to exceed
	// bitmap's bounds.
	uint32 tolerance = (uint32)((float)fToolSettings.tolerance / 100.0 * 255);

	PointStack stack;
	stack.Push(start);

	while (!stack.IsEmpty()) {
		BPoint span_start = stack.Pop();
		if (span_start.y == min_y && min_y != max_y) {
			// Only check the spans below this line
			CheckSpans(
				span_start, drawer, stack, min_x, max_x, old_color, tolerance, fill_map, LOWER);
		} else if (span_start.y == max_y && min_y != max_y) {
			// Only check the spans above this line.
			CheckSpans(
				span_start, drawer, stack, min_x, max_x, old_color, tolerance, fill_map, UPPER);
		} else if (min_y != max_y) {
			// Check the spans above and below this line.
			CheckSpans(
				span_start, drawer, stack, min_x, max_x, old_color, tolerance, fill_map, BOTH);
		} else {
			// The image is only one pixel high. Check the only span.
			FillSpan(span_start, drawer, min_x, max_x, old_color, tolerance, fill_map);
		}
	}

	// Remember to NULL the attribute binary_fill_map
	return fill_map;
}


void
SelectorTool::DrawSelection(BView* view)
{
	if (view != NULL) { // && selectionPoints != NULL && numPoints > 0) {
		ImageView* img_view = dynamic_cast<ImageView*>(view);
		float scale = img_view->getMagScale();

		if (fToolSettings.shape == HS_FREE_LINE && selectionPoints != NULL && numPoints > 0) {
			for (int i = 0; i < numPoints; ++i)
				img_view->StrokeLine(BPoint(selectionPoints[i].x * scale,
					selectionPoints[i].y * scale), HS_ANIMATED_STRIPES_1);

		} else if ((fToolSettings.shape == HS_CIRCLE || fToolSettings.shape == HS_RECTANGLE)
			&& selectionPoints != NULL && numPoints > 0) {

			BRect new_rect = MakeRectFromPoints(selectionPoints[0], selectionPoints[1]);
			if (fToolSettings.shape == HS_RECTANGLE)
				img_view->StrokeRect(img_view->convertBitmapRectToView(new_rect), HS_ANIMATED_STRIPES_1);
			else
				img_view->StrokeEllipse(img_view->convertBitmapRectToView(new_rect), HS_ANIMATED_STRIPES_1);
		} else if (fToolSettings.shape == HS_INTELLIGENT_SCISSORS) {
			if (selectionPoints != NULL && numPoints > 0) {
				BPolygon bpoly(selectionPoints, numPoints);
				BRect bitmap_rect = bpoly.Frame();
				BRect view_rect = img_view->convertBitmapRectToView(bitmap_rect);

				bpoly.MapTo(bitmap_rect, view_rect);
				img_view->MovePenTo(selectionPoints[0]);
				img_view->StrokePolygon(&bpoly, false, HS_ANIMATED_STRIPES_1);
			}

			if (activePoints != NULL && numActivePoints > 0) {
				drawing_mode oldMode = img_view->DrawingMode();
				img_view->SetDrawingMode(B_OP_INVERT);
				BPolygon apoly(activePoints, numActivePoints);
				BRect bitmap_rect = apoly.Frame();
				BRect view_rect = img_view->convertBitmapRectToView(bitmap_rect);

				apoly.MapTo(bitmap_rect, view_rect);
				img_view->StrokePolygon(&apoly, false);
				img_view->SetDrawingMode(oldMode);
			}
		}
	}
}


// #pragma mark -- SelectorToolConfigView


SelectorToolConfigView::SelectorToolConfigView(DrawingTool* tool)
	:
	DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SHAPE_OPTION);
		message->AddInt32("value", HS_FREE_LINE);
		fFreeLine = new BRadioButton(B_TRANSLATE("Freehand"), new BMessage(*message));

		message->ReplaceInt32("value", HS_RECTANGLE);
		fRectangle = new BRadioButton(B_TRANSLATE("Rectangle"), new BMessage(*message));

		message->ReplaceInt32("value", HS_CIRCLE);
		fEllipse = new BRadioButton(B_TRANSLATE("Ellipse"), new BMessage(*message));

		message->ReplaceInt32("value", HS_INTELLIGENT_SCISSORS);
		fScissors = new BRadioButton(B_TRANSLATE("Intelligent scissors"), new BMessage(*message));

		message->ReplaceInt32("value", HS_MAGIC_WAND);
		fMagicWand = new BRadioButton(B_TRANSLATE("Magic wand"), new BMessage(*message));

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", TOLERANCE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(TOLERANCE_OPTION));
		fTolerance
			= new NumberSliderControl(B_TRANSLATE("Tolerance:"), "10", message, 0, 100, false);
		fTolerance->SetEnabled(FALSE);

		BGridLayout* toleranceLayout = LayoutSliderGrid(fTolerance);

		layout->AddView(BGroupLayoutBuilder(B_VERTICAL, kWidgetSpacing)
			.AddGroup(B_VERTICAL, kWidgetSpacing)
				.Add(fFreeLine)
				.Add(fRectangle)
				.Add(fEllipse)
				.Add(fScissors)
				.Add(fMagicWand)
				.SetInsets(kWidgetInset, 0.0, 0.0, 0.0)
			.End()
			.AddGroup(B_HORIZONTAL, 0)
				.AddStrut(B_USE_DEFAULT_SPACING)
				.Add(toleranceLayout)
				.AddGlue()
				.SetInsets(B_USE_ITEM_SPACING, 0.0, 0.0, 0.0)
			.End()
			.TopView()
		);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_FREE_LINE)
			fFreeLine->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_RECTANGLE)
			fRectangle->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_CIRCLE)
			fEllipse->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_INTELLIGENT_SCISSORS)
			fScissors->SetValue(B_CONTROL_ON);

		if (tool->GetCurrentValue(SHAPE_OPTION) == HS_MAGIC_WAND) {
			fMagicWand->SetValue(B_CONTROL_ON);
			fTolerance->SetEnabled(TRUE);
		}
	}
}


void
SelectorToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fFreeLine->SetTarget(this);
	fRectangle->SetTarget(this);
	fEllipse->SetTarget(this);
	fMagicWand->SetTarget(this);
	fScissors->SetTarget(this);
	fTolerance->SetTarget(this);
}


void
SelectorToolConfigView::MessageReceived(BMessage* message)
{
	DrawingToolConfigView::MessageReceived(message);

	switch (message->what) {
		case OPTION_CHANGED:
		{
			if (message->FindInt32("option") == SHAPE_OPTION) {
				if (fMagicWand->Value() == B_CONTROL_OFF)
					fTolerance->SetEnabled(FALSE);
				else
					fTolerance->SetEnabled(TRUE);
			}
		} break;
	}
}
