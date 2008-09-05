/*
	Filename:	ColorDistanceMetric.cpp
	Contents:	Color distance measuring class.
	Author:		Heikki Suhonen (Heikki.Suhonen@Helsinki.FI)
*/


#include <math.h>


#include "ColorDistanceMetric.h"


BLocker ColorDistanceMetric::ctr_dtr_locker("ColorDistanceMetric::ctr_dtr_locker");
float* ColorDistanceMetric::sqrt_table = NULL;
int* ColorDistanceMetric::sqr_table = NULL;
int ColorDistanceMetric::instance_counter = 0;


ColorDistanceMetric::ColorDistanceMetric() {
	ctr_dtr_locker.Lock();
	instance_counter++;
	if (sqrt_table == NULL) {
		sqrt_table = new float[196608];	// 3*(256^2)
		float x=0;
		for (int32 i=0;i<196608;i++) {
			sqrt_table[i] = sqrt(x);
			x++;
		}

		sqr_table = new int[513];  // -256 to -1, 0, 1 to 256

		for (int32 i=0;i<513;i++) {
			sqr_table[i] = (i-256)*(i-256);
		}
		sqr_table = &sqr_table[256];
	}
	ctr_dtr_locker.Unlock();
}

ColorDistanceMetric::~ColorDistanceMetric() {
	ctr_dtr_locker.Lock();
	instance_counter--;
	if ((instance_counter == 0) && (sqrt_table != NULL)) {
		delete[] sqrt_table;
		sqrt_table = NULL;

		sqr_table = &sqr_table[-256];
		delete[] sqr_table;
		sqr_table = NULL;
	}
	ctr_dtr_locker.Unlock();
}
