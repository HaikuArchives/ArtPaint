/* 

	Filename:	ColorView.h
	Contents:	ColorView -class declarations	
	Author:		Heikki Suhonen
	
*/



/*	This class represents a view that accepts colordrops in the format
	defined by röColor. The format is following:
	
	struct roSColor {
		float m_Red;
		float m_Green;
		float m_Blue;
		float m_Alpha;
		float m_Hue;
	};
	
	All the values in the struct are normalized in the range between 0 and 1.0 .
	
	The message that is dropped on us has the following contents:

	"roColour"		is a struct roSColor	
	"RGBColor"		is a struct rgb_color
	"text/plain"	is a string of 7 chars like $FFFFFF
	
	The 'what' constant of message is B_PASTE.
	
		
	This view can also generate color-drags.
	
	Whenever color is dropped in this view, it will report the new value
	to the assigned target. For that reason this class will inherit from
	BControl.
	
*/

#ifndef COLOR_VIEW_H
#define COLOR_VIEW_H

#include <Control.h>
#include <StringView.h>

class ColorView : public BControl {
		rgb_color	current_color;
		BStringView	*label_view;
		float 		font_leading;
				
public:
		ColorView(BRect frame,char *label,BMessage *message,rgb_color initial_color);
		
void		MessageReceived(BMessage*);
void		MouseDown(BPoint);
void		Draw(BRect);
void		ResizeToPreferred();
void		SetColor(rgb_color);
rgb_color	GetColor() { return current_color; }	
};
#endif