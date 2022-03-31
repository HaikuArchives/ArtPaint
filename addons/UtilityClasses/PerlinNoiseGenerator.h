/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef PERLIN_NOISE_GENERATOR_H
#define PERLIN_NOISE_GENERATOR_H

#include <math.h>
#define PI M_PI

class PerlinNoiseGenerator {
		float		persistence;
		int32		number_of_frequencies;
		float		frequency_relation;

		float		normalization_coefficient;

		float		random_table[1024];

inline	float		LinearInterpolation(float a,float b,float x) { return a + x*(b-a); }
inline	float		CosineInterpolation(float a,float b,float x);


inline	float		Noise(int32 x, int32 y);
inline	float		InterpolatedNoise2D(float x, float y);
inline	float		InterpolatedNoise3D(float x, float y, float z);

public:
	PerlinNoiseGenerator(float p,int32 n,float relation=2.0) {
		persistence = p;
		number_of_frequencies = n;

		for (int32 i=0;i<1024;i++) {
			random_table[i] = Noise(i,i*7+19);
		}

		frequency_relation = relation;

		normalization_coefficient = 1.0 / n;
	}


inline	float	PerlinNoise2D(float x,float y,float = 1);
inline	float	PerlinNoise3D(float x,float y,float z,float=1);
};


float PerlinNoiseGenerator::PerlinNoise2D(float x,float y,float frequency)
{
	float total = 0;
	float p = persistence;
	int32 n = number_of_frequencies-1;

	float amplitude = 1.0 / p;
	for (int32 i=0;i<=n;++i) {
		frequency *= frequency_relation;
		amplitude *= p;

		total += InterpolatedNoise2D(x*frequency,y*frequency)*amplitude;
	}

	total = min_c(1.0,max_c(-1.0,total));

	return total;
}

float PerlinNoiseGenerator::PerlinNoise3D(float x,float y,float z,float frequency)
{
	float total = 0;
	float p = persistence;
	int32 n = number_of_frequencies-1;

	float amplitude = 1.0 / p;
	for (int32 i=0;i<=n;++i) {
		frequency *= 2.0;
		amplitude *= p;

		total += InterpolatedNoise3D(x*frequency,y*frequency,z*frequency)*amplitude;
	}

	total = min_c(1.0,max_c(-1.0,total));

	return total;
}

float PerlinNoiseGenerator::CosineInterpolation(float a, float b, float x)
{
	float ft = x * PI;
	float f = (1 - cos(ft)) * 0.5;

	return a*(1-f) + b*f;
}


inline float PerlinNoiseGenerator::Noise(int32 x, int32 y)
{
	int32 n = x + y*57;
	n = n<<13 ^ n;
	return(1.0-((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.0);
}

inline float PerlinNoiseGenerator::InterpolatedNoise2D(float x, float y)
{
	int32 integer_x = (int32)x;
	int32 integer_y = (int32)y;

	float fractional_x = x - integer_x;
	float fractional_y = y - integer_y;

//	float v1 = Noise(integer_x,integer_y);
//	float v2 = Noise(integer_x+1,integer_y);
//	float v3 = Noise(integer_x,integer_y+1);
//	float v4 = Noise(integer_x+1,integer_y+1);
	float v1 = random_table[(integer_x + 57*integer_y)%1024];
	float v2 = random_table[(integer_x+1 + 57*integer_y)%1024];
	float v3 = random_table[(integer_x + 57*(integer_y+1))%1024];
	float v4 = random_table[(integer_x+1 + 57*(integer_y+1))%1024];

	float i1 = LinearInterpolation(v1,v2,fractional_x);
	float i2 = LinearInterpolation(v3,v4,fractional_x);

	return LinearInterpolation(i1,i2,fractional_y);
}


inline float PerlinNoiseGenerator::InterpolatedNoise3D(float x, float y, float z)
{
	int32 integer_x = (int32)x;
	int32 integer_y = (int32)y;
	int32 integer_z = (int32)z;

	float fractional_x = x - integer_x;
	float fractional_y = y - integer_y;
	float fractional_z = z - integer_z;

	float v1 = random_table[(integer_x + 57*integer_y + integer_z*61)%1024];
	float v2 = random_table[(integer_x+1 + 57*integer_y + integer_z*61)%1024];
	float v3 = random_table[(integer_x + 57*(integer_y+1) + integer_z*61)%1024];
	float v4 = random_table[(integer_x+1 + 57*(integer_y+1) + integer_z*61)%1024];

	float v5 = random_table[(integer_x + 57*integer_y + (integer_z+1)*61)%1024];
	float v6 = random_table[(integer_x+1 + 57*integer_y + (integer_z+1)*61)%1024];
	float v7 = random_table[(integer_x + 57*(integer_y+1) + (integer_z+1)*61)%1024];
	float v8 = random_table[(integer_x+1 + 57*(integer_y+1) + (integer_z+1)*61)%1024];

	float i1 = LinearInterpolation(v1,v2,fractional_x);
	float i2 = LinearInterpolation(v3,v4,fractional_x);
	float p1 = LinearInterpolation(i1,i2,fractional_y);

	float i3 = LinearInterpolation(v5,v6,fractional_x);
	float i4 = LinearInterpolation(v7,v8,fractional_x);
	float p2 = LinearInterpolation(i3,i4,fractional_y);

	return LinearInterpolation(p1,p2,fractional_z);
}

#endif
