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


class BFile;
class BFilePanel;
class BPath;
class ColorPaletteWindow;
class ColorSet;
class DrawingTool;
struct entry_ref;


class PaintApplication : public BApplication {
public:
								PaintApplication();
	virtual						~PaintApplication();

			void				AboutRequested();
	virtual	void				ReadyToRun();
	virtual	bool				QuitRequested();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				MessageReceived(BMessage* message);

			rgb_color			Color(bool foreground) const;
			void				SetColor(rgb_color color, bool foreground);

			bool				ShuttingDown() const;

	static	void				HomeDirectory(BPath& path);

private:
			void				_InstallMimeType();
			void				_ReadPreferences();
			void				_WritePreferences();

			void				_ShowAlert(const BString& text);
			const char*			_OpenPath(const BMessage* message,
									const entry_ref& ref);
	static	BPath				_GetParentPath(const entry_ref& entryRef);

			status_t			_ReadProject(BFile& file, entry_ref& ref);
			status_t			_ReadProjectOldStyle(BFile& file, entry_ref& ref);

private:
			BFilePanel*			fImageOpenPanel;
			BFilePanel*			fProjectOpenPanel;
			bool				fShuttingDown;
};

#endif
