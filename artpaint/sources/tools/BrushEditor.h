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
	The class BrushEditor allows user to edit brushes. It will
	display a preview of the brush and necessary controls for modifying it.
	A brush-editor has one brush being edited at a time. It accepts and
	generates brush drags and drops. The brush can be queried from the editor
	and it can be copied to another object. BrushEditor tries to fit itself
	inside the parameter rectangle, but if it does not fit it will be as small
	as possible.

*/

#ifndef BRUSH_EDITOR_H
#define BRUSH_EDITOR_H

#include <Box.h>
#include <View.h>


#include "Brush.h"


class BButton;
class BRadioButton;
class BrushView;
class ControlSliderBox;


#define	BRUSH_ALTERED			'Bral'


class BrushEditor : public BBox {
public:
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage *message);

	static	void				BrushModified();
	static	BView*				CreateBrushEditor(Brush* brush);

private:
								BrushEditor(Brush* brush);
								~BrushEditor();

private:
			Brush*				fBrush;
			BrushView*			fBrushView;
			brush_info			fBrushInfo;

			ControlSliderBox*	fWidthSlider;
			ControlSliderBox*	fHeightSlider;
			ControlSliderBox*	fFadeSlider;
			BRadioButton*		fRectangle;
			BRadioButton*		fEllipse;
			BButton*			fStoreBrush;

	static	BrushEditor*		fBrushEditor;
};



class BrushView : public BView {
public:
								BrushView(BRect, Brush*);
	virtual						~BrushView();

	virtual	void				Draw(BRect);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint);
	virtual	void				MouseMoved(BPoint,uint32, const BMessage*);

			void				BrushModified();
			void				ChangeBrush(Brush*);

private:
			bool				fDrawControls;

			Brush*				fBrush;
			BBitmap*			fBrushPreview;
};

#endif
