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
#ifndef PAINT_APPLICATION_H
#define PAINT_APPLICATION_H

#include <Application.h>
#include <MessageFilter.h>

class BFile;
class BFilePanel;
class BPath;

class ColorPaletteWindow;
class ColorSet;
class DrawingTool;

struct entry_ref;
struct global_settings;


class PaintApplication : public BApplication {
public:
								PaintApplication();
	virtual						~PaintApplication();

	virtual	void				ReadyToRun();
	virtual	bool				QuitRequested();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				MessageReceived(BMessage* message);

			rgb_color			Color(bool foreground) const;
			void				SetColor(rgb_color color, bool foreground);


	static	void				HomeDirectory(BPath& path);
			global_settings*	GlobalSettings() { return fGlobalSettings; }

private:
			void				_ReadPreferences();
			void				_WritePreferences();

			void				_ShowAlert(const BString& text);
			void				_StorePath(const BMessage* message,
									const entry_ref& ref, char* target);
	static	BPath				_GetParentPath(const entry_ref& entryRef);

			status_t			_ReadProject(BFile& file, entry_ref& ref);
			status_t			_ReadProjectOldStyle(BFile& file, entry_ref& ref);

private:
			BFilePanel*			fImageOpenPanel;
			BFilePanel*			fProjectOpenPanel;
			global_settings*	fGlobalSettings;
};

filter_result AppKeyFilterFunction(BMessage* message, BHandler** handler,
	BMessageFilter* filter);

#endif
