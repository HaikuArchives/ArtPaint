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
#ifndef SELECTION_H
#define SELECTION_H

#include <Bitmap.h>
#include <View.h>

/*
	Selection-class offers a mechanism for specifying an arbitrarily-shaped,
	possibly disjoint, area from an image. In constructor it gets the size of
	the image. After constructing selections can be added to it or removed
	from it. It can return the selection's pixels sequentially as BPoint's or
	coordinate  pairs. It also calculates and returns the bounding rectangle of
	the	selected area. It can also return whether a given point belongs to the
	selection or not.

	Other major function of selection is drawing the selection to the view.
	The drawing is done with the Draw-function that gets magnifying-scale
	and bounding rectangle as a parameter. If everything is selected, the
	drawing	function should do nothing.

	The selection works in reverse. It records what is NOT selected. If
	everything is selected, like is the normal case, this will not store any
	information about the selection.

	If selection is empty when adding the first selection, we make the selection
	first full.

*/

class HSPolygon;
class SelectionIterator;
class SelectionData;


class Selection {
		// This list holds pointers to HSPolygons. The polygons make up the
		// selections.
//		BList	*selections;
		SelectionData*	selection_data;

		// This is used when rotating the selections to store the original
		// polygons.
		HSPolygon**		original_selections;


		// This is a binary-bitmap that has one bit for each pixel in the image.
		// If a bit is 1 then the corresponding pixel belongs to the selection.
		// Otherwise it doesn't belong to the selection.
		BBitmap*		selection_map;
		BView*			selection_view;

		uint8*			selection_bits;
		uint32			selection_bpr;

		// This attribute keeps track of the selection's bounds. If it is
		// not valid it should be calculated again with the
		// calculateBoundingRect function.
		BRect			selection_bounds;

		// This attribute records the image's bounding rectangle.
		BRect			image_bounds;

		// This points to the view that we want to draw to.
		BView*			image_view;

//		// If this is true, the selection is inverted.
//		bool	inverted;

		bool			needs_recalculating;

		float			view_magnifying_scale;

		// This is used to animate the lines that bound the selected area.
		int32			animation_offset;

		thread_id		drawer_thread;
		bool			continue_drawing;
		sem_id			selection_mutex;

		// This function calculates the smallest rectangle that contains all the
		// points that are selected. It records this fact in the bounding_rect
		// attribute. Bounding rectangle is calcultated when it is needed the
		// first time.
		void			calculateBoundingRect();

		// This function deselects everything.
		void			deSelect();

static	int32			thread_entry_func(void*);
		int32			thread_func();

		void			SimplifySelection();
		void		 	RasterToPolygonsMoore(BBitmap* map, BRect bounds,
							BList* polygons);

public:
						Selection(BRect);
						~Selection();

		void			SetSelectionData(const SelectionData*);

		void			StartDrawing(BView*, float);
		void			StopDrawing();

		// This function adds a polygon to the selection. The point list
		// describes polygons vertices. The polygon is stored as a point list
		// instead of BPolygon so that we can translate and rotate it.
		// The parameter point-list is not copied and thus should not be
		// deleted in the calling function.
		void			AddSelection(HSPolygon*, bool add_to_selection = TRUE);

		// This function adds a binary bitmap to the selection.
		void			AddSelection(BBitmap*, bool add_to_selection);

		// This function clears the selection.
		void			Clear();

		// this function selects the entire canvas
		void			SelectAll();

		// This dilates the selection map so that the size of the selection will
		// increase
		void			Dilate();

		// This erodes the selection so that the size of the selection will
		// decrease
		void			Erode();

		// This will draw the selection. This function does not care about
		// clipping region.
		void			Draw();

		// This function returns the bounding rectangle of the selection.
		BRect			GetBoundingRect();

		// This function inverts the whole selection. If everything is selected,
		// invert does the same as Clear.
		void			Invert();

		// This function returns true, if the selection is empty (i.e.
		// nothing is selected).
		bool			IsEmpty();

		// This function rotates the selection. The parameter is in degrees
		// with positive degrees clockwise. This only rotates the selection,
		// not the actual image.
		void			RotateTo(BPoint, float);

		// This function translates the selection by the amount given in the
		// parameters.
		void			Translate(int32, int32);

		// This function scales the selection by the amount given.
 		void			ScaleBy(BPoint, float, float);

		void			ScaleTo(BPoint, float, float);

		// These functions flip the selection around its central axes
		void			FlipHorizontally();
		void			FlipVertically();

		// This recalculates the actual contents of the selection. It can be
		// used for example after rotation, translation, or scale.
		void			Recalculate();

		void			ChangeMagnifyingScale(float);

		void			ImageSizeChanged(BRect);

const	SelectionData*	ReturnSelectionData() { return selection_data; }

		// These functions return true if the point in parameter belongs to the
		// selection (i.e. is not DEselected). The BPoint version will also
		// check the bounds rectangle while the int, int version will not check
		// bounds. The latter function can be used in combination with the
		// GetBoundingRect-function.
inline	bool			ContainsPoint(BPoint);
inline	bool			ContainsPoint(int32, int32);
};


bool
Selection::ContainsPoint(BPoint p)
{
	return ContainsPoint((int32)p.x, (int32)p.y);
}


bool
Selection::ContainsPoint(int32 x, int32 y)
{
	if (x < 0 || y < 0)
		return false;

	return (selection_bits == NULL ||
		(image_bounds.Contains(BPoint(x, y)) &&
		((*(selection_bits + y * selection_bpr + x)) != 0x00)));
}


// This class contains the vital data that describe selection. A selection
// can be archived with such data and selection can also be modified to
// represent the same selection as the SelectionData represents.
class SelectionData {
		HSPolygon**	selections;
		int32		selections_array_size;
		int32		number_of_selections;
public:
					SelectionData();
					SelectionData(const SelectionData*);
					~SelectionData();

		bool		operator==(const SelectionData&);

		void		AddSelection(HSPolygon*);
		void		EmptySelectionData();
		int32		SelectionCount() { return number_of_selections; }
		HSPolygon*	ReturnSelectionAt(int32 i) { return selections[i]; }
};

#endif
