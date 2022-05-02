/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef HALFTONE_H
#define HALFTONE_H

#include "Manipulator.h"

class ManipulatorInformer;


#define	ROUND_DOT_SIZE		8
#define	DIAGONAL_LINE_SIZE	5
#define	ORDERED_MATRIX_SIZE	4

class Halftone : public Manipulator {
	const	int32	round_dot_size;
	const	int32	diagonal_line_size;
	const	int32	ordered_matrix_size;

			uint32	round_dot_pattern[ROUND_DOT_SIZE][ROUND_DOT_SIZE];
			uint32	diagonal_line_pattern[DIAGONAL_LINE_SIZE][DIAGONAL_LINE_SIZE];
			uint32	ordered_matrix[ORDERED_MATRIX_SIZE][ORDERED_MATRIX_SIZE];

ManipulatorInformer	*informer;

	BBitmap*	round_dot_halftone(BBitmap*,Selection*,BStatusBar*);
	BBitmap*	diagonal_line_halftone(BBitmap*,Selection*,BStatusBar*);
	BBitmap*	ordered_dither_halftone(BBitmap*,Selection*,BStatusBar*);
	BBitmap*	fs_dither_halftone(BBitmap*,Selection*,BStatusBar*);
	BBitmap*	ncandidate_dither_halftone(BBitmap*,Selection*,BStatusBar*);

public:
			Halftone(ManipulatorInformer*);
			~Halftone();

BBitmap*	ManipulateBitmap(BBitmap*,Selection*,BStatusBar*);
const char*	ReturnHelpString();
const char*	ReturnName();
};
#endif
