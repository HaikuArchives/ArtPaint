/*

	Filename:	PaletteWindowClient.h
	Contents:	PaletteWindowClient-class declarations.
	Author:		Heikki Suhonen

*/



#ifndef PALETTE_WINDOW_CLIENT_H
#define	PALETTE_WINDOW_CLIENT_H


#include "ColorPalette.h"
#include <stdio.h>

class PaletteWindowClient {
public:
				PaletteWindowClient() {
					//printf("PaletteWindowClient\n");
					ColorPaletteWindow::AddPaletteWindowClient(this);
				}

virtual			~PaletteWindowClient() {
				//	printf("~PaletteWindowClient\n");
					ColorPaletteWindow::RemovePaletteWindowClient(this);
				}

virtual	void	PaletteColorChanged(const rgb_color&) = 0;
};

#endif
