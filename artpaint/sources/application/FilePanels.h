/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef FILE_PANELS_H
#define FILE_PANELS_H

#include <FilePanel.h>

class ImageSavePanel : public BFilePanel {
public:
		ImageSavePanel(entry_ref *directory, BMessenger *target, int32 save_format, BBitmap *saved_bitmap=NULL);
virtual	~ImageSavePanel();
};

#endif
