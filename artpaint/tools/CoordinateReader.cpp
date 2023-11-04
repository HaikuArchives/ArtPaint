/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include <Debug.h>
#include <Window.h>
#include <stdio.h>

#include "CoordinateReader.h"
#include "ImageView.h"


CoordinateReader::CoordinateReader(
	ImageView* v, interpolation_styles s, bool trace_path, bool duplicates, double delay)
{
	view = v;
	style = s;
	trace = trace_path;
	allow_duplicates = duplicates;
	reader_delay = delay;

	point_queue_head = NULL;
	point_queue_tail = NULL;
	point_queue_length = 0;


	interpolation_parameter = 0.0;
	interpolation_step = 0.0;
	interpolation_started = FALSE;

	benaphore_count = 0;
	benaphore_mutex = create_sem(0, "benaphore_mutex");
	benaphore_debug_variable = 0;

	continue_reading = TRUE;

	reader_thread = spawn_thread(thread_entry, "coordinate_reader", B_NORMAL_PRIORITY, this);
	resume_thread(reader_thread);
}


CoordinateReader::~CoordinateReader()
{
	continue_reading = FALSE;

	int32 return_value;
	wait_for_thread(reader_thread, &return_value);


	delete_sem(benaphore_mutex);
}


status_t
CoordinateReader::GetPoint(BPoint& point, int32 step_factor)
{
	switch (style) {
		case NO_INTERPOLATION:
			return NextPointNoInterpolation(point);
		case LINEAR_INTERPOLATION:
			return NextPointLinearInterpolation(point, step_factor);
		case CARDINAL_SPLINE_INTERPOLATION:
			return NextPointCardinalSplineInterpolation(point, step_factor);
		default:
			return B_ERROR;
	}
}


int32
CoordinateReader::thread_entry(void* data)
{
	CoordinateReader* this_pointer = (CoordinateReader*)data;

	return this_pointer->reader_function();
}


int32
CoordinateReader::reader_function()
{
	BWindow* window = view->Window();
	if (window == NULL)
		return B_ERROR;

	BPoint point;
	BPoint prev_point(-50000, -50000);
	uint32 buttons;

	while (continue_reading == TRUE) {
		window->Lock();
		view->getCoords(&point, &buttons);
		window->Unlock();

		if (buttons != 0) {
			if ((point != prev_point) || (allow_duplicates == TRUE)) {
				EnterCS();
				queue_entry* entry = new queue_entry();
				entry->next_entry = NULL;
				entry->point = point;
				if (point_queue_length == 0) {
					point_queue_head = entry;
					point_queue_tail = entry;
				} else {
					point_queue_tail->next_entry = entry;
					point_queue_tail = entry;
				}

				point_queue_length++;
				ExitCS();
				prev_point = point;
			}
			snooze((bigtime_t)reader_delay);
		} else
			continue_reading = FALSE;
	}

	return B_OK;
}


status_t
CoordinateReader::NextPointNoInterpolation(BPoint& point)
{
	while ((point_queue_length == 0) && (continue_reading))
		//snooze(10 * 1000);
		snooze((bigtime_t)reader_delay);
		
	if (point_queue_head != NULL) {
		EnterCS();
		queue_entry* entry = point_queue_head;
		point_queue_head = entry->next_entry;

		point = entry->point;
		delete entry;

		point_queue_length--;
		ExitCS();

		return B_OK;
	}

	return B_ERROR;
}


status_t
CoordinateReader::NextPointLinearInterpolation(BPoint& point, int32 step_factor)
{
	if (!interpolation_started) {
		// Take the two first interpolation points.
		while ((point_queue_length < 1) && (continue_reading))
			snooze(20 * 1000);

		if (point_queue_length >= 1) {
			EnterCS();
			queue_entry* entry = point_queue_head;
			point_queue_head = point_queue_head->next_entry;
			point_queue_length--;
			if (entry == point_queue_tail)
				point_queue_tail = NULL;

			ExitCS();
			p1 = entry->point;
			delete entry;
		} else
			return B_ERROR;

		interpolation_started = TRUE;
		interpolation_parameter = 1.0;

		point = p1;
		prev_x = (int32)p1.x;
		prev_y = (int32)p1.y;
		return B_OK;
	}

	if (interpolation_parameter >= 1.0) {
		while ((point_queue_length < 1) && (continue_reading))
			snooze(20 * 1000);

		if (point_queue_length >= 1) {
			p0 = p1;
			EnterCS();
			queue_entry* entry = point_queue_head;
			point_queue_head = point_queue_head->next_entry;
			point_queue_length--;
			if (entry == point_queue_tail)
				point_queue_tail = NULL;
			ExitCS();
			p1 = entry->point;
			delete entry;
		} else
			return B_ERROR;

		interpolation_parameter = 0.0;
		interpolation_step = 1.0;
		if (fabs(p0.x - p1.x) > 0)
			interpolation_step = 1.0 / fabs(p0.x - p1.x);
		if (fabs(p0.y - p1.y) > 0)
			interpolation_step = min_c(interpolation_step, 1.0 / fabs(p0.y - p1.y));
	}


	int32 new_y;
	int32 new_x;

	do {
		interpolation_parameter += interpolation_step * step_factor;
		new_x = (int32)round(
			p0.x * (1.0 - interpolation_parameter) + p1.x * interpolation_parameter);
		new_y = (int32)round(
			p0.y * (1.0 - interpolation_parameter) + p1.y * interpolation_parameter);
	} while ((interpolation_parameter < 1.0) && (prev_x == new_x) && (prev_y == new_y));

	if ((prev_x == new_x) && (prev_y == new_y))
		PRINT(("Linear interpolation returning a duplicate point\n"));

	prev_x = new_x;
	prev_y = new_y;


	point.x = new_x;
	point.y = new_y;

	return B_OK;
}


status_t
CoordinateReader::NextPointCardinalSplineInterpolation(BPoint& point, int32 step_factor)
{
	if (!interpolation_started) {
		// Take the four first interpolation points.
		while ((point_queue_length < 4) && (continue_reading))
			snooze(20 * 1000);

		if (point_queue_length >= 4) {
			EnterCS();
			queue_entry* entry = point_queue_head;
			point_queue_head = point_queue_head->next_entry;
			point_queue_length--;
			p0 = entry->point;
			delete entry;

			entry = point_queue_head;
			point_queue_head = point_queue_head->next_entry;
			point_queue_length--;
			p1 = entry->point;
			delete entry;

			entry = point_queue_head;
			point_queue_head = point_queue_head->next_entry;
			point_queue_length--;
			p2 = entry->point;
			delete entry;

			entry = point_queue_head;
			point_queue_head = point_queue_head->next_entry;
			point_queue_length--;
			p3 = entry->point;
			delete entry;

			if (entry == point_queue_tail)
				point_queue_tail = NULL;
			ExitCS();
		} else
			return B_ERROR;

		interpolation_started = TRUE;
		interpolation_parameter = 0.0;
		interpolation_step = 1.0;
		if ((fabs(p1.x - p2.x) > 0) || (fabs(p1.y - p2.y) > 0))
			interpolation_step = 1.0 / (fabs(p1.x - p2.x) + fabs(p1.y - p2.y));

		interpolation_step /= 10.0;

		point = p1;
		prev_x = (int32)point.x;
		prev_y = (int32)point.y;
		return B_OK;
	}

	int32 new_x = prev_x;
	int32 new_y = prev_y;

	while ((prev_x == new_x) && (prev_y == new_y)) {
		if (interpolation_parameter >= 1.0) {
			while ((point_queue_length < 1) && (continue_reading))
				snooze(20 * 1000);

			if (point_queue_length >= 1) {
				p0 = p1;
				p1 = p2;
				p2 = p3;
				EnterCS();
				queue_entry* entry = point_queue_head;
				point_queue_head = point_queue_head->next_entry;
				point_queue_length--;
				if (entry == point_queue_tail)
					point_queue_tail = NULL;
				ExitCS();
				p3 = entry->point;
				delete entry;
			} else
				return B_ERROR;

			interpolation_parameter = 0.0;
			interpolation_step = 1.0;
			if ((fabs(p1.x - p2.x) > 0) || (fabs(p1.y - p2.y) > 0))
				interpolation_step = 1.0 / (fabs(p1.x - p2.x) + fabs(p1.y - p2.y));

			interpolation_step /= 10.0;
		}

		float u = interpolation_parameter;

		interpolation_step *= step_factor;
		do {
			u = min_c(u + interpolation_step, 1);
			new_x
				= (int32)round(p0.x * car0(u) + p1.x * car1(u) + p2.x * car2(u) + p3.x * car3(u));
			new_y
				= (int32)round(p0.y * car0(u) + p1.y * car1(u) + p2.y * car2(u) + p3.y * car3(u));
		} while ((u < 1.0) && (prev_x == new_x) && (prev_y == new_y));

		interpolation_parameter = u;
	}

	if ((prev_x == new_x) && (prev_y == new_y))
		PRINT(("Returning a duplicate point\n"));

	prev_x = new_x;
	prev_y = new_y;
	point.x = new_x;
	point.y = new_y;

	return B_OK;
}


bool
CoordinateReader::EnterCS()
{
	int32 previous = atomic_add(&benaphore_count, 1);
	if (previous >= 1) {
		if (acquire_sem(benaphore_mutex) != B_OK)
			return FALSE;
	}
	return TRUE;
}


bool
CoordinateReader::ExitCS()
{
	int32 previous = atomic_add(&benaphore_count, -1);
	if (previous > 1) {
		release_sem(benaphore_mutex);
		benaphore_debug_variable++;
	}
	return TRUE;
}
