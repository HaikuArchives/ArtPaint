/*
 * Copyright 2022, Dale Cieslak
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#ifndef FLOATCONTROL_H
#define FLOATCONTROL_H

#include <TextControl.h>


namespace ArtPaint {
	namespace Interface {

class FloatControl : public BTextControl {

public:
						FloatControl(const char* label, const char* text,
							BMessage* message, int32 maxBytes = 5,
							bool allowNegative = false, bool continuous = true);

			float		Value() const;
	virtual	void		SetValue(float value);
			void		SetWidthInBytes(uint32 bytes);

	virtual	void		AttachedToWindow();

private:
			void		_InitControl(int32 maxBytes, bool allowNegative,
							bool continuous);
};

	}	// namespace Interface
}	// namespace ArtPaint

#endif // FLOATCONTROL_H
