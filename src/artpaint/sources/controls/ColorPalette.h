/* 

	Filename:	ColorPalette.h
	Contents:	declaretions for ColorPaletteWindow and some of its children views	
	Author:		Heikki Suhonen
	
*/

#ifndef COLOR_PALETTE_H
#define COLOR_PALETTE_H

#include <ColorControl.h>
#include <Window.h>

class ColorContainer;
class ColorSet;
class VisualColorControl;
class PaletteWindowClient;


// we have to derive the BColorControl-class just to get
// the mouse-button that was pressed
class HSColorControl : public BColorControl {

public:
		HSColorControl(BPoint left_top, color_control_layout matrix, float cellSide, const char *name);
void	MouseDown(BPoint location);
};

enum color_window_modes {
	HS_SIMPLE_COLOR_MODE 	=	'SiCM',
	HS_RGB_COLOR_MODE		=	'RgbM',
	HS_CMY_COLOR_MODE		=	'CmyM',
	HS_YIQ_COLOR_MODE		=	'YiqM',
	HS_YUV_COLOR_MODE		=	'YuvM',
	HS_HSV_COLOR_MODE		=	'HsvM'
};


class ColorPaletteWindow : public BWindow {
private:
		int32 selector_mode;

		// This lists the windows that have the colorwindow in
		// their subset.
static	BList	*master_window_list;
static	BList	*palette_window_clients;


// this variable holds a derived color control object		
		HSColorControl 			*color_control;

// this holds an ColorControl-object
		VisualColorControl		*color_slider;

// this variable points to the color container object
		ColorContainer *color_container;

// these buttons can change to next or previous color-set
		BPictureButton *previous_set,*next_set;

// this variable points to menubar that is opened
		BMenuBar *menu_bar;

// these are boxes that can be used to hold the components
		BBox *box1,*box2;

// these point to file-panels for opening and saving palette
		BFilePanel *open_panel,*save_panel;


// This static holds the pointer to the open palette-window.
// If no window is open, it is NULL
static	ColorPaletteWindow	*palette_window;
				
// these functions are used to open necessary control views
// and delete them when necessary
bool	openControlViews(int32 mode);
void	deleteControlViews(int32 mode);
void	openMenuBar();


// these functions handle the palette loading and saving
void	handlePaletteLoad(BMessage *message);
void	handlePaletteSave(BMessage *message);


static	void	InformClients(const rgb_color&);
public:
		ColorPaletteWindow(BRect frame,int32 mode);		
		~ColorPaletteWindow();
bool	QuitRequested();
void	MessageReceived(BMessage *message);

rgb_color	getColor(int32 index);

// This static function will bring the palette window to front, or open
// it if none is already open. If a message is provided, it is presumed
// to contain refs to a palette file, and that will be loaded.
static	void	showPaletteWindow(BMessage* = NULL);
static	void	ChangePaletteColor(rgb_color&);
static	void	setFeel(window_feel);
static	void	AddMasterWindow(BWindow*);
static	void	RemoveMasterWindow(BWindow*);

static	void	AddPaletteWindowClient(PaletteWindowClient*);
static	void	RemovePaletteWindowClient(PaletteWindowClient*);
};




class ColorContainer : public BView {

		// these variables hold the vital info about the container
		int32 	color_count;
		int32	row_count;
		int32	horiz_c_size;
		int32	vert_c_size;
		int32	horiz_gutter;
		int32	vert_gutter;


		// this variable holds the information whether the selected color
		// should be highlighted or not
		bool	highlight_selected;
		bool	dragging_enabled;		
		bool	contains_arrows;
		
		
		// this points to list that holds the pointers to every color container
		// an entry is added to the front of list whenever container is created
		// and is removed whenever a container is deleted
		static 	BList 	*container_list;

		
		BPictureButton	*left_arrow;
		BPictureButton	*right_arrow;


// this function gives the rect for palette entry at index
BRect	colorBounds(int32 index);

// this function gives the index for palette entry at point
// or -1 if none
int32	pointIndex(BPoint point);		

// this sets up the container view and initializes variables
// it is called whenever a container is created and whenever
// palette is changed
void	setUpContainer(BRect frame, int32 number_of_colors,bool add_arrows);

void	SetHighAndLowColors(const rgb_color&);

public:
		// this constructs a container that fits inside the rectangle
		// or stretches the rectangle if it is too small
		ColorContainer(BRect frame, int32 amount_of_colors,uint32 resizingMode=B_FOLLOW_NONE, bool highlight=TRUE,bool add_arrows=FALSE);
		
		// this removes the container from container_list
		~ColorContainer();

void	AttachedToWindow();
void	Draw(BRect);
void	MouseDown(BPoint point);
void	MouseMoved(BPoint,uint32,const BMessage*);
void	MessageReceived(BMessage *message);

void	colorChanged(int32 color_index);


void	SetDraggingEnabled(bool e) { dragging_enabled = e; }

// this function sends a message to all color containers
static	void	sendMessageToAllContainers(BMessage *message);
};


// this is a class that holds a color_set
class ColorSet {
		rgb_color		*palette;
		int32			current_color_index;
		int32			color_count;
		char			*name;

		// here is the list that holds all the color-sets and the index of current set
		static	BList	*color_set_list;
		static	int32	current_set_index;	
		
		// we should probably move the selected color from color-container
		// to here so that each set could have different selected color
		// but each container would have same selected color and
		// current color would then be gotten by calling
		// ColorSetList::currentSet()->currentColor(), or something
		// similar. Also a function for setting the current color
		// should be made available.
public:
				ColorSet(int32 amount_of_colors, ColorSet *copy_this_palette = NULL);
				~ColorSet();
inline	int32	sizeOfSet() { return color_count; }
rgb_color	colorAt(int32 index);	

inline	rgb_color	currentColor() { return palette[current_color_index]; }
inline	int32	currentColorIndex() { return current_color_index; }


// these are used to set colors of the set, the first is used when loading a set
void	setColor(int32 index,rgb_color color);
inline	void	setCurrentColor(rgb_color color) { palette[current_color_index] = color; }
inline	void	setCurrentColorIndex(int32 index) { current_color_index = index; }

void	setName(const char *set_name);
char*	getName();

static	inline	ColorSet*	currentSet() { return (ColorSet*)color_set_list->ItemAt(current_set_index); }
static	inline	void		moveToNextSet() { current_set_index = min_c(current_set_index+1,color_set_list->CountItems()-1); }
static	inline	void		moveToPrevSet() { current_set_index = max_c(current_set_index-1,0); }
static	inline	int32		numberOfSets() { return color_set_list->CountItems(); }

// these functions read and write all the sets to the preferences-file
static			status_t	readSets(BFile &file);
static			status_t	writeSets(BFile &file);
};

#endif