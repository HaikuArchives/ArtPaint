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

/*
	The class BrushEditor allows user to edit brushes. It will display a preview
	of the brush and necessary controls for modifying it. A brush-editor has one
	brush being edited at a time. It accepts and generates brush drags & drops.
	The brush can be queried from the editor and it can be copied to another
	object. BrushEditor tries to fit itself inside the parameter rectangle, but
	if it does not fit it will be as small as possible.

*/

#ifndef BRUSH_EDITOR_H
#define BRUSH_EDITOR_H

#include <Box.h>
#include <View.h>


#include "Brush.h"


class BButton;
class BCheckBox;
class BRadioButton;
class BrushView;


namespace ArtPaint {
	namespace Interface {
		class NumberSliderControl;
	}
}
using ArtPaint::Interface::NumberSliderControl;


class BrushEditor : public BBox {
public:
	virtual	void					AttachedToWindow();
	virtual	void					MessageReceived(BMessage* message);

	static	void					BrushModified();
	static	BView*					CreateBrushEditor(Brush* brush);

private:
									BrushEditor(Brush* brush);
	virtual							~BrushEditor();

private:
			Brush*					fBrush;
			BrushView*				fBrushView;
			brush_info				fBrushInfo;

			NumberSliderControl*	fBrushWidth;
			NumberSliderControl*	fBrushHeight;
			NumberSliderControl*	fBrushAngle;
			NumberSliderControl*	fBrushFade;
			BCheckBox*				fLockDimensions;
			BRadioButton*			fRectangle;
			BRadioButton*			fEllipse;
			BButton*				fStoreBrush;
			BButton*				fResetBrush;

	static	BrushEditor*			fBrushEditor;
};


class BrushView : public BView {
public:
									BrushView(BRect frame, Brush* brush);
	virtual							~BrushView();

	virtual	void					Draw(BRect updateRect);
	virtual	void					MessageReceived(BMessage* message);
	virtual	void					MouseDown(BPoint where);
	virtual	void					MouseMoved(BPoint where, uint32 transit,
										const BMessage* messge);

			void					BrushModified();
			void					ChangeBrush(Brush* brush);

private:
			bool					fDrawControls;

			Brush*					fBrush;
			BBitmap*				fBrushPreview;
};

#endif	// BRUSH_EDITOR_H
