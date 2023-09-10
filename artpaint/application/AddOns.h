/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef ADD_ONS_H
#define	ADD_ONS_H


class BBitmap;
class Manipulator;
class ManipulatorInformer;


enum add_on_types {
	BLUR_FILTER_ADD_ON,
	SHARPEN_FILTER_ADD_ON,
	EFFECT_FILTER_ADD_ON,
	DISTORT_ADD_ON,
	COLOR_ADD_ON,
	ANALYZER_ADD_ON,
	GENERIC_ADD_ON
};


extern "C" Manipulator* instantiate_add_on(BBitmap*, ManipulatorInformer*);


#endif // ADD_ONS_H
