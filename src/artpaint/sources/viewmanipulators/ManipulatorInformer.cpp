/* 

	Filename:	ManipulatorInformer.cpp
	Contents:	ManipulatorInformer-class definitions		
	Author:		Heikki Suhonen
	
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