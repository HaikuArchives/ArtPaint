/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *
 */

#include "NumberControl.h"


#include <stdlib.h>


namespace ArtPaint {
	namespace Interface {

NumberControl::NumberControl(const char* label, const char* text,
		BMessage* message, int32 maxBytes, bool allowNegative, bool continuos)
	: BTextControl(label, text, message)
{
	_InitControl(maxBytes, allowNegative, continuos);
}


int32
NumberControl::Value() const
{
	return atoi(Text());
}


void
NumberControl::SetValue(int32 value)
{
	BTextControl::SetValue(value);

	BString text;
	text << value;

	SetText(text.String());
}


void
NumberControl::AttachedToWindow()
{
	BTextControl::AttachedToWindow();
}


void
NumberControl::_InitControl(int32 maxBytes, bool allowNegative, bool continuos)
{
	for (uint32 i = 0; i < 256; ++i)
		TextView()->DisallowChar(i);

	TextView()->AllowChar('0');
	TextView()->AllowChar('1');
	TextView()->AllowChar('2');
	TextView()->AllowChar('3');
	TextView()->AllowChar('4');
	TextView()->AllowChar('5');
	TextView()->AllowChar('6');
	TextView()->AllowChar('7');
	TextView()->AllowChar('8');
	TextView()->AllowChar('9');

	if (allowNegative)
		TextView()->AllowChar('-');

	TextView()->SetMaxBytes(maxBytes);
	SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);
}

	}	// namespace Interface
}	// namespace ArtPaint
