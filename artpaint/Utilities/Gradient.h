/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef GRADIENT_H
#define GRADIENT_H


#include <ObjectList.h>
#include <Locker.h>

#include <math.h>


struct ColorStop {
	float pos;
	uint32 color;
};


class Gradient {
public:
									Gradient();
									~Gradient();

			void					addStop(float pos, uint32 color);
			void					insertStop(float pos, uint32 color, int index);
			void					deleteStop(int index);
			void					clearStops();

			uint32					getColor(float pos);
			uint32					getAverageColor(float pos, float pos1, float pos2,
										float pos3, float pos4);
private:
			void					recalculateLUT();
			float					getLuminance(uint32 color);

			uint16					resolution;
			uint32 					lut[1024];
			BObjectList<ColorStop>	stops;
			BLocker*				fStopLocker;
};



#endif // GRADIENT_H
