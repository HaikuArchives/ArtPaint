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
	ARTISTIC_FILTER_ADD_ON,
	BLUR_FILTER_ADD_ON,
	COLOR_ADD_ON,
	DISTORT_ADD_ON,
	EDGE_DETECT_FILTER_ADD_ON,
	ENHANCE_FILTER_ADD_ON,
	NOISE_FILTER_ADD_ON,
	RENDERING_FILTER_ADD_ON,
	GENERIC_ADD_ON
};


extern "C" Manipulator* instantiate_add_on(BBitmap*, ManipulatorInformer*);


#endif // ADD_ONS_H
