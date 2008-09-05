/* 

	Filename:	FilePanels.h
	Contents:	Declarations for various file-panel classes	
	Author:		Heikki Suhonen
	
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