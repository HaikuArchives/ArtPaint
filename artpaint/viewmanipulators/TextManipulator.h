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
#ifndef TEXT_MANIPULATOR_H
#define TEXT_MANIPULATOR_H

#include "ColorView.h"
#include "ManipulatorSettings.h"
#include "PaletteWindowClient.h"
#include "WindowGUIManipulator.h"


#include <TextView.h>


class BBitmap;
class BCheckBox;
class BMenu;
class BMenuField;
class BView;
class Selection;
class TextManipulatorView;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


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


class TextManipulator : public WindowGUIManipulator {
	BBitmap*	ManipulateBitmap(BBitmap* b, Selection* s, BStatusBar* stb)
	{ return WindowGUIManipulator::ManipulateBitmap(b, s, stb); }

	BBitmap	*preview_bitmap;
	BBitmap	*copy_of_the_preview_bitmap;

	BRegion	previously_updated_region;

	TextManipulatorSettings	fSettings;
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
	BView*		MakeConfigurationView(const BMessenger& target);

	void		MouseDown(BPoint,uint32,BView*,bool);
	BRegion		Draw(BView*,float);
	void		Reset(Selection*);
	void		SetPreviewBitmap(BBitmap*);
	const	char*	ReturnHelpString();
	const	char*	ReturnName();

	ManipulatorSettings*	ReturnSettings();

	void		ChangeSettings(ManipulatorSettings*);
	void		SetStartingPoint(BPoint point) { fSettings.starting_point = point; }

	status_t	Save(BMessage& settings) const;
	status_t	Restore(const BMessage& settings);
};


// This class is like a regular BTextView, but it also reports whenever the text
// is changed. It also allows the direct manipulation of text-color with the help
// of the ColorPaletteWindow.
class TextEditor : public BTextView, PaletteWindowClient {
public:
							TextEditor();
	virtual					~TextEditor();

			void			PaletteColorChanged(const rgb_color& color);

			void			SetMessage(BMessage* message);
			void			SetTarget(const BMessenger& target);

protected:
			void			InsertText(const char* text, int32 length,
								int32 offset, const text_run_array* runs);
			void			DeleteText(int32 start, int32 finish);

private:
			void			_SendMessage();

private:
			BMessage*		fMessage;
			BMessenger		fTarget;
};


class TextManipulatorView : public WindowGUIManipulatorView {
public:
									TextManipulatorView(TextManipulator*,
										const BMessenger& target);
	virtual							~TextManipulatorView() {}

	virtual	void					AllAttached();
	virtual	void					AttachedToWindow();
	virtual	void					MessageReceived(BMessage* message);

			void					ChangeSettings(TextManipulatorSettings* s);

private:
			void					_FontFamilyAndStyleChanged(uint32 fontCode);

private:
			BMessenger				fTarget;
			TextManipulatorSettings	fSettings;
			bool					fTracking;

			TextManipulator*		fManipulator;


			TextEditor*				fTextView;
			BMenu*					fFontMenu;
			BMenuField*				fFontMenuField;

			NumberSliderControl*	fSizeControl;
			NumberSliderControl*	fRotationControl;
			NumberSliderControl*	fShearControl;
			BCheckBox*				fAntiAliasing;
};

#endif	// TEXT_MANIPULATOR_H
