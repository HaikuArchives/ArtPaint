/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H


#include <Cursor.h>
#include <MessageFilter.h>
#include <Region.h>
#include <View.h>


// these constants are used to limit view magnifying scale
// when 10.0 every pixel on bitmap translates to 10 points on screen
// when 0.1 every 10th point on bitmap is displayed on screen
#define HS_MIN_MAG_SCALE 0.1
#define HS_MAX_MAG_SCALE 16.0


// These constants are used in communication between ImageView and PaintWindow
#define	HS_HIDE_FILE_PANELS			'HiFp'


enum thread_constants {
	PAINTER_THREAD,
	MANIPULATOR_MOUSE_THREAD,
	MANIPULATOR_UPDATER_THREAD,
	MANIPULATOR_FINISHER_THREAD
};


enum {
	CANNOT_ADD_LAYER_ALERT,
	CANNOT_START_MANIPULATOR_ALERT,
	CANNOT_FINISH_MANIPULATOR_ALERT
};


enum cursor_modes {
	NORMAL_CURSOR_MODE,
	MANIPULATOR_CURSOR_MODE,
	BLOCKING_CURSOR_MODE
};


enum {
	FULL_RGB_DISPLAY_MODE,
	DITHERED_8_BIT_DISPLAY_MODE,
	CMYK_DISPLAY_MODE
};


// These two will be used in window's add-on-menus.
#define	HS_START_ADD_ON_WITH_GUI	'AowG'
#define	HS_START_PLAIN_ADD_ON		'PlAo'


class Layer;
class Selection;
class UndoEvent;
class Manipulator;
class Image;
class UndoQueue;


class ImageView : public BView {
private:
friend	filter_result	KeyFilterFunction(BMessage*,
							BHandler**, BMessageFilter*);
		Image*		the_image;

		int32		mag_scale_array_length;
		int32		mag_scale_array_index;
		float*		mag_scale_array;
		float		magnify_scale;	// magnifying scale for view, normally 1

		int32		grid_unit;		// how many pixels is one grid unit,
									// normally 1
		BPoint		grid_origin;	// the origin for grid, normally (0, 0)

		// This is the selection for the image.
		Selection*	selection;

		bool		continue_manipulator_updating;
		BWindow*	manipulator_window;

		// This message is posted to the image-view when a manipulator has
		// finished. It can be used to postpone some message only after
		// finishing the manipulation. This is used when starting a
		// manipulator before finishing the previous and when merging
		// layers when a manipulator is on.
		BMessage*	manipulator_finishing_message;

		// This returns true if the manipulator actually exists to be finished
		bool		PostponeMessageAndFinishManipulator(bool status = FALSE);

		void		AddChange();
		void		RemoveChange();

		// This variable records how many changes have been made to the
		// project
		int32		project_changed;
		int32		image_changed;

		void		setWindowTitle();
		char*		image_name;
		char*		project_name;

		cursor_modes	cursor_mode;
		BCursor*	fGrabCursor;
		BCursor* 	fGrabbingCursor;
		int32		fKeyRate;

		// These two semaphores should always be acquired in the order they
		// are declared here. Currently there seems to be some problems with
		// the acquisition of semaphores. It seems that sometimes mouse_mutex
		// is released even though it is not acquired. This is used to block
		// the MouseDown-function from doing anything if it is not permitted.
		sem_id		mouse_mutex;

		// This semaphore is used to guarantee that actions that are done on
		// the image can be finished before the view is destroyed. Such
		// actions include for example any calculations that are done by
		// manipulators and updating the undo-queue.
		sem_id		action_semaphore;

		int32		current_display_mode;

		Manipulator* fManipulator;
		int32		manipulated_layers;
		int32		add_on_id;
		int32		manip_type;
		BRegion		region_drawn_by_manipulator;

		void		DrawManipulatorGUI(bool blit_image);

		// This is the undo-queue.
		UndoQueue*	undo_queue;

		// This is used as a reference-point when displaying deltas in the
		// coordinate-view.
		BPoint		reference_point;
		bool		use_reference_point;

		bool		space_down;

		// this function rounds a point to nearest grid-unit
		BPoint		roundToGrid(BPoint point);

		void		Undo();
		void		Redo();

		void		ActiveLayerChanged();

		// These functions are used for the threads that we may spawn from
		// within the window's thread.

		// The first function is a function that can be used to start one of
		// the actual thread functions.
		void		start_thread(int32);

static	int32		enter_thread(void*);

		// For normal painting tools
		int32		PaintToolThread();
		// For tracking the mouse for manipulator
		int32		ManipulatorMouseTrackerThread();
		// For updating the view when manipulators settings change
		int32		GUIManipulatorUpdaterThread();
		// For calculating the manipulator's effect
		int32		ManipulatorFinisherThread();

		void		BlitImage(BRect);

		status_t	DoCopyOrCut(int32 layers, bool cut = FALSE);
		status_t	DoPaste();

		status_t	ShowAlert(int32);
		void		SetCursor();

		void		SetReferencePoint(BPoint, bool);

		bool		show_selection;
public:
					ImageView(BRect frame, float width, float height);
					~ImageView();
		void		AttachedToWindow();
		void		Draw(BRect updateRect);
		void		KeyDown(const char*, int32);
		void		KeyUp(const char*, int32);
		void		MessageReceived(BMessage* message);
		void		MouseDown(BPoint point);
		void		MouseMoved(BPoint where, uint32 transit, const BMessage*);
		bool		Quit();

		// These two functions are used to freeze the image structure while
		// saving. The second function then unfreezes the image. If the image
		// cannot be frozen for some reason the first function returns B_ERROR.
		status_t	Freeze();
		status_t	UnFreeze();

		// this is like GetMouse but it takes grid and magnify into account
		void		getCoords(BPoint* bitmap_point, uint32* buttons,
						BPoint* view_point = NULL);

		// this function converts a Rect from bitmap to view with
		// magnifying_scale
		BRect		convertBitmapRectToView(BRect rect);
		BRect		convertViewRectToBitmap(BRect rect);

		// this function sets the grid size
		void		setGrid(BPoint origin, int32 unit);

		// these functions set and return the magnification scale
		// the first also draws the view and resizes the window
		void		setMagScale(float scale);
inline	float		getMagScale() { return magnify_scale; }
		int32		findClosestMagIndex(float scale);

		void		UpdateImage(BRect bitmap_rect);

		// The window should be locked before calling these.
		void		adjustScrollBars();
		void		adjustSize();
		void		adjustPosition();

		// This returns a pointer to the selection.
		Selection*	GetSelection();

		Image*		ReturnImage() { return the_image; }

		void		SetDisplayMode(int32);

		void		SetToolHelpString(const char*);
		void		ResetChangeStatistics(bool project, bool image);

		void		SetProjectName(const char*);
		void		SetImageName(const char*);

const	char*		ReturnProjectName() { return project_name; }
const	char*		ReturnImageName() { return image_name; }

		bool		ShowSelection() { return show_selection; }
};

filter_result KeyFilterFunction(BMessage*, BHandler**, BMessageFilter*);

#endif
