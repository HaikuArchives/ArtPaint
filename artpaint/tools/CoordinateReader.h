/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef COORDINATE_READER_H
#define COORDINATE_READER_H

#include <OS.h>
#include <Point.h>


class ImageView;

enum interpolation_styles {
	NO_INTERPOLATION,
	LINEAR_INTERPOLATION,
	CARDINAL_SPLINE_INTERPOLATION
};


class CoordinateReader {
			sem_id		benaphore_mutex;
			int32		benaphore_count;

			int32		benaphore_debug_variable;


			bool		EnterCS();
			bool		ExitCS();

			bool		continue_reading;
			bool		trace;
			bool		allow_duplicates;
			double		reader_delay;

			ImageView*	view;
			interpolation_styles style;

			struct 		queue_entry {
							BPoint			point;
							queue_entry*	next_entry;
						} *point_queue_head, *point_queue_tail;

			int32		point_queue_length;

			float		interpolation_parameter;
			float		interpolation_step;

			bool		interpolation_started;

			BPoint		p0;
			BPoint		p1;
			BPoint		p2;
			BPoint		p3;

			int32		prev_x;
			int32		prev_y;


	// These are the cardinal blending-functions.
	inline	float		car0(float);
	inline	float		car1(float);
	inline	float		car2(float);
	inline	float		car3(float);

			thread_id	reader_thread;

	static	int32		thread_entry(void*);
			int32		reader_function();

			status_t	NextPointNoInterpolation(BPoint& point);
			status_t	NextPointLinearInterpolation(BPoint& point, int32 step_factor);
			status_t	NextPointCardinalSplineInterpolation(BPoint& point, int32 step_factor);

	inline	float		round(float);

public:
					CoordinateReader(ImageView* ,interpolation_styles = NO_INTERPOLATION,
						bool trace_path = FALSE, bool duplicates = FALSE, double delay = 10000.0);
					~CoordinateReader();

			status_t	GetPoint(BPoint& point, int32 step_factor = 1);
};


// This is just an utility-function for rounding the values.
// It can be made faster by using inline assebler code. Especially
// instructions frsp and fctlw could be useful on the PPC.
inline float CoordinateReader::round(float c)
{
	return (((c - floor(c)) > 0.5) ? ceil(c) : floor(c));
}


float CoordinateReader::car0(float u)
{
	float s = 1.0;
	return 2 * s * u * u - s * u * u * u - s * u;
}


float CoordinateReader::car1(float u)
{
	float s = 1.0;
	return (2 - s) * u * u * u + (s - 3) * u * u + 1;
}


float CoordinateReader::car2(float u)
{
	float s = 1.0;
	return (s - 2) * u * u * u + (3 - 2 * s) * u * u + s * u;
}


float CoordinateReader::car3(float u)
{
	float s = 1.0;
	return s * u * u * u - s * u * u;
}


#endif // COORDINATE_READER_H
