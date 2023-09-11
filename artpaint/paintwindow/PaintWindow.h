/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef PAINT_WINDOW_H
#define PAINT_WINDOW_H

#include "SettingsServer.h"

#include <Entry.h>
#include <Message.h>
#include <TranslatorRoster.h>
#include <Window.h>


class BBox;
class BFilePanel;
class BMenu;
class BNode;


class BackgroundView;
class Image;
class ImageView;
class ImageSavePanel;
class LayerWindow;
class StatusView;


namespace ArtPaint {
	namespace Interface {
		class NumberControl;
	}
}
using ArtPaint::Interface::NumberControl;


// views of a paint window
#define	HS_QUICK_TOOLS 	0x0001
#define	HS_HELP_VIEW	0x0002
#define	HS_STATUS_VIEW	0x0004
#define	HS_MENU_BAR		0x0008
#define HS_SIZING_VIEW	0x0010


enum menu_modes {
	FULL_MENU,
	MINIMAL_MENU,
	NO_IMAGE_MENU
};


class PaintWindow : public BWindow {
public:
	static	PaintWindow*	CreatePaintWindow(BBitmap* bitmap = NULL,
								const char* fileName = NULL,
								uint32 translatorType = 0,
								const entry_ref& entryRef = entry_ref(),
								translator_id outTranslator = 0);

	virtual					~PaintWindow();

	virtual	void			FrameResized(float newWidth, float newHeight);
	virtual	void			FrameMoved(BPoint newPosition);
	virtual	void			MenusBeginning();
	virtual	void			MenusEnded();
	virtual	void			MessageReceived(BMessage* message);
	virtual	bool			QuitRequested();
	virtual	void			WindowActivated(bool state);
	virtual	void			WorkspaceActivated(int32 workspace, bool state);
	virtual	void			Zoom(BPoint leftTop, float width, float height);

			void			DisplayCoordinates(BPoint point, BPoint reference,
								bool useReference);
			void			displayMag(float mag);

			void			SetHelpString(const char* string, int32 type);

			BMessage*		Settings() { return &fSettings; }

			// this function will read the attributes from a node, it is called
			// whenever an image is loaded
			void			ReadAttributes(const BNode& node);

	static  void			Redraw();

			// This returns the number of paint-windows
	static	int32			CountPaintWindows() { return sgPaintWindowCount; }

			// This function will create the ImageView. The width and height
			// parameters should reflect the actual size of image in pixels.
			status_t		OpenImageView(int32 width, int32 height);

			// This function will add the image-view to window's view hierarchy.
			// And finish its initialization.
			status_t		AddImageView();

			ImageView*		ReturnImageView() const { return fImageView; }

			void			SetImageEntry(const BEntry& entry) {
								fImageEntry = entry;
							}
			BEntry			ImageEntry() const { return fImageEntry; }

			void			SetProjectEntry(const BEntry& entry) {
								fProjectEntry = entry;
							}
			BEntry			ProjectEntry() const { return fProjectEntry; }

			StatusView*		ReturnStatusView() const { return fStatusView; }

			BMenuBar*		ReturnMenuBar() const { return fMenubar; }

private:
							PaintWindow(BRect frame, const char* name,
								uint32 views, const BMessage& settings);

			// this function will create the main menubar
			bool			openMenuBar();

			// this function will resize the window to fit image
			void			_ResizeToImage();
			BRect			_PreferredSize(Image* image) const;

			// These function will save the image.
	static	int32			save_image(void*);
			status_t		_SaveImage(BMessage *message);

			// These functions will save the project.
	static	int32			save_project(void*);
			status_t		_SaveProject(BMessage *message);

			// this function will write the attributes to node
			void			writeAttributes(BNode& node);

			void			_AddMenuItems(BMenu* menu, BString label,
								uint32 what, char shortcut, uint32 modifiers,
								BHandler* target, BString help);
			void			_AddRecentMenuItems(BMenu* menu,
								const StringList* list);
	static	int32			_AddAddOnsToMenu(void*);

			void			_ChangeMenuMode(menu_modes newMode);
			void			_EnableMenuItems(BMenu* menu);
			void			_DisableMenuItem(const char* label);

private:
			BMessage		fSettings;
			ImageView*		fImageView;
			BackgroundView*	fBackground;

			BScrollBar*		fVerticalScrollbar;
			BScrollBar*		fHorizontalScrollbar;

			BMenuBar*		fMenubar;
			StatusView*		fStatusView;
			BMenu*			fRecentImages;
			BMenu*			fRecentProjects;

			BBox*			fContainerBox;
			BButton*		fSetSizeButton;
			NumberControl*	fWidthNumberControl;
			NumberControl*	fHeightNumberControl;

			ImageSavePanel*	fImageSavePanel;
			BFilePanel*		fProjectSavePanel;

			BWindow*		fImageSizeWindow;

			// The BEntrys should be replace by something else as they
			// consume a file-descriptor.
			BEntry			fImageEntry;
			BEntry			fProjectEntry;
			uint32			fCurrentHandler;

			// This list contains pointers to all of the paint-windows
			// This counter keeps count of how many paint-windows we have open,
			// it is incremented in constructor and dectremented in destructor.
			static	BList	sgPaintWindowList;
			static	int32	sgPaintWindowCount;
			static	int32	sgUntitledWindowNumber;

			// these variables hold the info about how much space
			// all the additional views take, used when resizing the window
			// to fit image
			BRect			fUserFrame;
};


#endif	// PAINT_WINDOW_H
