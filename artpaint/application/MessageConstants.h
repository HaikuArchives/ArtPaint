/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MESSAGE_CONSTANTS_H
#define MESSAGE_CONSTANTS_H

// these constants are for communication between paint window and application
#define HS_PAINT_WINDOW_CLOSED 'PwCl'
#define HS_PAINT_WINDOW_OPENED 'PwOp'
#define HS_PAINT_WINDOW_ACTIVATED	'PwAc'
#define HS_NEW_PAINT_WINDOW		'NPtW'

// these constants are for comunication between color palette window and application
#define HS_COLOR_CHANGED		'CoCH'

// these are used with color and rgb-controls in palette-window's
// internal communication
#define	HS_COLOR_CONTROL_INVOKED	'CCIn'
#define	HS_RGB_CONTROL_INVOKED		'RGIn'

// and these are used when creating new palettes or loading and saving them
#define HS_NEW_PALETTE_CREATED	'NPaC'
#define HS_PALETTE_CHANGED		'PaCh'
#define HS_NEXT_PALETTE			'NxPl'
#define HS_PREVIOUS_PALETTE		'PrPl'
#define HS_SET_NAME_CHANGED		'SnCh'
#define HS_SHOW_PALETTE_SAVE_PANEL	'SpSp'
#define HS_SHOW_PALETTE_OPEN_PANEL	'SpOp'
#define HS_PALETTE_SAVE_REFS	'PaSr'
#define HS_PALETTE_OPEN_REFS	'PaOr'
#define HS_DELETE_PALETTE		'DePa'

// this is used to inform palette-window that its container has a new selection
// and all other contaners are informed with this too
#define HS_PALETTE_SELECTION_CHANGED	'PslC'

// these constants are used in controlling the paint window
#define HS_RESIZE_WINDOW_TO_FIT 	'RwTF'
#define HS_IMAGE_SIZE_SET			'IsiS'

// these constants are used with palette_pop_up_menu
#define HS_PALETTE_ENTRY_SELECTED 	'PeSl'

// these constants are used with magnifying
#define HS_ZOOM_IMAGE_IN 			'ZImI'
#define HS_ZOOM_IMAGE_OUT			'ZImO'
#define	HS_SET_MAGNIFYING_SCALE		'SmgS'

// This constant informs about change in grid
#define HS_GRID_ADJUSTED			'Grdj'

// this constant is used to inform the window that mouse has been pressed down in background-view
#define HS_MOUSE_DOWN_IN_BACKGROUNDVIEW	'MdBV'


// These constants are used in controlling the status view.
#define	HS_DISPLAY_MESSAGE	'DisM'
#define	HS_DISPLAY_BUTTONS	'DisB'
#define	HS_NORMAL_DISPLAY	'NorD'
#define	HS_CHANGE_MESSAGE	'ChgM'
#define	HS_DISPLAY_PROGRESS	'DiPr'



// These constants are used in controlling the help-view.
#define	HS_TEMPORARY_HELP_MESSAGE	'ThlM'
#define	HS_TOOL_HELP_MESSAGE		'TolM'


// this constant is used to inform that tool has changed for
// one of the mouse-buttons
#define	HS_TOOL_CHANGED		'TlCh'


// this constant is used in a message from drawing-tool to inform the
// view that the tool is finished
#define	HS_DRAWING_TOOL_FINISHED	'DtFh'


// these constants are used when loading image or project files
#define	HS_SHOW_IMAGE_OPEN_PANEL	'SiOp'
#define	HS_SHOW_IMAGE_SAVE_PANEL	'SiSp'
#define	HS_IMAGE_SAVE_REFS			'ImSr'
#define	HS_SAVE_IMAGE				'Saim'
#define	HS_SHOW_PROJECT_OPEN_PANEL	'PrOp'
#define	HS_SHOW_PROJECT_SAVE_PANEL	'PrSp'
#define	HS_PROJECT_SAVE_REFS		'PrSr'
#define	HS_SAVE_PROJECT				'Sapr'
// these constants are for PaintWindow's and it's setup-window's
// communication
//#define	HS_AREA_COLOR_CHANGED			'AcoC'

// These are used for communication between file-panel and paint-window
#define	HS_SAVE_FORMAT_CHANGED		'SFCh'
#define	HS_SHOW_DATATYPE_SETTINGS	'SdtS'


// these constants are used in the Selection menu
#define	HS_INVERT_SELECTION			'InSl'
#define	HS_CLEAR_SELECTION			'ClSl'
#define HS_SELECT_ALL				'SlAl'
#define HS_GROW_SELECTION			'GrSl'
#define HS_SHRINK_SELECTION			'SrSL'
#define HS_HIDE_SELECTION_BORDERS	'HbSL'
#define HS_SELECT_LAYER_PIXELS		'SlPx'

// These constants are used to inform about deleting a layer or selection
#define HS_EDIT_DELETE			'Clla'

// This constant is used in a message that contains
// an int32 'mode' that describes the new mode for ImageView.
//#define	HS_CHANGE_MODE		'Chmd'

// These constants are used in Window-menu.
#define HS_SHOW_COLOR_WINDOW		'ShCw'
#define	HS_SHOW_LAYER_WINDOW		'ShLW'
#define HS_SHOW_TOOL_WINDOW			'ShtW'
#define HS_SHOW_TOOL_SETUP_WINDOW	'StsW'
#define	HS_SHOW_BRUSH_STORE_WINDOW	'Sbsw'

// these constants are used in the help-menu
#define	HS_SHOW_USER_DOCUMENTATION	'Sudc'


// This is the message constant that is sent from the status-view's
// OK- and Cancel-buttons.
//#define	HS_OPERATION_FINISHED	'OpFn'



#define	HS_UNDO	'unDo'
#define	HS_REDO	'reDo'


#define	HS_START_MANIPULATOR			'Stmi'
#define	HS_MANIPULATE_CURRENT_LAYER		'Mncl'
#define	HS_MANIPULATE_ALL_LAYERS		'Mnal'
#define	HS_MANIPULATE_NO_LAYER			'Mnnl'

#define	HS_MANIPULATOR_FINISHED			'MnFi'

#endif
