/* 

	Filename:	SplineGenerator.h
	Contents:	A class that can be used to genarate various splines.	
	Author:		Heikki Suhonen
	
*/



#ifndef SPLINE_GENERATOR_H
#define	SPLINE_GENERATOR_H


class SplineGenerator {
static	inline	float	car0(float);
static	inline	float	car1(float);
static	inline	float	car2(float);
static	inline	float	car3(float);


public:

static	inline	float	CardinalSpline(float v0,float v1,float v2,float v3,float u);
};

float SplineGenerator::car0(float u)
{
	float s = 1.0;	
	return 2*s*u*u - s*u*u*u - s*u;
}


float SplineGenerator::car1(float u)
{
	float s = 1.0;	
	return (2-s)*u*u*u + (s-3)*u*u + 1;
}


float SplineGenerator::car2(float u)
{
	float s = 1.0;	
	return (s-2)*u*u*u + (3-2*s)*u*u + s*u;
}


float SplineGenerator::car3(float u)
{
	float s = 1.0;	
	return s*u*u*u - s*u*u;
}


float SplineGenerator::CardinalSpline(float v0,float v1,float v2,float v3,float u)
{
	return v0*car0(u) + v1*car1(u) + v2*car2(u) + v3*car3(u);
}

#endif