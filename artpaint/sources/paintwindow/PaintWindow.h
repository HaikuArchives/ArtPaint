/*

	Filename:	PaintWindow.h
	Contents:	PaintWindow class declaration + constants for PaintWindow class
	Author:		Heikki Suhonen

*/

#ifndef PAINT_WINDOW_H
#define PAINT_WINDOW_H


#include <Box.h>
#include <FilePanel.h>
#include <Entry.h>
#include <TranslatorRoster.h>
#include <Window.h>


// these constants are used to determine views of a paint window
#define	HS_QUICK_TOOLS 	0x0001
#define	HS_HELP_VIEW	0x0002
#define	HS_STATUS_VIEW	0x0004
#define	HS_MENU_BAR		0x0008
#define HS_SIZING_VIEW	0x0010

// these constants are for the internal communication of the PaintWindow-class
#define	HS_SHOW_VIEW_SETUP_WINDOW		'SvsW'
#define HS_SHOW_GLOBAL_SETUP_WINDOW		'SgsW'
#define	HS_SAVE_IMAGE_INTO_RESOURCES	'Sirc'
#define	HS_SAVE_IMAGE_AS_CURSOR			'Siac'
#define	HS_RECENT_IMAGE_SIZE			'Rsis'
#define	HS_SHOW_ABOUT_WINDOW			'Sabw'

enum menu_modes {
	FULL_MENU,
	MINIMAL_MENU,
	NO_IMAGE_MENU
};


// ImageView declared in ImageView.h
class ImageView;

// BackgroundView declared in BackgroundView.h
class BackgroundView;

// StatusView declared in StatusView.h
class StatusView;

// NumberControl declared in UtilityClasses.h
class NumberControl;

// LayerWindow declared in LayerWindow.h
class LayerWindow;

// ImageSavePanel declared in FilePanels.h
class ImageSavePanel;
//PaintWindow class declaration

struct window_settings;


class PaintWindow : public BWindow {
private:
		ImageView 		*image_view;			// view to draw in
		BackgroundView 	*background;	// the backround for image

		BScrollBar 		*horiz_scroll, *vert_scroll;	// the scroll-bars

		BMenuBar 		*menubar;				// main menu for the window
		char			tool_help_string[256];	// The text that the help view will display
												// for a tool or a manipulator.

		StatusView 		*status_view;

		// these next views are used when setting the image size
		NumberControl 	*width_view,*height_view;
		BButton			*set_size_button;
		BBox			*container_box;

		// these variables hold the info about how much space
		// all the additional views take, used when resizing the window
		// to fit image
		float 			additional_height, additional_width;

		// this is the file-panel that is used when saving the image
		ImageSavePanel	*image_save_panel;
		BFilePanel		*project_save_panel;

		window_settings	*settings;

		// The BEntrys should be replace by something else as they
		// consume a file-descriptor.
		BEntry			image_entry;
		BEntry			project_entry;
		uint32			current_handler;


// This counter keeps count of how many paint-windows we have open,
// it is incremented in constructor and dectremented in destructor.
static	int32			paint_window_count;

static	int32			untitled_window_number;

// This list contains pointers to all of the paint-windows
static	BList			*paint_window_list;


// this function will create the main menubar
bool		openMenuBar();


// this function will resize the window to fit image
void		resizeToFit();
BRect		getPreferredSize();

BRect		user_frame;

// this function will show the layer-window
//void	showLayerWindow();


// These function will save the image.
static	int32		save_image(void*);
		status_t	saveImage(BMessage *message);

// These functions will save the project.
static	int32		save_project(void*);
		status_t	saveProject(BMessage *message);

// this function will save the image in to a resource-file.
status_t	saveImageIntoResources();
status_t	saveImageAsCursor();

// this function will write the attributes to node
void			writeAttributes(BNode &node);

static	int32	AddAddOnsToMenu(void*);

// This function changes the enability of some menu-items. The parameter
// is used as a guide to what should be enabled and what not.
void	ChangeMenuMode(menu_modes new_mode);
		PaintWindow(char *name,BRect frame,uint32 views,const window_settings *setup);

public:
static	PaintWindow*	createPaintWindow(BBitmap* =NULL,char* = NULL,int32 =0,entry_ref =entry_ref(), translator_id outTranslator=0);
		~PaintWindow();
void	FrameResized(float, float);
void	FrameMoved(BPoint);
void	MenusBeginning();
void	MenusEnded();
void 	MessageReceived(BMessage *message);
bool	QuitRequested();
void	WindowActivated(bool active);
void	WorkspaceActivated(int32,bool);
void	Zoom(BPoint,float,float);

void	DisplayCoordinates(BPoint point,BPoint reference,bool use_reference);
void	displayMag(float mag);

void	SetHelpString(const char*,int32);

inline	window_settings*	Settings() { return settings; }
// this function will read the attributes from a node, it is called whenever an image
// is loaded
void	readAttributes(BNode &node);

// this is the function used by layer-window to inform paint-windows that
// it has been closed if no target window is specified at the moment
//static	void	LayerWindowClosed();

// This returns the number of paint-windows
static	int32	CountPaintWindows() { return paint_window_count; }

// This function will create the ImageView. The width and height parameters
// should reflect the actual size of image in pixels.
status_t	OpenImageView(int32,int32);

// This function will add the image-view to window's view hierarchy.
// And finish its initialization.
status_t	AddImageView();



// This function returns a pointer to the image
ImageView*	ReturnImageView() { return image_view; }

BEntry*		ImageEntry() { return &image_entry; }
BEntry*		ProjectEntry() { return &project_entry; }


StatusView*	ReturnStatusView();
};

#endif
