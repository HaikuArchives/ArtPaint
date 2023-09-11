/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "FloatControl.h"


#include <String.h>


#include <stdlib.h>


namespace ArtPaint {
	namespace Interface {


FloatControl::FloatControl(const char* label, const char* text, BMessage* message, int32 maxBytes,
	bool allowNegative, bool continuous)
	:
	BTextControl(label, text, message)
{
	_InitControl(maxBytes, allowNegative, continuous);
}


float
FloatControl::Value() const
{
	return atof(Text());
}


void
FloatControl::SetValue(float value)
{
	BTextControl::SetValue(value);

	BString text;
	text << value;

	SetText(text.String());
}


void
FloatControl::_InitControl(int32 maxBytes, bool allowNegative, bool continuous)
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
	TextView()->AllowChar('.');
	TextView()->AllowChar(','); // should localize

	if (allowNegative)
		TextView()->AllowChar('-');

	TextView()->SetMaxBytes(maxBytes);
	SetAlignment(B_ALIGN_LEFT, B_ALIGN_RIGHT);

	SetWidthInBytes(maxBytes);
}


void
FloatControl::SetWidthInBytes(uint32 bytes)
{
	BFont font;
	TextView()->SetExplicitMinSize(BSize(font.StringWidth("D") * bytes, B_SIZE_UNSET));
	TextView()->SetExplicitMaxSize(BSize(font.StringWidth("D") * bytes, B_SIZE_UNSET));
}


	} // namespace Interface
} // namespace ArtPaint
