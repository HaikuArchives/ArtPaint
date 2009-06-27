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
#ifndef FILE_PANELS_H
#define FILE_PANELS_H

#include <FilePanel.h>

class BBitmap;
class BMessenger;

struct entry_ref;

class ImageSavePanel : public BFilePanel {
public:
				ImageSavePanel(const entry_ref& startDir, BMessenger& target,
					int32 saveFormat, BBitmap* savedBitmap = NULL);
	virtual			~ImageSavePanel();
};

#endif
