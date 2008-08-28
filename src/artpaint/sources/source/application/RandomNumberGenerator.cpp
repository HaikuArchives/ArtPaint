/* 

	Filename:	RandomNumberGenerator.cpp
	Contents:	RandomNumberGenerator-class definitions.		
	Author:	Heikki Suhonen
	
*/


#include "RandomNumberGenerator.h"


RandomNumberGenerator::RandomNumberGenerator(int32 seed_number, int32 minimum_sequence_length)
{
	random_array_length = 0;
	integer_random_number_array = NULL;
	float_random_number_array = NULL;
	
	normal_stream_position = 0;
	uniform_stream_position = 0;
	integer_uniform_stream_position = 0;

	seed = seed_number;
	
	
	if (minimum_sequence_length < 4096) {
		random_array_length = minimum_sequence_length;
		integer_random_number_array = new int32[random_array_length];				
		float_random_number_array = new float[random_array_length];

		for (int32 i=0;i<random_array_length;i++) {
			integer_random_number_array[i] = IntegerNoise(i);
			float_random_number_array[i] = FloatNoise(i);
		}
	}
}


RandomNumberGenerator::~RandomNumberGenerator()
{
	delete[] integer_random_number_array;
	delete[] float_random_number_array;
}




void RandomNumberGenerator::ResetStreams()
{
	normal_stream_position = 0;
	uniform_stream_position = 0;
}