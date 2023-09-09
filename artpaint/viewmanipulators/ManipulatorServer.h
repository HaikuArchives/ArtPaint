/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#ifndef MANIPULATOR_SERVER_H
#define MANIPULATOR_SERVER_H

#include "Manipulator.h"

#include <Locker.h>
#include <String.h>

#include <list>
#include <set>


class BNode;
class BPath;


typedef std::set<BString> StringSet;
typedef std::list<image_id> ImageList;


class ManipulatorServer {
	friend class PaintApplication;

public:
	static	ManipulatorServer*		Instance();

			Manipulator*			ManipulatorFor(manipulator_type type,
										int32 imageId = -1) const;
			void					StoreManipulatorSettings(Manipulator*);

			bool					AddOnsLoaded() const { return fAddonsLoaded; }
			int32					AddOnCount() const { return fAddOnImages.size(); }
			const ImageList&		AddOnImageList() const { return fAddOnImages; }

private:
									ManipulatorServer();
									ManipulatorServer(const ManipulatorServer& server);
									~ManipulatorServer();

	static	ManipulatorServer*		Instantiate();
	static	void					DestroyServer();

	static	status_t				_AddOnLoaderThread(void* data);
			void					_LoadAddOns(const BPath& path);
			status_t				_GetNodeFor(image_id imageId, BNode* node) const;

private:
			StringSet				fAddonNames;
			ImageList				fAddOnImages;
			bool					fAddonsLoaded;

	static	BLocker					fLocker;
	static	ManipulatorServer*		fManipulatorServer;
};


#endif	// MANIPULATOR_SERVER_H
