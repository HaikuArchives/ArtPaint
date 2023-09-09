/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef PALETTE_WINDOW_CLIENT_H
#define	PALETTE_WINDOW_CLIENT_H

#include "ColorPalette.h"
#include <stdio.h>


class PaletteWindowClient {
public:
					PaletteWindowClient() { ColorPaletteWindow::AddPaletteWindowClient(this); }
	virtual			~PaletteWindowClient() { ColorPaletteWindow::RemovePaletteWindowClient(this); }

	virtual	void	PaletteColorChanged(const rgb_color&) = 0;
};


#endif // PALETTE_WINDOW_CLIENT_H
