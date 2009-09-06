/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef WAVE_H
#define WAVE_H

#include <stdio.h>

#include "WindowGUIManipulator.h"
#include "Controls.h"

#define	WAVE_LENGTH_ADJUSTING_STARTED	'Wlas'
#define	WAVE_AMOUNT_ADJUSTING_STARTED	'Waas'

#define	WAVE_AMOUNT_CHANGED				'Wamc'
#define	WAVE_LENGTH_CHANGED				'Wlec'
#define	WAVE_DAMPENING_CHANGED			'Wdch'

#define	MAX_WAVE_AMOUNT		100
#define MIN_WAVE_AMOUNT		3
#define MAX_WAVE_LENGTH		200
#define MIN_WAVE_LENGTH		3
#define MAX_WAVE_DAMPENING	100

class WaveManipulatorSettings : public ManipulatorSettings {
public:
		WaveManipulatorSettings()
			: ManipulatorSettings() {
			center = BPoint(0,0);
			wave_length = 30;
			wave_amount = 30;
		}

		WaveManipulatorSettings(const WaveManipulatorSettings& s)
			: ManipulatorSettings() {
			center = s.center;
			wave_length = s.wave_length;
			wave_amount = s.wave_amount;
		}


		WaveManipulatorSettings& operator=(const WaveManipulatorSettings& s) {
			center = s.center;
			wave_length = s.wave_length;
			wave_amount = s.wave_amount;
			return *this;
		}


		bool operator==(WaveManipulatorSettings s) {
			return ((center == s.center) && (wave_length == s.wave_length)
					&& (wave_amount == s.wave_amount));
		}

		BPoint	center;
		float	wave_length;
		float	wave_amount;
};


class WaveManipulatorView;

class WaveManipulator : public WindowGUIManipulator {
BBitmap				*preview_bitmap;
BBitmap				*copy_of_the_preview_bitmap;


WaveManipulatorSettings	settings;
WaveManipulatorSettings	previous_settings;

WaveManipulatorView		*config_view;

float					*sin_table;

int32					last_calculated_resolution;
int32					lowest_available_quality;
int32					highest_available_quality;



public:
			WaveManipulator(BBitmap*);
			~WaveManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString() { return "Click on the image to set the wave center. Use the sliders to adjust wave."; }
char*		ReturnName() { return "Wave"; }

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger& target);

void		ChangeSettings(ManipulatorSettings*);
};



class WaveManipulatorView : public WindowGUIManipulatorView {
		BMessenger				*target;
		WaveManipulator			*manipulator;
		ControlSlider			*wave_length_slider;
		ControlSlider			*wave_amount_slider;
		WaveManipulatorSettings	settings;

		bool					preview_started;

public:
		WaveManipulatorView(BRect,WaveManipulator*, const BMessenger& target);
		~WaveManipulatorView();

void	AttachedToWindow();
void	AllAttached();
void	MessageReceived(BMessage*);
void	ChangeSettings(WaveManipulatorSettings *s);
};


#ifdef __POWERPC__
inline 	asm	float reciprocal_of_square_root(register float number)
{
	machine		604
	frsqrte		fp1,number;	// Estimates reciprocal of square-root
	blr
}
#elif __INTEL__
float reciprocal_of_square_root(register float number)
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



