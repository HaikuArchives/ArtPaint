/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef INTELLIGENT_PATH_FINDER_H
#define	INTELLIGENT_PATH_FINDER_H

#include <OS.h>
#include <SupportDefs.h>

/*
	This class calculates shortest paths between seed-point and other points.
	This is primarily intended to be used with intelligent scissors but it could
	be used for other purposes too.
*/
class OrderedPointList {
		struct point {
			point	*next_point;
			point	*prev_point;
			int32	x;
			int32	y;
			uint16	cost;
		};

uint16	highest_cost;
point	*point_list_head;
point	**cost_limits;

public:
		OrderedPointList();
		~OrderedPointList();

void	RemoveLowestCostPoint(int32 *x,int32 *y,uint16 *cost);
void	RemovePoint(int32 x,int32 y,uint16 cost);
void	InsertPoint(int32 x, int32 y,uint16 cost);
int32	ContainsPoint(int32 x, int32 y,uint16 min_cost);
bool	IsEmpty() { return point_list_head == NULL; }
};


class IntelligentPathFinder {
BBitmap	*original_bitmap;
uint32	*bitmap_bits;
int32	bitmap_bpr;
int32	right_border;
int32	bottom_border;

// Entry that is 0 in local cost map means that the value for that particular pixel
// has not yet been calculated.
uint8	**local_cost_map;
uint8	LocalCost(int32 x, int32 y,int32 dx, int32 dy);


// The total costs and the paths are stored here.
uint16	**total_cost_map;
uint32	**path_pointers_map;	// High 16 bits is the x coordinate while the rest is y coordinate
uint16	ReturnTotalCost(int32 x,int32 y);
void	SetTotalCost(int32 x,int32 y,uint16 cost);
BPoint	ReturnNextPointInPath(int32 x,int32 y);
void	SetNextPointInPath(int32 x,int32 y,int32 nx,int32 ny);
void	ResetTotalCostsAndPaths();

// The expanded pixels are stored here.
uint8	**expanded_bits;		// 1 means that the pixel is expanded, 0 means it is not.
bool	IsExpanded(int32 x, int32 y);
void	SetExpanded(int32 x, int32 y);
void	ResetExpanded();



// Here is the list that holds the active points.
OrderedPointList	*active_point_list;


int32	seed_point_x;
int32	seed_point_y;


thread_id		dp_thread;

static	int32	dp_thread_entry(void*);
		int32	dp_thread_function();

static	int32	lc_thread_entry(void*);
		int32	lc_thread_function();


// The next variables are used in controlling the two threads.
bool	calculation_continuing;
bool	seed_point_changed;

void	PrintCostMap();

public:
		IntelligentPathFinder(BBitmap*);
		~IntelligentPathFinder();

void		SetSeedPoint(int32 x, int32 y);
BPoint*		ReturnPath(int32 x, int32 y,int32 *num_points);
};

#endif
