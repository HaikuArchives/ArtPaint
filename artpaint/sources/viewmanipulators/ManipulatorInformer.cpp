/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include "ManipulatorInformer.h"
#include "PaintApplication.h"

rgb_color ManipulatorInformer::GetForegroundColor()
{
	return ((PaintApplication*)be_app)->GetColor(true);
}


rgb_color ManipulatorInformer::GetBackgroundColor()
{
	return ((PaintApplication*)be_app)->GetColor(false);
}
