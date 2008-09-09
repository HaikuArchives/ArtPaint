/*

	Filename:	TextManipulator.h
	Contents:	TextManipulator-class declaration
	Author:		Heikki Suhonen

*/


#ifndef TEXT_MANIPULATOR_H
#define TEXT_MANIPULATOR_H

#include "WindowGUIManipulator.h"
#include "Controls.h"
#include "ColorView.h"
#include "PaletteWindowClient.h"

#define TEXT_CHANGED							'Txch'
#define FONT_FAMILY_CHANGED						'Ffch'
#define	FONT_STYLE_CHANGED						'Fsch'
#define FONT_SIZE_CHANGED						'Fsic'
#define FONT_ROTATION_CHANGED					'Froc'
#define FONT_ROTATION_ADJUSTING_STARTED			'Fras'
#define	FONT_SHEAR_CHANGED						'Fshc'
#define FONT_SHEAR_ADJUSTING_STARTED			'Fsas'
#define FONT_COLOR_CHANGED						'Fcoc'
#define FONT_ANTI_ALIAS_CHANGED					'Faac'
#define FONT_TRANSPARENCY_CHANGED				'Ftpc'
#define FONT_TRANSPARENCY_ADJUSTING_STARTED		'Ftas'

#define TEXT_SETTINGS_VERSION	0x03


class BBitmap;
class BCheckBox;
class BMenu;
class BMenuField;
class BView;


class TextManipulatorSettings : public ManipulatorSettings {
public:
		TextManipulatorSettings();
		TextManipulatorSettings(const TextManipulatorSettings&);
virtual	~TextManipulatorSettings();

		bool operator==(const TextManipulatorSettings&);
		bool operator!=(const TextManipulatorSettings&);
		TextManipulatorSettings& operator=(const TextManipulatorSettings&);

char		*text;
rgb_color	*text_color_array;
int32		text_array_length;

BPoint 		starting_point;
BFont		font;
};


class TextManipulatorView;

class TextManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b,Selection* s,BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, s, stb); };

	BBitmap	*preview_bitmap;
	BBitmap	*copy_of_the_preview_bitmap;

	BRegion	previously_updated_region;

	TextManipulatorSettings	settings;
	TextManipulatorSettings previous_settings;
	BPoint					origo;

	TextManipulatorView	*config_view;

	int32	last_used_quality;
	int32	lowest_allowed_quality;

public:
			TextManipulator(BBitmap*);
			~TextManipulator();

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion *updated_region=NULL);
BView*		MakeConfigurationView(BMessenger*);

void		MouseDown(BPoint,uint32,BView*,bool);
BRegion		Draw(BView*,float);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
const	char*	ReturnHelpString();
const	char*	ReturnName();

ManipulatorSettings*	ReturnSettings();

void		ChangeSettings(ManipulatorSettings*);
void		SetStartingPoint(BPoint point) { settings.starting_point = point; }


status_t	ReadSettings(BNode*);
status_t	WriteSettings(BNode*);
};


class TextEditor;

class TextManipulatorView : public WindowGUIManipulatorView {
		TextManipulator	*manipulator;
		BMessenger		*target;

		TextManipulatorSettings	settings;

//		BTextControl			*text_control;
		TextEditor				*text_view;
		BMenu					*font_menu;
		BMenuField				*font_menu_field;
		ControlSliderBox		*size_slider;
		ControlSliderBox		*rotation_slider;
		ControlSliderBox		*shear_slider;
//		ControlSliderBox		*transparency_slider;
//		ColorView				*color_view;
		BCheckBox				*anti_aliasing_box;

		bool					preview_started;

void	FontFamilyAndStyleChanged(uint32);

public:
		TextManipulatorView(BRect,TextManipulator*,BMessenger*);

void	AttachedToWindow();
void	AllAttached();
void	MessageReceived(BMessage*);

void	ChangeSettings(TextManipulatorSettings*);
};


// This class is like a regular BTextView, but it also reports whenever the text
// is changed. It also allows the direct manipulation of text-color with the help
// of the ColorPaletteWindow.
class TextEditor : public BTextView, PaletteWindowClient {
public:
						TextEditor(BRect rect);
	virtual				~TextEditor();

			void		PaletteColorChanged(const rgb_color& color);

			void		SetMessage(BMessage* message);
			void		SetTarget(const BMessenger& target);

protected:
			void		InsertText(const char* text, int32 length, int32 offset,
							const text_run_array* runs);
			void		DeleteText(int32 start, int32 finish);

private:
			void		_SendMessage();

private:
		BMessage*		fMessage;
		BMessenger		fTarget;
};

#endif
