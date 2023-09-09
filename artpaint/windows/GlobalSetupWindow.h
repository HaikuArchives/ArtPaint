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
#ifndef GLOBAL_SETUP_WINDOW_H
#define GLOBAL_SETUP_WINDOW_H

#include <Control.h>
#include <Window.h>


class BTabView;


class GlobalSetupWindow : public BWindow {
public:
	static	void					ShowGlobalSetupWindow();
	static	void					CloseGlobalSetupWindow();

	virtual	void					MessageReceived(BMessage* message);

private:
									GlobalSetupWindow(const BPoint& leftTop);
	virtual							~GlobalSetupWindow();

private:
	class	WindowFeelView;
			WindowFeelView*			fWindowFeelView;

	class	UndoControlView;
			UndoControlView*		fUndoControlView;

	class	TransparencyControlView;
			TransparencyControlView*	fTransparencyControlView;

	class	MiscControlView;
			MiscControlView*		fMiscControlView;

			BTabView*				fTabView;
	static	GlobalSetupWindow*		fSetupWindow;
};


class PreviewPane : public BView {
public:
						PreviewPane(BRect frame);
	virtual				~PreviewPane();

	virtual void 		Draw(BRect updateRect);

			BBitmap*	previewBitmap() { return fPreviewBitmap; }

			void		Redraw() { Draw(Bounds()); }

private:
			BBitmap*	fPreviewBitmap;
};


class ColorSwatch : public BControl {
public:
						ColorSwatch(BRect frame, const char* name);
	virtual				~ColorSwatch();

	virtual void 		Draw(BRect updateRect);
	virtual void		MessageReceived(BMessage* message);
	virtual void		MouseDown(BPoint point);

			BBitmap*	swatchBitmap() { return fSwatchBitmap; }

			void		Redraw() { Draw(Bounds()); }

			void		SetColor(uint32 new_color);
			uint32		Color() { return fColor; }
private:
			BBitmap*	fSwatchBitmap;
			uint32		fColor;
};


#endif // GLOBAL_SETUP_WINDOW_H
