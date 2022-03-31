/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef TWIRL_H
#define TWIRL_H

#include <stdio.h>

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"

class BSlider;

#define	TWIRL_AMOUNT_ADJUSTING_STARTED	'Taas'
#define	TWIRL_RADIUS_ADJUSTING_STARTED	'Tras'
#define	TWIRL_AMOUNT_CHANGED			'Tamc'
#define	TWIRL_RADIUS_CHANGED			'Tlec'

#define	MAX_TWIRL_AMOUNT			720

#define	MIN_TWIRL_AMOUNT			-720

class TwirlManipulatorSettings : public ManipulatorSettings {

public:
		TwirlManipulatorSettings()
			: ManipulatorSettings() {
			center = BPoint(0,0);
			twirl_amount = 10;
			twirl_radius = 100;
		}


		TwirlManipulatorSettings(const TwirlManipulatorSettings& s)
			: ManipulatorSettings() {
			center = s.center;
			twirl_amount = s.twirl_amount;
			twirl_radius = s.twirl_radius;
		}


		TwirlManipulatorSettings& operator=(const TwirlManipulatorSettings& s) {
			center = s.center;
			twirl_amount = s.twirl_amount;
			twirl_radius = s.twirl_radius;
			return *this;
		}

		bool operator==(TwirlManipulatorSettings s) {
			return ((center == s.center) && (twirl_amount == s.twirl_amount) &&
					(twirl_radius == s.twirl_radius));
		}


		BPoint 	center;
		float	twirl_amount;
		float	twirl_radius;
};


class TwirlManipulatorView;


class TwirlManipulator : public WindowGUIManipulator {
BBitmap						*preview_bitmap;
BBitmap						*copy_of_the_preview_bitmap;

TwirlManipulatorSettings	settings;
TwirlManipulatorSettings	previous_settings;

TwirlManipulatorView		*config_view;

float						*sin_table;
float						*cos_table;

int32						last_calculated_resolution;
int32						lowest_available_quality;
int32						highest_available_quality;

public:
			TwirlManipulator(BBitmap*);
			~TwirlManipulator();

void		MouseDown(BPoint,uint32 buttons,BView*,bool);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);
char*		ReturnHelpString() { return "Click on the image to set the twirl center. Use sliders to adjust twirl."; }
char*		ReturnName() { return "Twirl"; }

ManipulatorSettings*	ReturnSettings();

BView*		MakeConfigurationView(const BMessenger& target);
void		ChangeSettings(ManipulatorSettings *s);

};

class TwirlManipulatorView : public WindowGUIManipulatorView {
		BMessenger					*target;
		TwirlManipulator			*manipulator;
		BSlider						*twirl_radius_slider;
		BSlider						*twirl_amount_slider;
		TwirlManipulatorSettings	settings;

		bool						preview_started;

public:
		TwirlManipulatorView(BRect,TwirlManipulator*, const BMessenger& target);
		~TwirlManipulatorView();

void	AttachedToWindow();
void	AllAttached();
void	MessageReceived(BMessage*);
void	ChangeSettings(TwirlManipulatorSettings *s);
};


#ifdef __POWERPC__
inline 	asm	float reciprocal_of_square_root(float number)
{
	machine		603
	frsqrte		fp1,number;	// Estimates reciprocal of square-root
	blr
}
#else
// This is very slow.
inline float reciprocal_of_square_root(float number)
{
	return 1.0/sqrt(number);
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



