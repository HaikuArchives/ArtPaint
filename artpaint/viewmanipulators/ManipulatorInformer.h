/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MANIPULATOR_INFORMER_H
#define	MANIPULATOR_INFORMER_H

#include <GraphicsDefs.h>


class ManipulatorInformer {
public:
		rgb_color	GetForegroundColor();
		rgb_color	GetBackgroundColor();
};


#endif // MANIPULATOR_INFORMER_H
