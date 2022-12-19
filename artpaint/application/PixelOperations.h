/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */
#ifndef PIXEL_OPERATIONS_H
#define PIXEL_OPERATIONS_H


#include <Catalog.h>


#include <math.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PixelOperations"


union color_conversion {
	uint8 bytes[4];
	uint32 word;
};


enum BlendModes {
	BLEND_NORMAL = 0,
	BLEND_MULTIPLY,
	BLEND_DIVIDE,
	BLEND_SCREEN,
	BLEND_OVERLAY,
	BLEND_DARKEN,
	BLEND_LIGHTEN,
	BLEND_DODGE,
	BLEND_BURN,
	BLEND_LINEAR_DODGE,
	BLEND_LINEAR_BURN,
	BLEND_HARD_LIGHT,
	BLEND_SOFT_LIGHT,
	BLEND_VIVID_LIGHT,
	BLEND_LINEAR_LIGHT,
	BLEND_PIN_LIGHT,
	BLEND_HARD_MIX,
	BLEND_DIFFERENCE,
	BLEND_EXCLUSION,
	BLEND_DISSOLVE
};


inline const char* mode_to_string(BlendModes mode)
{
	switch(mode) {
		case BLEND_NORMAL:
			return B_TRANSLATE("Normal");
		case BLEND_MULTIPLY:
			return B_TRANSLATE("Multiply");
		case BLEND_DIVIDE:
			return B_TRANSLATE("Divide");
		case BLEND_SCREEN:
			return B_TRANSLATE("Screen");
		case BLEND_OVERLAY:
			return B_TRANSLATE("Overlay");
		case BLEND_DARKEN:
			return B_TRANSLATE("Darken");
		case BLEND_LIGHTEN:
			return B_TRANSLATE("Lighten");
		case BLEND_DODGE:
			return B_TRANSLATE("Color dodge");
		case BLEND_BURN:
			return B_TRANSLATE("Color burn");
		case BLEND_LINEAR_DODGE:
			return B_TRANSLATE("Linear dodge");
		case BLEND_LINEAR_BURN:
			return B_TRANSLATE("Linear burn");
		case BLEND_HARD_LIGHT:
			return B_TRANSLATE("Hard light");
		case BLEND_SOFT_LIGHT:
			return B_TRANSLATE("Soft light");
		case BLEND_VIVID_LIGHT:
			return B_TRANSLATE("Vivid light");
		case BLEND_LINEAR_LIGHT:
			return B_TRANSLATE("Linear light");
		case BLEND_PIN_LIGHT:
			return B_TRANSLATE("Pin light");
		case BLEND_HARD_MIX:
			return B_TRANSLATE("Hard mix");
		case BLEND_DIFFERENCE:
			return B_TRANSLATE("Difference");
		case BLEND_EXCLUSION:
			return B_TRANSLATE("Exclusion");
		case BLEND_DISSOLVE:
			return B_TRANSLATE("Dissolve");
	}

	return "";
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
	float inv_c = 1.0-c;

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

	uint32 result = ((((((p1 >> 24) &0xFF) * c + ((p2 >> 24)&0xFF) * inv_c)>>15)<<24) & 0xFF000000) |
			((((((p1 >> 16)&0xFF) * c + ((p2 >> 16)&0xFF) * inv_c)>>15)<<16) & 0x00FF0000) |
			((((((p1 >> 8)&0xFF) * c + ((p2 >> 8)&0xFF) * inv_c)>>15)<<8) & 0x0000FF00) |
			(((((p1&0xFF) * c + (p2&0xFF) * inv_c)>>15)) & 0x000000FF);

	return result;
}


inline uint32 src_over_fixed(uint32 dst, uint32 src)
{
	union color_conversion src_rgba, dst_rgba, result_rgba;

	src_rgba.word = src;
	dst_rgba.word = dst;

	uint8 src_alpha = src_rgba.bytes[3] ;
	uint8 dst_alpha = dst_rgba.bytes[3];

	uint32 inv_src_alpha = 255 - src_alpha;
	uint32 inv_dst_alpha = (dst_alpha * inv_src_alpha) / 255;
	uint32 result_alpha = src_alpha + inv_dst_alpha;
	if (result_alpha == 0)
		return 0;

	// r-rgb * r-a = s-rgb * s-a + d-rgb * d-a * (1 - s-a)

	result_rgba.bytes[0] = (src_rgba.bytes[0] * src_alpha +
		dst_rgba.bytes[0] * inv_dst_alpha) / result_alpha;
	result_rgba.bytes[1] = (src_rgba.bytes[1] * src_alpha +
		dst_rgba.bytes[1] * inv_dst_alpha) / result_alpha;
	result_rgba.bytes[2] = (src_rgba.bytes[2] * src_alpha +
		dst_rgba.bytes[2] * inv_dst_alpha) / result_alpha;
	result_rgba.bytes[3] = result_alpha;

	return result_rgba.word;
}


inline uint8 add(uint8 dst, uint8 src)
{
	return min_c(255, dst + src);
}


inline uint8 subtract(uint8 dst, uint8 src)
{
	return max_c(0, dst - src);
}


inline uint8 multiply(uint8 dst, uint8 src)
{
	return (dst * src) / 255;
}


inline uint8 divide(uint8 dst, uint8 src)
{
	if (src == 0)
		return 255;

	if (src == 255)
		return dst;

	return max_c(0, min_c(255, dst * 255 / src));
}


inline uint8 screen(uint8 dst, uint8 src)
{
	return dst + src - multiply(dst, src);
}


inline uint8 hard_light(uint8 dst, uint8 src)
{
	if (src <= 127)
		return multiply(dst, 2 * src);

	return screen(dst, 2 * src - 255);
}


inline uint8 darken(uint8 dst, uint8 src)
{
	return min_c(src, dst);
}


inline uint8 lighten(uint8 dst, uint8 src)
{
	return max_c(src, dst);
}


inline uint8 color_dodge(uint8 dst, uint8 src)
{
	if (dst == 0)
		return 0;

	if (src == 255)
		return 255;

	return min_c(255, divide(dst, 255 - src));
}


inline uint8 color_burn(uint8 dst, uint8 src)
{
	if (dst == 255)
		return 255;

	if (src == 0)
		return 0;

	return 255 - min_c(255, divide(255 - dst, src));
}


inline uint8 soft_light(uint8 dst, uint8 src)
{
	if (src <= 127)
		return dst - multiply(255 - (2 * src), dst - multiply(dst, dst));

	return dst + multiply((2 * src) - 255,
		(sqrt((float)dst / 255) * 255) - dst);
}


inline uint8 vivid_light(uint8 dst, uint8 src)
{
	if (src > 127)
		return color_dodge(dst, 2*src);

	return color_burn(dst, 2*src);
}


inline uint8 linear_light(uint8 dst, uint8 src)
{
	return max_c(0, min_c(255, dst + (2 * src) - 255));
}


inline uint8 pin_light(uint8 dst, uint8 src)
{
	uint16 cmp = 2 * src;
	if (dst < cmp - 255)
		return min_c(255, cmp - 255);
	if (dst > cmp)
		return min_c(255, cmp);

	return dst;
}


inline uint8 difference(uint8 dst, uint8 src)
{
	return abs(dst - src);
}


inline uint8 exclusion(uint8 dst, uint8 src)
{
	return dst + src - (2 * dst * src / 255);
}


inline uint8 linear_dodge(uint8 dst, uint8 src)
{
	return min_c(255, add(dst, src));
}


inline uint8 linear_burn(uint8 dst, uint8 src)
{
	return max_c(0, src + dst - 255);
}


inline uint8 hard_mix(uint8 dst, uint8 src)
{
	if (src < 255 - dst)
		return 0;

	return 255;
}


inline uint32 dissolve(uint32 dst, uint32 src)
{
	uint8 fac = (uint8)(rand() * 255);

	if (fac > 96)
		return src;

	return dst;
}


inline uint32 blend(uint32 dst, uint32 src, uint32 mode)
{
	union color_conversion blend_color, src_color, dst_color;

	src_color.word = src;
	dst_color.word = dst;
	blend_color.word = src;

	switch(mode) {
		case(BLEND_MULTIPLY): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					multiply(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_SCREEN): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					screen(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_OVERLAY): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					hard_light(src_color.bytes[i], dst_color.bytes[i]);
			}
		} break;
		case (BLEND_HARD_LIGHT): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					hard_light(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_DARKEN): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					darken(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_LIGHTEN): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					lighten(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_DODGE): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					color_dodge(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_BURN): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					color_burn(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_SOFT_LIGHT): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					soft_light(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_DIFFERENCE): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					difference(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_EXCLUSION): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					exclusion(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_VIVID_LIGHT): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					vivid_light(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_LINEAR_LIGHT): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					linear_light(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_PIN_LIGHT): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					pin_light(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case(BLEND_DIVIDE): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					divide(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_LINEAR_DODGE): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					linear_dodge(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_LINEAR_BURN): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					linear_burn(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_HARD_MIX): {
			for (int i = 0; i < 3; ++i) {
				blend_color.bytes[i] =
					hard_mix(dst_color.bytes[i], src_color.bytes[i]);
			}
		} break;
		case (BLEND_DISSOLVE): {
			 blend_color.word = dissolve(dst_color.word, src_color.word);
		} break;
	}

	return blend_color.word;
}


inline uint32 src_over_fixed_blend(uint32 dst, uint32 src, uint32 mode=0)
{
	union color_conversion src_rgba, dst_rgba, blend_color, result_rgba;

	src_rgba.word = src;
	dst_rgba.word = dst;
	blend_color.word = blend(dst_rgba.word, src_rgba.word, mode);

	uint8 src_alpha = src_rgba.bytes[3];
	uint8 dst_alpha = dst_rgba.bytes[3];

	uint8 inv_src_alpha = 255 - src_alpha;
	uint8 inv_dst_alpha = 255 - dst_alpha;
	uint8 inv_src_dst_alpha = (dst_alpha * inv_src_alpha) / 255;
	uint8 inv_dst_src_alpha = (src_alpha * inv_dst_alpha) / 255;
	uint8 src_dst_alpha = (src_alpha * dst_alpha) / 255;
	uint8 result_alpha = src_alpha + inv_src_dst_alpha;

	if (result_alpha == 0)
		return 0;

	// r-rgb * r-a = s-a * (1 - d-a) * s-rgb +
	//		s-a * d-a * blend(d-rgb, s-rgb) +
	// 		d-rgb * d-a * (1 - s-a)

	result_rgba.bytes[0] = (src_rgba.bytes[0] * inv_dst_src_alpha +
		src_dst_alpha * blend_color.bytes[0] +
		dst_rgba.bytes[0] * inv_src_dst_alpha) / result_alpha;
	result_rgba.bytes[1] = (src_rgba.bytes[1] * inv_dst_src_alpha +
		src_dst_alpha * blend_color.bytes[1] +
		dst_rgba.bytes[1] * inv_src_dst_alpha) / result_alpha;
	result_rgba.bytes[2] = (src_rgba.bytes[2] * inv_dst_src_alpha +
		src_dst_alpha * blend_color.bytes[2] +
		dst_rgba.bytes[2] * inv_src_dst_alpha) / result_alpha;

	result_rgba.bytes[3] = result_alpha;

	return result_rgba.word;
}


inline uint32 dst_over_fixed(uint32 dst, uint32 src)
{
	union color_conversion src_rgba, dst_rgba, result_rgba;

	src_rgba.word = src;
	dst_rgba.word = dst;

	uint8 src_alpha = src_rgba.bytes[3];
	uint8 dst_alpha = dst_rgba.bytes[3];

	uint32 inv_dst_alpha = 255 - dst_alpha;
	uint32 inv_src_alpha = (src_alpha * inv_dst_alpha) / 255;
	uint32 result_alpha = inv_src_alpha + dst_alpha;
	if (result_alpha == 0)
		result_alpha = 1;

	// r-rgb * r-a = s-rgb * s-a * (1 - d-a) + d-rgb * d-a

	result_rgba.bytes[0] = (src_rgba.bytes[0] * inv_src_alpha +
		dst_rgba.bytes[0] * dst_alpha) / result_alpha;
	result_rgba.bytes[1] = (src_rgba.bytes[1] * inv_src_alpha +
		dst_rgba.bytes[1] * dst_alpha) / result_alpha;
	result_rgba.bytes[2] = (src_rgba.bytes[2] * inv_src_alpha +
		dst_rgba.bytes[2] * dst_alpha) / result_alpha;
	result_rgba.bytes[3] = result_alpha;

	return result_rgba.word;
}


inline uint32 src_out_fixed(uint32 dst, uint32 src)
{
	union color_conversion src_rgba, dst_rgba, result_rgba;

	src_rgba.word = src;
	dst_rgba.word = dst;

	uint8 src_alpha = src_rgba.bytes[3];
	uint8 dst_alpha = dst_rgba.bytes[3];

	uint32 inv_dst_alpha = 255 - dst_alpha;
	uint32 result_alpha = (src_alpha * inv_dst_alpha) / 255;
	if (result_alpha == 0)
		return 0;

	// r-rgb * r-a = s-rgb * s-a * (1 - d-a)

	result_rgba.bytes[0] = (src_rgba.bytes[0] * src_alpha * inv_dst_alpha) /
		result_alpha;
	result_rgba.bytes[1] = (src_rgba.bytes[1] * src_alpha * inv_dst_alpha) /
		result_alpha;
	result_rgba.bytes[2] = (src_rgba.bytes[2] * src_alpha * inv_dst_alpha) /
		result_alpha;
	result_rgba.bytes[3] = result_alpha;

	return result_rgba.word;
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


inline uint32 lerp(uint32 p1, uint32 p2, float t)
{
	union color_conversion one, two, result;

	one.word = p1;
	two.word = p2;

	for (int i = 0; i < 4; ++i)
		result.bytes[i] = (one.bytes[i] * (1.0 - t)) + (two.bytes[i] * t);

	return result.word;
}

#endif
