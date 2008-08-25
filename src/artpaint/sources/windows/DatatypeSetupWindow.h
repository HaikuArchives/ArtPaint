/* 

	Filename:	DatatypeSetupWindo.h
	Contents:	Declaration for DatatypeSetupWindow-class	
	Author:		Heikki Suhonen
	
*/
#ifndef DATATYPE_SETUP_WINDOW_H
#define DATATYPE_SETUP_WINDOW_H

#include <TranslatorRoster.h>
#include <Window.h>

class BWindow;
class DatatypeSetupWindow : public BWindow {

static	BWindow	*setup_window;
		BView	*container;		

			
				DatatypeSetupWindow();
				~DatatypeSetupWindow();
public:
static	void	ChangeHandler(translator_id);	
static	void	showWindow(translator_id);
};

#endif