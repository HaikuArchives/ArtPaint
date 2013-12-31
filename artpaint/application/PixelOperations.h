/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef PIXEL_OPERATIONS_H
#define PIXEL_OPERATIONS_H


#include <math.h>


union color_conversion_union {
	char bytes[4];
	uint32 word;
};

// This is just an utility-function for rounding the values.
// It can be made faster by using inline assebler code. Especially
// instructions frsp and fctlw could be useful on the PPC.
inline float round(float c)
{
	return (((c - floor(c)) > 0.5) ? ceil(c) : floor(c));
}


// This function combines four pixels p1-p4 with weights c1-c4.
// It accumulates the rounding error for each component and
// then adds it to the final values. It does not check if the values
// exceed 255 or 0. The coefficients c1-c4 should add up to 1.0.
// This seems to work perfectly, but it is a little bit slow
inline uint32 combine_4_pixels(uint32 p1,uint32 p2,uint32 p3,uint32 p4,float c1,float c2,float c3,float c4)
{
//	float red_error = 0;
//	float green_error = 0;
//	float blue_error = 0;
//	float alpha_error = 0;
//	uint32 new_value = 0x00000000;
//
//	float red,green,blue,alpha;
//	// First handle the red value.
//	red = ((p1>>8) & 0xFF)*c1;
//	red_error += red - (uint32)red;
//	new_value += (uint32)red<<8;
//
//	red = ((p2>>8) & 0xFF)*c2;
//	red_error += red - (uint32)red;
//	new_value += (uint32)red<<8;
//
//	red = ((p3>>8) & 0xFF)*c3;
//	red_error += red - (uint32)red;
//	new_value += (uint32)red<<8;
//
//	red = ((p4>>8) & 0xFF)*c4;
//	red_error += red - (uint32)red;
//	new_value += (uint32)red<<8;
//	// Finally add the red_error
//	new_value += ((uint32)round(red_error))<<8;
//
//	// Handle the green
//	green = ((p1>>16) & 0xFF)*c1;
//	green_error += green - (uint32)green;
//	new_value += (uint32)green<<16;
//
//	green = ((p2>>16) & 0xFF)*c2;
//	green_error += green - (uint32)green;
//	new_value += (uint32)green<<16;
//
//	green = ((p3>>16) & 0xFF)*c3;
//	green_error += green - (uint32)green;
//	new_value += (uint32)green<<16;
//
//	green = ((p4>>16) & 0xFF)*c4;
//	green_error += green - (uint32)green;
//	new_value += (uint32)green<<16;
//	// Finally add the green_error
//	new_value += ((uint32)round(green_error))<<16;
//
//	// Handle the blue
//	blue = ((p1>>24) & 0xFF)*c1;
//	blue_error += blue - (uint32)blue;
//	new_value += (uint32)blue<<24;
//
//	blue = ((p2>>24) & 0xFF)*c2;
//	blue_error += blue - (uint32)blue;
//	new_value += (uint32)blue<<24;
//
//	blue = ((p3>>24) & 0xFF)*c3;
//	blue_error += blue - (uint32)blue;
//	new_value += (uint32)blue<<24;
//
//	blue = ((p4>>24) & 0xFF)*c4;
//	blue_error += blue - (uint32)blue;
//	new_value += (uint32)blue<<24;
//	// Finally add the blue_error
//	new_value += ((uint32)round(blue_error))<<24;
//
//	// Handle the alpha
//	alpha = ((p1) & 0xFF)*c1;
//	alpha_error += alpha - (uint32)alpha;
//	new_value += (uint32)alpha;
//
//	alpha = ((p2) & 0xFF)*c2;
//	alpha_error += alpha - (uint32)alpha;
//	new_value += (uint32)alpha;
//
//	alpha = ((p3) & 0xFF)*c3;
//	alpha_error += alpha - (uint32)alpha;
//	new_value += (uint32)alpha;
//
//	alpha = ((p4) & 0xFF)*c4;
//	alpha_error += alpha - (uint32)alpha;
//	new_value += (uint32)alpha;
//	// Finally add the blue_error
//	new_value += ((uint32)round(alpha_error));

	uint32 new_value;
	float red,green,blue,alpha;
	red = (((p1>>8) & 0xFF)*c1) + (((p2>>8) & 0xFF)*c2) + (((p3>>8) & 0xFF)*c3) + (((p4>>8) & 0xFF)*c4);
	green = (((p1>>16) & 0xFF)*c1) + (((p2>>16) & 0xFF)*c2) + (((p3>>16) & 0xFF)*c3) + (((p4>>16) & 0xFF)*c4);
	blue = (((p1>>24) & 0xFF)*c1) + (((p2>>24) & 0xFF)*c2) + (((p3>>24)&0xFF)*c3) + (((p4>>24)&0xFF)*c4);
	alpha = ((p1&0xFF)*c1) + ((p2&0xFF)*c2) + ((p3&0xFF)*c3) + ((p4&0xFF)*c4);

	blue = round(blue);
	green = round(green);
	red = round(red);
	alpha = round(alpha);


	new_value =	(((uint32)blue<<24)&0xFF000000) |
				(((uint32)green<<16)&0x00FF0000) |
				(((uint32)red<<8)&0x0000FF00) |
				(((uint32)alpha)&0x000000FF);

	return new_value;
}

// This function combines four pixels p1-p4 with weights c1-c4.
// This does not accumulate any rounding errors, but instead it uses
// fixed-point arithmetic.
inline uint32 combine_4_pixels_fixed(uint32 p1,uint32 p2,uint32 p3,uint32 p4,uint32 c1,uint32 c2,uint32 c3,uint32 c4)
{
	uint32 new_value = 0x00000000;
	uint32 red,green,blue,alpha;
	// First handle the 'red' value.
	red = (((p1 >> 8) & 0xFF) * c1) + (((p2 >> 8) & 0xFF) * c2) + (((p3 >> 8) & 0xFF) * c3) + (((p4 >> 8) & 0xFF) * c4);
	new_value |= ((red>>15)<<8) & 0x0000FF00;

	green = (((p1 >> 16) & 0xFF) * c1) + (((p2 >> 16) & 0xFF) * c2) + (((p3 >> 16) & 0xFF) * c3) + (((p4 >> 16) & 0xFF) * c4);
	new_value |= ((green>>15)<<16) & 0x00FF0000;

	blue = (((p1 >> 24) & 0xFF) * c1) + (((p2 >> 24) & 0xFF) * c2) + (((p3 >> 24) & 0xFF) * c3) + (((p4 >> 24) & 0xFF) * c4);
	new_value |= ((blue>>15)<<24) & 0xFF000000;

	alpha = (((p1) & 0xFF) * c1) + (((p2) & 0xFF) * c2) + (((p3) & 0xFF) * c3) + (((p4) & 0xFF) * c4);
	new_value |= ((alpha>>15)) & 0x000000FF;

	return new_value;
}



// Parameters u and v should be in range [0,1]
inline uint32 bilinear_interpolation(uint32 p1,uint32 p2,uint32 p3,uint32 p4,float u,float v)
{
	union {
		uint8 b[4];
		uint32 word;
	} f,c1,c2,c3,c4;

	c1.word = p1;
	c2.word = p2;
	c3.word = p3;
	c4.word = p4;
	float one_minus_v = 1.0-v;
	float one_minus_u = 1.0-u;

	f.b[0] = (uint8)(one_minus_v * ( one_minus_u * c1.b[0] +  u * c2.b[0]) + v * ( one_minus_u*c3.b[0] + u * c4.b[0]));
	f.b[1] = (uint8)(one_minus_v * ( one_minus_u * c1.b[1] +  u * c2.b[1]) + v * ( one_minus_u*c3.b[1] + u * c4.b[1]));
	f.b[2] = (uint8)(one_minus_v * ( one_minus_u * c1.b[2] +  u * c2.b[2]) + v * ( one_minus_u*c3.b[2] + u * c4.b[2]));
	f.b[3] = (uint8)(one_minus_v * ( one_minus_u * c1.b[3] +  u * c2.b[3]) + v * ( one_minus_u*c3.b[3] + u * c4.b[3]));

	return f.word;
}


// This mixes two pixels according to the coefficient c.
// The coefficient c should be between 0 and 1. This does not
// take into account any rounding errors, thus 255*0.5 + 255*0.5
// will add up to 127 + 127 = 254. This is too bad for things that
// require accurate calculation.
inline uint32 mix_2_pixels(uint32 p1,uint32 p2,float c)
{
	register float inv_c = 1.0-c;

	return 	(((uint32)(((p1 >> 24) & 0xFF) * c)<<24) + ((uint32)(((p2 >> 24) & 0xFF) * inv_c)<<24)) |
			(((uint32)(((p1 >> 16) & 0xFF) * c)<<16) + ((uint32)(((p2 >> 16) & 0xFF) * inv_c)<<16)) |
			(((uint32)(((p1 >> 8) & 0xFF) * c)<<8) + ((uint32)(((p2 >> 8) & 0xFF) * inv_c)<<8)) |
			(((uint32)(((p1) & 0xFF) * c)) + ((uint32)(((p2) & 0xFF) * inv_c)));
}



#if __POWERPC__
//// This inline assemler function mixes the two pixels just like mix_2_pixels
inline asm uint32 mix_2_pixels_asm(uint32 p1, uint32 p2, float c)
{
	// first take the right shifted versions of the components
	li		r6,8
	srw		r10,r4,r6
	srw		r11,r5,r6
	li		r6,16
	srw		r12,r4,r6
	srw		r13,r5,r6
	li		r6,24
	srw		r14,r4,r6
	srw		r15,r5,r6
	andi.	r10,r10,0xFF
	andi.	r11,r11,0xFF
	andi.	r12,r12,0xFF
	andi.	r13,r13,0xFF
	andi.	r14,r14,0xFF
	andi.	r15,r15,0xFF
	andi.	r4,r4,0xFF
	andi.	r6,r5,0xFF

	// Then we must store the values to memory in order to copy them to
	// float registers.


	blr
}
#endif

inline uint32 mix_2_pixels_fixed(uint32 p1,uint32 p2,uint32 c)
{
	uint32 inv_c = 32768 - c;

	return 	((((((p1 >> 24) &0xFF) * c + ((p2 >> 24)&0xFF) * inv_c)>>15)<<24) & 0xFF000000) |
			((((((p1 >> 16)&0xFF) * c + ((p2 >> 16)&0xFF) * inv_c)>>15)<<16) & 0x00FF0000) |
			((((((p1 >> 8)&0xFF) * c + ((p2 >> 8)&0xFF) * inv_c)>>15)<<8) & 0x0000FF00) |
			(((((p1&0xFF) * c + (p2&0xFF) * inv_c)>>15)) & 0x000000FF);
}


// Returns true if the two pixels p1 and p2 are the same within the limits of var.
// If var is 0, the pixels must be exactly the same color.
inline bool compare_2_pixels_with_variance(uint32 p1, uint32 p2, uint32 var)
{
	return 	(((p1>>24)&0xFF) <= (((p2>>24)&0xFF)+var) && (((p1>>24)&0xFF)+var) >= ((p2>>24)&0xFF)) &&
			(((p1>>16)&0xFF) <= (((p2>>16)&0xFF)+var) && (((p1>>16)&0xFF)+var) >= ((p2>>16)&0xFF)) &&
			(((p1>>8)&0xFF) <= (((p2>>8)&0xFF)+var) && (((p1>>8)&0xFF)+var) >= ((p2>>8)&0xFF)) &&
			((p1&0xFF) <= ((p2&0xFF)+var) && ((p1&0xFF)+var) >= (p2&0xFF));
}

#endif
