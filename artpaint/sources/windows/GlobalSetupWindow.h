/*

	Filename:	GlobalSetupWindow.h
	Contents:	GlobalSetupWindow class declaration
	Author:		Heikki Suhonen

*/

#ifndef GLOBAL_SETUP_WINDOW_H
#define GLOBAL_SETUP_WINDOW_H


class WindowFeelView;
class UndoControlView;
class LanguageControlView;
class GeneralControlView;
class NumberControl;

#define HS_CLOSE_GLOBAL_SETTINGS_WINDOW	'Cgsw'
#define HS_WINDOW_FEEL_CHANGED			'Wflc'

#include <StringView.h>
#include <TabView.h>
#include <Window.h>
#include <RadioButton.h>
#include <CheckBox.h>

/*
	This class creates a window that is used to set up
	some global settings. These settings include the feel of
	tool-windows and things like that.
*/

class GlobalSetupWindow : public BWindow {
static	GlobalSetupWindow	*setup_window;
		WindowFeelView		*feel_view;
		UndoControlView		*undo_view;
		LanguageControlView	*language_view;
		GeneralControlView	*general_view;

		BTabView			*tab_view;
public:
		GlobalSetupWindow(BRect frame);
		~GlobalSetupWindow();

void	MessageReceived(BMessage*);
static	void	showGlobalSetupWindow();
static	void	closeGlobalSetupWindow();
};




class WindowFeelView : public BView {
		window_feel tool_window_feel;
		window_feel tool_setup_window_feel;
		window_feel palette_window_feel;
		window_feel	brush_window_feel;
		window_feel	layer_window_feel;
		window_feel	add_on_window_feel;

public:
		WindowFeelView(BRect);

void 	AttachedToWindow();
void	MessageReceived(BMessage*);

void	ApplyChanges();
};



#define	UNLIMITED_UNDO_SET	'Unlm'
#define	ADJUSTABLE_UNDO_SET	'Auns'
#define UNDO_DEPTH_ADJUSTED	'Udad'
#define UNDO_OFF			'unOf'

class UndoControlView : public BView {
		BRadioButton	*unlimited_depth_button;
		BRadioButton	*adjustable_depth_button;
		BRadioButton	*zero_depth_button;
		NumberControl	*undo_depth_control;

		int32			undo_depth;

public:
		UndoControlView(BRect);

void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ApplyChanges();
};


#define	ENGLISH_LANGUAGE_SET	'Enln'
#define	FINNISH_LANGUAGE_SET	'Filn'
#define	GERMAN_LANGUAGE_SET		'Grln'
#define	FRENCH_LANGUAGE_SET		'Frln'


class LanguageControlView : public BView {
		BRadioButton	*language_button_1;
		BRadioButton	*language_button_2;
		BRadioButton	*language_button_3;

		BStringView		*message_view;

		int32 language;
		int32 original_language;
public:
		LanguageControlView(BRect);

void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ApplyChanges();
};


#define	CURSOR_MODE_CHANGED	'Cmcn'
#define	CONFIRM_MODE_CHANGED	'Cofc'

class GeneralControlView : public BView {
		BRadioButton	*cursor_button_1;
		BRadioButton	*cursor_button_2;

		BCheckBox		*quit_confirm_box;

		int32	cursor_mode;
		int32	quit_confirm_mode;
public:
		GeneralControlView(BRect);

void	AttachedToWindow();
void	MessageReceived(BMessage*);
void	ApplyChanges();
};


#endif
