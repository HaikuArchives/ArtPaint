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
#ifndef NUMBERCONTROL_H
#define NUMBERCONTROL_H

#include <TextControl.h>


namespace ArtPaint {
	namespace Interface {

class NumberControl : public BTextControl {

public:
						NumberControl(const char* label, const char* text,
							BMessage* message, int32 maxBytes = 5,
							bool allowNegative = false, bool continuos = true);


			int32		Value() const;
	virtual	void		SetValue(int32 value);

	virtual	void		AttachedToWindow();

private:
			void		_InitControl(int32 maxBytes, bool allowNegative,
							bool continuos);
};

	}	// namespace Interface
}	// namespace ArtPaint

#endif // NUMBERCONTROL_H
