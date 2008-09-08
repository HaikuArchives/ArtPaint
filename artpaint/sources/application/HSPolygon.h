/*

	Filename:	HSPolygon.h
	Contents:	HSPolygon-class declaration.
	Author:		Heikki Suhonen

*/


/*
	HSPolygon is a polygon representation that can do operations
	such as translation, rotation and changing of size. It cannot
	draw itself, but it will return a BPolygon representation of itself
	when asked.

*/

#ifndef HS_POLYGON_H
#define HS_POLYGON_H


#include <Point.h>
#include <Polygon.h>

enum polygon_direction {
	HS_POLYGON_CLOCKWISE,
	HS_POLYGON_COUNTERCLOCKWISE,
	HS_POLYGON_ANY_DIRECTION
};

class HSPolygon {
BPoint				*points;
int32				point_count;
polygon_direction	direction;


public:
				HSPolygon(BPoint*,int32,polygon_direction dir=HS_POLYGON_ANY_DIRECTION);
				HSPolygon(const HSPolygon*);
				~HSPolygon();

void			AddPoints(BPoint*,int32,bool reverse_points=FALSE);
void			Rotate(const BPoint&,float);
void			RotateAboutCenter(float);

void			TranslateBy(int32,int32);

BPolygon*		GetBPolygon();


// This function checks that each point has follower within the parameter
// distance. If there is no such point it will add points that are needed.
// The distance between the points is the euclidean distance and units are
// pixels.
void			SetMaximumInterPointDistance(float);


inline	BPoint*	GetPointList() { return points; }
inline	int32	GetPointCount() { return point_count; }
		void	RoundToInteger();
		BRect	BoundingBox();

		void	ChangeDirection(polygon_direction);

polygon_direction	GetDirection();


		bool	operator==(const HSPolygon&);
};

#endif
