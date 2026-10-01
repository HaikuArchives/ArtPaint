/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef ADD_ON_TYPES_H
#define	ADD_ON_TYPES_H

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Categories"


inline const char* add_on_category[GENERIC_ADD_ON + 1] = {
	B_TRANSLATE("Artistic"),
	B_TRANSLATE("Blur"),
	B_TRANSLATE("Color"),
	B_TRANSLATE("Distort"),
	B_TRANSLATE("Edges"),
	B_TRANSLATE("Enhance"),
	B_TRANSLATE("Noise"),
	B_TRANSLATE("Render"),
	B_TRANSLATE("Generic")
};


#endif // ADD_ON_TYPES_H
