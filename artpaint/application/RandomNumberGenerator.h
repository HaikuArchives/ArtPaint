/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	RANDOM_NUMBER_GENERATOR_H
#define	RANDOM_NUMBER_GENERATOR_H

#include <SupportDefs.h>


class RandomNumberGenerator {
			// These can be used if the required sequence length is not too long.
			int32*	integer_random_number_array;	// values between -???? and ????
			float*	float_random_number_array;		// values in [0,1.0]
			int32	random_array_length;

			uint32	normal_stream_position;
			uint32	uniform_stream_position;
			uint32	integer_uniform_stream_position;

			int32 	seed;

	inline	float	FloatNoise(int32);
	inline	int32	IntegerNoise(int32);

public:
					RandomNumberGenerator(int32 seed_number, int32 minimum_sequence_length);
					~RandomNumberGenerator();

			void	ResetStreams();

	inline	float	StandardNormalDistribution();
	inline	float	UniformDistribution(float a, float b);
	inline	int32	IntegerUniformDistribution(int32 a, int32 b);
};


inline float RandomNumberGenerator::FloatNoise(int32 s)
{
	int32 n = (s + seed) * 57;
	n = n << 13 ^ n;
	return(1.0 - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}


inline int32 RandomNumberGenerator::IntegerNoise(int32 s)
{
	int32 n = (s + seed) * 57;
	n = n << 13 ^ n;
	return (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
}


inline float RandomNumberGenerator::StandardNormalDistribution()
{
	if (random_array_length > 0)
		return 0.0;
	else
		return 0.0;
}


inline float RandomNumberGenerator::UniformDistribution(float a, float b)
{
	if (random_array_length > 0) {
		return
			a + (b - a) * float_random_number_array[uniform_stream_position++
			% random_array_length];
	}
	else
		return a + (b - a) * FloatNoise(uniform_stream_position++);
}


inline int32 RandomNumberGenerator::IntegerUniformDistribution(int32 a, int32 b)
{
	if (random_array_length > 0) {
		return
			a + integer_random_number_array[integer_uniform_stream_position++
			% random_array_length] % (b - a + 1);
 	}
	else
		return a + IntegerNoise(integer_uniform_stream_position++) % (b - a + 1);
}


#endif // RANDOM_NUMBER_GENERATOR_H
