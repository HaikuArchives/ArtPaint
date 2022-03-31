/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef _INTERFERENCE_H
#define _INTERFERENCE_H

#include <stdio.h>
#include <Messenger.h>

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


class InterferenceManipulatorSettings : public ManipulatorSettings {
public:
		InterferenceManipulatorSettings()
			: ManipulatorSettings() {
			centerA = BPoint(0,0);
			waveLengthA = 30;
			centerB = BPoint(0,0);
			waveLengthB = 30;
		}

		InterferenceManipulatorSettings(const InterferenceManipulatorSettings& s)
			: ManipulatorSettings() {
			centerA = s.centerA;
			waveLengthA = s.waveLengthA;
			centerB = s.centerB;
			waveLengthB = s.waveLengthB;
		}


		InterferenceManipulatorSettings& operator=(const InterferenceManipulatorSettings& s) {
			centerA = s.centerA;
			waveLengthA = s.waveLengthA;
			centerB = s.centerB;
			waveLengthB = s.waveLengthB;
			return *this;
		}


		bool operator==(InterferenceManipulatorSettings s) {
			return ((centerA == s.centerA) && (waveLengthA == s.waveLengthA) &&
					(centerB == s.centerB) && (waveLengthB == s.waveLengthB));
		}

		BPoint	centerA;
		BPoint	centerB;
		float	waveLengthA;
		float	waveLengthB;
};


class InterferenceManipulatorView;

class InterferenceManipulator : public WindowGUIManipulator {
BBitmap				*preview_bitmap;
BBitmap				*copy_of_the_preview_bitmap;


InterferenceManipulatorSettings	settings;
InterferenceManipulatorSettings	previous_settings;

InterferenceManipulatorView		*config_view;
float							*sin_table;
bool							livePreview;

void		MakeInterference(BBitmap*,InterferenceManipulatorSettings*,Selection*);

public:
			InterferenceManipulator(BBitmap*);
			~InterferenceManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString() { return "Click on the image to move the wave centers and adjust wave-lengths. "; }
char*		ReturnName() { return "Interference"; }

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger& target);

void		ChangeSettings(ManipulatorSettings*);
};



class InterferenceManipulatorView : public WindowGUIManipulatorView {
		BMessenger				*target;
		InterferenceManipulator			*manipulator;
//		ControlSlider			*waveLengthSliderA;
//		ControlSlider			*waveLengthSliderB;
		InterferenceManipulatorSettings	settings;

		bool					preview_started;

public:
		InterferenceManipulatorView(BRect,InterferenceManipulator*, const BMessenger&);
		~InterferenceManipulatorView() { delete target; }

void	AttachedToWindow();
void	AllAttached();
void	MessageReceived(BMessage*);
void	ChangeSettings(InterferenceManipulatorSettings *s);
};


#ifdef __POWERPC__
inline 	asm	float reciprocal_of_square_root(float number)
{
	machine		604
	frsqrte		fp1,number;	// Estimates reciprocal of square-root
	blr
}
#else
float reciprocal_of_square_root(float number)
{
	return 1.0 / sqrt(number);
}
#endif


/*
 * A High Speed, Low Precision Square Root by Paul Lalonde and Robert Dawson
 * from "Graphics Gems", Academic Press, 1990
 *
 * A fast square root program adapted from the code of Paul Lalonde and
 * Robert Dawson in Graphics Gems I. Most likely written by Hill, Steve
 *
 * The Graphics Gems code is copyright-protected. In other words, you cannot
 * claim the text of the code as your own and resell it. Using the code
 * is permitted in any program, product, or library, non-commercial or
 * commercial.
 *
 * see also: http://tog.acm.org/GraphicsGems
 *
 * The format of IEEE double precision floating point numbers is:
 * SEEEEEEEEEEEMMMM MMMMMMMMMMMMMMMM MMMMMMMMMMMMMMMM MMMMMMMMMMMMMMMM
 *
 * S = Sign bit for whole number
 * E = Exponent bit (exponent in excess 1023 form)
 * M = Mantissa bit
 *
 */
#define SQRT_TABLE
#ifdef SQRT_TABLE

/* MOST_SIG_OFFSET gives the (int *) offset from the address of the double
 * to the part of the number containing the sign and exponent.
 * You will need to find the relevant offset for your architecture.
 */

#define MOST_SIG_OFFSET 1

/* SQRT_TAB_SIZE - the size of the lookup table - must be a power of four.
 */

#define SQRT_TAB_SIZE 16384

/* MANT_SHIFTS is the number of shifts to move mantissa into position.
 * If you quadruple the table size subtract two from this constant,
 * if you quarter the table size then add two.
 * Valid values are: (16384, 7) (4096, 9) (1024, 11) (256, 13)
 */

#define MANT_SHIFTS   7

#define EXP_BIAS   1023       /* Exponents are always positive     */
#define EXP_SHIFTS 20         /* Shifs exponent to least sig. bits */
#define EXP_LSB    0x00100000 /* 1 << EXP_SHIFTS                   */
#define MANT_MASK  0x000FFFFF /* Mask to extract mantissa          */

int        sqrt_tab[SQRT_TAB_SIZE];

void
init_sqrt_tab()
{
        int           i;
        double        f;
        unsigned int  *fi = (unsigned int *) &f + MOST_SIG_OFFSET;

        for (i = 0; i < SQRT_TAB_SIZE/2; i++)
        {
                f = 0; /* Clears least sig part */
                *fi = (i << MANT_SHIFTS) | (EXP_BIAS << EXP_SHIFTS);
                f = sqrt(f);
                sqrt_tab[i] = *fi & MANT_MASK;

                f = 0; /* Clears least sig part */
                *fi = (i << MANT_SHIFTS) | ((EXP_BIAS + 1) << EXP_SHIFTS);
                f = sqrt(f);
                sqrt_tab[i + SQRT_TAB_SIZE/2] = *fi & MANT_MASK;
        }
}

double
fsqrt(double f)
{
        unsigned int e;
        unsigned int   *fi = (unsigned int *) &f + MOST_SIG_OFFSET;

        if (f == 0.0) return(0.0);
        e = (*fi >> EXP_SHIFTS) - EXP_BIAS;
        *fi &= MANT_MASK;
        if (e & 1)
                *fi |= EXP_LSB;
        e >>= 1;
        *fi = (sqrt_tab[*fi >> MANT_SHIFTS]) |
              ((e + EXP_BIAS) << EXP_SHIFTS);
        return(f);
}

void
dump_sqrt_tab()
{
        int        i, nl = 0;

        printf("unsigned int sqrt_tab[] = {\n");
        for (i = 0; i < SQRT_TAB_SIZE-1; i++)
        {
                printf("0x%x,", sqrt_tab[i]);
                nl++;
                if (nl > 8) { nl = 0; putchar('\n'); }
        }
        printf("0x%x\n", sqrt_tab[SQRT_TAB_SIZE-1]);
        printf("};\n");
}
#endif


#endif



