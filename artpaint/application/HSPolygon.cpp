/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include <math.h>
#include <stdio.h>

#include "HSPolygon.h"

#define PI M_PI


HSPolygon::HSPolygon(BPoint* point_list, int32 num_points, polygon_direction dir)
{
	if (num_points > 0) {
		point_count = 1;
		points = new BPoint[num_points];
		points[0] = point_list[0];
		for (int32 i = 1; i < num_points; i++) {
			if (point_list[i] != point_list[i - 1])
				points[point_count++] = point_list[i];
		}
		direction = dir;
	} else {
		point_count = 0;
		points = NULL;
		direction = HS_POLYGON_ANY_DIRECTION;
	}
}


HSPolygon::HSPolygon(const HSPolygon* poly)
{
	point_count = poly->point_count;
	points = new BPoint[point_count];

	for (int32 i = 0; i < point_count; i++)
		points[i] = poly->points[i];

	direction = poly->direction;
}


HSPolygon::~HSPolygon()
{
	delete[] points;
}


void
HSPolygon::AddPoints(BPoint* point_list, int32 num_points, bool reverse_points)
{
	BPoint* new_point_array = new BPoint[point_count + num_points];

	for (int32 i = 0; i < point_count; i++)
		new_point_array[i] = points[i];

	delete[] points;
	points = new_point_array;

	if (reverse_points) {
		for (int32 i = num_points - 1; i >= 0; i--) {
			// Do not copy same point twice in a row.
			if ((i == 0) || (point_list[i] != point_list[i - 1])) {
				points[point_count] = point_list[i];
				point_count++;
			}
		}
	} else {
		for (int32 i = 0; i < num_points; i++) {
			// Do not copy same point twice in a row.
			if ((i == num_points - 1) || (point_list[i] != point_list[i + 1])) {
				points[point_count] = point_list[i];
				point_count++;
			}
		}
	}
}


void
HSPolygon::Rotate(const BPoint& center, float angle)
{
	// Ensure that the angle is between 180˚ and -180˚
	while (angle > 180)
		angle = angle - 360;
	while (angle < -180)
		angle = angle + 360;


	// The parameter angle is in degrees. We must convert it to radians
	float rad_angle = angle / 360.0 * 2 * PI;

	// Do for each point:
	//	1.	Translate center to origin.
	//	2.	Rotate about origin.
	//	3.	Translate origin to center.

	float new_x, new_y;

	for (int32 i = 0; i < point_count; i++) {
		points[i] = points[i] - center;
		new_x = cos(rad_angle) * points[i].x - sin(rad_angle) * points[i].y;
		new_y = sin(rad_angle) * points[i].x + cos(rad_angle) * points[i].y;
		points[i] = BPoint(new_x, new_y) + center;
	}
}


void
HSPolygon::RotateAboutCenter(float angle)
{
	// Do for each point:
	//	1.	Calculate centroid as an average of all points.
	//	2.	Translate centroid to origin.
	//	3.	Rotate about origin.
	//	4.	Translate origin to centroid.

	BPoint centroid = BPoint(0, 0);
	for (int32 i = 0; i < point_count; i++)
		centroid += points[i];

	centroid.x /= point_count;
	centroid.y /= point_count;

	Rotate(centroid, angle);
}


void
HSPolygon::TranslateBy(int32 dx, int32 dy)
{
	for (int32 i = 0; i < point_count; i++) {
		points[i].x += dx;
		points[i].y += dy;
	}
}


void
HSPolygon::ScaleBy(BPoint origin, float sx, float sy)
{
	for (int32 i = 0; i < point_count; i++) {
		points[i].x = (points[i].x - origin.x) * sx + origin.x;
		points[i].x = min_c(100000, points[i].x);
		points[i].y = (points[i].y - origin.y) * sy + origin.y;
		points[i].y = min_c(100000, points[i].y);
	}
}


void
HSPolygon::FlipX(float axis)
{
	for (int32 i = 0; i < point_count; ++i)
		points[i].x = (axis - points[i].x) + axis;
}


void
HSPolygon::FlipY(float axis)
{
	for (int32 i = 0; i < point_count; ++i)
		points[i].y = (axis - points[i].y) + axis;
}


BPolygon*
HSPolygon::GetBPolygon()
{
	BPolygon* poly = new BPolygon(points, point_count);
	return poly;
}


void
HSPolygon::RoundToInteger()
{
	// This rounds every point in the polygon to nearest point that has integer
	// as it's coordinates
	for (int32 i = 0; i < point_count; i++) {
		points[i].x
			= (((points[i].x - floor(points[i].x)) < 0.5) ? floor(points[i].x) : ceil(points[i].x));
		points[i].y
			= (((points[i].y - floor(points[i].y)) < 0.5) ? floor(points[i].y) : ceil(points[i].y));
	}
}


BRect
HSPolygon::BoundingBox()
{
	BRect b_box = BRect(1000000, 1000000, -1000000, -1000000);

	for (int32 i = 0; i < point_count; i++) {
		b_box.left = floor(min_c(b_box.left, points[i].x));
		b_box.top = floor(min_c(b_box.top, points[i].y));
		b_box.right = ceil(max_c(b_box.right, points[i].x));
		b_box.bottom = ceil(max_c(b_box.bottom, points[i].y));
	}

	return b_box;
}


void
HSPolygon::SetMaximumInterPointDistance(float max_dist)
{
	float current_distance;

	// First count how many new points are needed
	int32 additional_point_count = 0;
	for (int32 i = 0; i < point_count; i++) {
		BPoint p1 = points[i % point_count];
		BPoint p2 = points[(i + 1) % point_count];
		current_distance = sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
		additional_point_count += (int32)floor(current_distance / max_dist);
	}

	// Then reserve a new array for all points
	BPoint* new_points = new BPoint[point_count + additional_point_count];

	// Then copy the points to new_points and add needed points between
	// other points. New points are generated as linear interpolation between
	// the two endpoints.
	int32 number = 0;
	for (int32 i = 0; i < point_count; i++) {
		new_points[number] = points[i];
		BPoint p1 = points[i % point_count];
		BPoint p2 = points[(i + 1) % point_count];
		current_distance = sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));

		float add_amount = floor(current_distance / max_dist);
		if (add_amount > 0) {
			add_amount = add_amount + 1;
			for (float j = 1; j < add_amount; j++) {
				number++;
				BPoint new_point;
				new_point.x = p2.x * j / add_amount + p1.x * (add_amount - j) / add_amount;
				new_point.y = p2.y * j / add_amount + p1.y * (add_amount - j) / add_amount;
				new_points[number] = new_point;
			}
		}
		number++;
	}
	delete[] points;
	point_count += additional_point_count;
	points = new_points;
}


void
HSPolygon::ChangeDirection(polygon_direction dir)
{
	// A polygon might actually have some part going clockwise and
	// some other part going counterclockwise. If we first simplify the
	// polygon to contain only the outlines, then it will be either clockwise
	// or counterclockwise.

	if (dir == HS_POLYGON_ANY_DIRECTION)
		return;

	if (direction == HS_POLYGON_ANY_DIRECTION) {
		// Check what the direction is. This is done by
		// adding the sums of the angles between edges.
		// The sum will be either positive or negative depending
		// whether the polygon is clockwise or counterclockwise.
		float angle_sum = 0;

		for (int32 i = 0; i < point_count; i++) {
			BPoint p1 = points[i % point_count];
			BPoint p2 = points[(i + 1) % point_count];
			BPoint p3 = points[(i + 2) % point_count];

			// The angle is between p1,p2 and p3. That is the same angle as
			// between vectors (p2-p1) and (p3-p2)
			BPoint v1 = p2 - p1;
			BPoint v2 = p3 - p2;

			float v1_length = sqrt(v1.x * v1.x + v1.y * v1.y);
			float v2_length = sqrt(v2.x * v2.x + v2.y * v2.y);

			float cross_product;
			cross_product = v1.x * v2.y - v2.x * v1.y;
			float angle;

			angle = asin(fabs(cross_product) / (v1_length * v2_length)) / PI * 180;
			angle_sum += angle;
		}
	}

	if (direction != dir) {
		// Swap the direction
		BPoint* new_points = new BPoint[point_count];
		for (int32 i = 0; i < point_count; i++)
			new_points[i] = points[point_count - 1 - i];

		delete[] points;
		points = new_points;
		direction = dir;
	}
}


polygon_direction
HSPolygon::GetDirection()
{
	return direction;
}


bool
HSPolygon::Contains(BPoint test_point)
{
	BRect b_box = BoundingBox();
	bool inside = false;

	if (b_box.Contains(test_point) == false)
		return inside;

	for (int i = 0, j = point_count - 1; i < point_count; j = i++) {
		if ((points[i].y > test_point.y) != (points[j].y > test_point.y)
			&& test_point.x < (points[j].x - points[i].x)
			* (test_point.y - points[i].y) / (points[j].y - points[i].y)
			+ points[i].x)
				inside = !inside;
	}

	return inside;
}


bool
HSPolygon::Contains(int32 x, int32 y)
{
	return Contains(BPoint(x, y));
}


bool
HSPolygon::operator==(const HSPolygon& poly)
{
	bool similar = (point_count == poly.point_count);

	for (int32 i = 0; (i < point_count) && similar; i++)
		similar = (points[i] == poly.points[i]);

	return similar;
}
