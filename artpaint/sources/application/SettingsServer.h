/*
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef SETTINGSSERVER_H
#define SETTINGSSERVER_H

#include <Locker.h>
#include <Message.h>


#include <list>


class BString;


typedef std::list<BString> StringList;


class SettingsServer
{
	friend class PaintApplication;

public:
	static	SettingsServer*		Instance();

			void				Sync();

			status_t			GetWindowSettings(BMessage* message);
			void				SetWindowSettings(const BMessage& message);
			status_t			GetDefaultWindowSettings(BMessage* message);

			status_t			GetApplicationSettings(BMessage* message);
			void				SetApplicationSettings(const BMessage& message);
			status_t			GetDefaultApplicationSettings(BMessage* message);

			const StringList&	RecentImagePaths() const;
			void				AddRecentImagePath(const BString& path);

			const StringList&	RecentProjectPaths() const;
			void				AddRecentProjectPath(const BString& path);

private:
								SettingsServer();
								SettingsServer(const SettingsServer& server);
								~SettingsServer();

	static	SettingsServer*		Instantiate();
	static	void				DestroyServer();

			status_t			_ReadSettings(const BString& name,
									BMessage& message);
			void				_InsertRecentPath(const BString& path,
									StringList& list);

private:
			BMessage			fWindowSettings;
			BMessage			fDefaultWindowSettings;

			BMessage			fApplicationSettings;
			BMessage			fDefaultApplicationSettings;

			StringList			fRecentImagePaths;
			StringList			fRecentProjectPaths;

	static	BLocker				fLocker;
	static	SettingsServer*		fSettingsServer;
};

#endif // SETTINGSSERVER_H
