/*

	Filename:	SymbolImageServer.h
	Contents:	SymbolImageServer-class declarations.
	Author:	Heikki Suhonen

*/



#ifndef	SYMBOL_IMAGE_SERVER_H
#define	SYMBOL_IMAGE_SERVER_H

#include <Picture.h>
#include <Resources.h>

enum symbol_types {
	LEFT_ARROW,
	LEFT_ARROW_PUSHED,
	RIGHT_ARROW,
	RIGHT_ARROW_PUSHED,
	OK_BUTTON,
	OK_BUTTON_PUSHED,
	CANCEL_BUTTON,
	CANCEL_BUTTON_PUSHED,
	POP_UP_LIST,
	POP_UP_LIST_PUSHED
};

class SymbolImageServer {
static	BResources	*app_resource;

static	BPicture*	CreatePicture(int32,int32,int32);
static	BBitmap*	CreateBitmap(int32,int32,int32);

static	void		get_size_and_id(symbol_types,int32&,int32&,int32&);
static	void		initialize_resource();


public:
static	BPicture*	ReturnSymbolAsPicture(symbol_types,int32&,int32&);
static	BBitmap*	ReturnSymbolAsBitmap(symbol_types,int32&,int32&);
};


#endif
