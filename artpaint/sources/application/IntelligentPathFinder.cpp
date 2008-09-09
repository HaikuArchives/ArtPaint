/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <StopWatch.h>


#include "IntelligentPathFinder.h"


IntelligentPathFinder::IntelligentPathFinder(BBitmap *bm)
{
	original_bitmap = bm;
	bitmap_bits = (uint32*)bm->Bits();
	bitmap_bpr = bm->BytesPerRow()/4;
	right_border = (int32)bm->Bounds().right + 1;
	bottom_border = (int32)bm->Bounds().bottom + 1;

	local_cost_map = new uint8*[right_border];
	for (int32 i=0;i<right_border;i++) {
		local_cost_map[i] = new uint8[bottom_border];
	}
	for (int32 y=0;y<bottom_border;y++) {
		for (int32 x=0;x<right_border;x++) {
			local_cost_map[x][y] = 0x00;
		}
	}

	total_cost_map = new uint16*[right_border];
	path_pointers_map = new uint32*[right_border];
	for (int32 i=0;i<right_border;i++) {
		total_cost_map[i] = new uint16[bottom_border];
		path_pointers_map[i] = new uint32[bottom_border];
		for (int32 y=0;y<bottom_border;y++) {
			total_cost_map[i][y] = 0x0000;
			path_pointers_map[i][y]= 0x00000000;
		}
	}

	expanded_bits = new uint8*[(right_border>>3)+1];
	for (int32 i=0;i<(right_border>>3)+1;i++) {
		expanded_bits[i] = new uint8[bottom_border];
		for (int32 y=0;y<bottom_border;y++) {
			expanded_bits[i][y] = 0x00;
		}
	}

	calculation_continuing = TRUE;
	seed_point_x = -1;
	seed_point_y = -1;

	dp_thread = spawn_thread(dp_thread_entry,"dp_thread",B_NORMAL_PRIORITY,this);
	resume_thread(dp_thread);

}


IntelligentPathFinder::~IntelligentPathFinder()
{
	calculation_continuing = FALSE;
	int32 return_value;
	wait_for_thread(dp_thread,&return_value);

	for (int32 x=0;x<right_border;x++) {
		delete[] local_cost_map[x];
		local_cost_map[x] = NULL;
	}
	for (int32 i=0;i<right_border;i++) {
		delete[] total_cost_map[i];
		total_cost_map[i] = NULL;

		delete[] path_pointers_map[i];
		path_pointers_map[i] = NULL;
	}
	delete[] local_cost_map;
	delete[] total_cost_map;
	delete[] path_pointers_map;

	for (int32 i=0;i<(right_border>>3)+1;i++) {
		delete[] expanded_bits[i];
		expanded_bits[i] = NULL;
	}
	delete[] expanded_bits;

}


void IntelligentPathFinder::SetSeedPoint(int32 x,int32 y)
{
	seed_point_x = x;
	seed_point_y = y;
	seed_point_changed = TRUE;
}

BPoint* IntelligentPathFinder::ReturnPath(int32 x, int32 y,int32 *num_points)
{
	x = max_c(0,min_c(right_border-1,x));
	y = max_c(0,min_c(bottom_border-1,y));
	int32 point_array_length = 128;
	BPoint *point_array = new BPoint[point_array_length];

	int32 next_x = x;
	int32 next_y = y;
	int32 point_count = 0;

	while (((next_x != seed_point_x) || (next_y != seed_point_y)) && ((next_x >= 0) && (next_y >= 0))) {
		if (point_count == point_array_length) {
			point_array_length *= 2;
			BPoint *new_array = new BPoint[point_array_length];
			for (int32 i=0;i<point_count;i++) {
				new_array[i] = point_array[i];
			}
			delete[] point_array;
			point_array = new_array;
		}
		BPoint point = ReturnNextPointInPath(next_x,next_y);
		next_x = (int32)point.x;
		next_y = (int32)point.y;
		point_array[point_count++] = point;
	}

	*num_points = point_count;

	if (point_count > 1)
		return point_array;
	else {
		delete[] point_array;
		return NULL;
	}
}


uint8 IntelligentPathFinder::LocalCost(int32 x, int32 y, int32 dx, int32 dy)
{
	if (local_cost_map[x][y] != 0x00) {
		if (abs(dx)+abs(dy)>1)
			return local_cost_map[x][y];
		else
			return (uint8)(local_cost_map[x][y]/sqrt(2.0));
	}
	else {
		// Here we must calculate the local cost between x,y and all of its neighbours.
		union {
			uint8 bytes[4];
			uint32 word;
		} lt,t,rt,l,r,lb,b,rb,c;
		int32 new_x,new_y;
		new_x = max_c(min_c(x-1,right_border-1),0);
		new_y = max_c(min_c(y-1,bottom_border-1),0);
		lt.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x,right_border-1),0);
		new_y = max_c(min_c(y-1,bottom_border-1),0);
		t.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x+1,right_border-1),0);
		new_y = max_c(min_c(y-1,bottom_border-1),0);
		rt.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x-1,right_border-1),0);
		new_y = max_c(min_c(y,bottom_border-1),0);
		l.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x+1,right_border-1),0);
		new_y = max_c(min_c(y,bottom_border-1),0);
		r.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x-1,right_border-1),0);
		new_y = max_c(min_c(y+1,bottom_border-1),0);
		lb.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x,right_border-1),0);
		new_y = max_c(min_c(y+1,bottom_border-1),0);
		b.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		new_x = max_c(min_c(x+1,right_border-1),0);
		new_y = max_c(min_c(y+1,bottom_border-1),0);
		rb.word = *(bitmap_bits + new_x + new_y*bitmap_bpr);

		c.word = *(bitmap_bits + x + y*bitmap_bpr);

		// Here we can calculate the costs. Costs are calculated
		// for each RGB(A)-element and then summed.
		// For now lets just use the inverse of gradient magnitude as the cost.
		int32 gradient_magnitude;
		int32 red_magn,green_magn,blue_magn;
		red_magn = 	abs(lb.bytes[2] - lt.bytes[2]) +
					abs(2*b.bytes[2] - 2*t.bytes[2]) +
					abs(rb.bytes[2] - rt.bytes[2]);

		green_magn =	abs(lb.bytes[1] - lt.bytes[1]) +
						abs(2*b.bytes[1] - 2*t.bytes[1]) +
						abs(rb.bytes[1] - rt.bytes[1]);

		blue_magn =	abs(lb.bytes[0] - lt.bytes[0]) +
					abs(2*b.bytes[0] - 2*t.bytes[0]) +
					abs(rb.bytes[0] - rt.bytes[0]);

		gradient_magnitude = red_magn+blue_magn+green_magn;

		red_magn = 	abs(rt.bytes[2] - lt.bytes[2]) +
					abs(2*r.bytes[2] - 2*l.bytes[2]) +
					abs(rb.bytes[2] - lb.bytes[2]);

		green_magn =	abs(rt.bytes[1] - lt.bytes[1]) +
						abs(2*r.bytes[1] - 2*l.bytes[1]) +
						abs(rb.bytes[1] - lb.bytes[1]);

		blue_magn = abs(rt.bytes[0] - lt.bytes[0]) +
					abs(2*r.bytes[0] - 2*l.bytes[0]) +
					abs(rb.bytes[0] - lb.bytes[0]);

		gradient_magnitude = max_c(gradient_magnitude,red_magn+blue_magn+green_magn);

		uint8 red_zero,green_zero,blue_zero;

		red_zero = (abs(8*c.bytes[2] - (lt.bytes[2] + t.bytes[2]
					+ rt.bytes[2] + l.bytes[2] + r.bytes[2]
					+ lb.bytes[2] + b.bytes[2] + rb.bytes[2])) < 2 ? 32 : 0);

		green_zero = (abs(8*c.bytes[1] - (lt.bytes[1] + t.bytes[1]
					+ rt.bytes[1] + l.bytes[1] + r.bytes[1]
					+ lb.bytes[1] + b.bytes[1] + rb.bytes[1])) < 2 ? 32 : 0);

		blue_zero = (abs(8*c.bytes[0] - (lt.bytes[0] + t.bytes[0]
					+ rt.bytes[0] + l.bytes[0] + r.bytes[0]
					+ lb.bytes[0] + b.bytes[0] + rb.bytes[0])) < 2 ? 32 : 0);


		gradient_magnitude = (int32)(32 - (gradient_magnitude / 3000.0)*32);
		uint8 value = red_zero + green_zero + blue_zero + gradient_magnitude;

		local_cost_map[x][y] = value;

		if (abs(dx)+abs(dy)>1)
			return value;
		else
			return (uint8)(value / sqrt(2.0));
	}
}


uint16 IntelligentPathFinder::ReturnTotalCost(int32 x, int32 y)
{
	return total_cost_map[x][y];
}

void IntelligentPathFinder::SetTotalCost(int32 x, int32 y, uint16 cost)
{
	total_cost_map[x][y] = cost;
}

BPoint IntelligentPathFinder::ReturnNextPointInPath(int32 x, int32 y)
{
	if (ReturnTotalCost(x,y) > 0) {
		int32 value = path_pointers_map[x][y];
		BPoint point;
		point.x = (value >> 16) & 0xFFFF;
		point.y = value & 0xFFFF;

		return point;
	}
	else {
		BPoint point(-1,-1);
		return point;
	}

}


void IntelligentPathFinder::SetNextPointInPath(int32 x, int32 y, int32 nx, int32 ny)
{
	path_pointers_map[x][y] = (nx<<16) | ny;
}

void IntelligentPathFinder::ResetTotalCostsAndPaths()
{
	for (int32 i=0;i<right_border;i++) {
		for (int32 y=0;y<bottom_border;y++) {
			total_cost_map[i][y] = 0x0000;
			path_pointers_map[i][y]= 0x00000000;
		}
	}
}


bool IntelligentPathFinder::IsExpanded(int32 x, int32 y)
{
	return (expanded_bits[x>>3][y] >> (7-x&0x7)) & 0x01;
}

void IntelligentPathFinder::SetExpanded(int32 x, int32 y)
{
	expanded_bits[x>>3][y] |= 0x01 << (7-x&0x7);
}

void IntelligentPathFinder::ResetExpanded()
{
	for (int32 i=0;i<(right_border>>3)+1;i++) {
		for (int32 y=0;y<bottom_border;y++) {
			expanded_bits[i][y] = 0x00;
		}
	}
}



int32 IntelligentPathFinder::dp_thread_entry(void *data)
{
	IntelligentPathFinder *this_pointer = (IntelligentPathFinder*)data;
	return this_pointer->dp_thread_function();
}


int32 IntelligentPathFinder::dp_thread_function()
{
	// Wait until the seed-point is set.
	while ((seed_point_x == -1) && (seed_point_y == -1) && calculation_continuing) {
		snooze(50 * 1000);
	}
	seed_point_changed = FALSE;

	while (calculation_continuing) {
		active_point_list = new OrderedPointList();
		active_point_list->InsertPoint(seed_point_x,seed_point_y,0);
		SetTotalCost(seed_point_x,seed_point_y,0);
		while (!active_point_list->IsEmpty() && !seed_point_changed && calculation_continuing) {
			int32 x,y;
			uint16 cost;
			active_point_list->RemoveLowestCostPoint(&x,&y,&cost);
			SetExpanded(x,y);

			for (int32 dy=-1;dy<=1;dy++) {
				for (int32 dx=-1;dx<=1;dx++) {
					if ((dx != 0) || (dy != 0)) {
						if ((x+dx >= 0) && (x+dx<right_border) &&
							(y+dy >= 0) && (y+dy<bottom_border)) {
							if (IsExpanded(x+dx,y+dy) == FALSE) {
								uint16 temp_cost = ReturnTotalCost(x,y) + LocalCost(x,y,dx,dy);
								uint16 previous_cost = ReturnTotalCost(x+dx,y+dy);
								if ((previous_cost > 0) && (temp_cost < previous_cost))
									active_point_list->RemovePoint(x+dx,y+dy,previous_cost);
								if ((previous_cost == 0) || (temp_cost < previous_cost)) {
									SetTotalCost(x+dx,y+dy,temp_cost);
									SetNextPointInPath(x+dx,y+dy,x,y);
									active_point_list->InsertPoint(x+dx,y+dy,temp_cost);
								}
							}
						}
					}
				}
			}
		}
		while (!seed_point_changed && calculation_continuing)
			snooze(100 * 1000);	// Calculated all costs.

		if (calculation_continuing) {
			delete active_point_list;
			ResetExpanded();
			ResetTotalCostsAndPaths();
			seed_point_changed = FALSE;
		}

	}

	return B_OK;
}


int32 IntelligentPathFinder::lc_thread_entry(void *data)
{
	return B_NO_ERROR;
}


int32 IntelligentPathFinder::lc_thread_function()
{
	return B_NO_ERROR;
}


void IntelligentPathFinder::PrintCostMap()
{
	for (int32 y=0;y<bottom_border;y++) {
		for (int32 x=0;x<right_border;x++) {
			printf("%d ",ReturnTotalCost(x,y));
		}
		printf("\n");
	}
}

// -------------------------


OrderedPointList::OrderedPointList()
{
	point_list_head = NULL;
	cost_limits = new point*[(int32)pow(2,16)];
	for (int32 i=0;i<pow(2,16);i++)
		cost_limits[i] = NULL;

	highest_cost = 0;
}


OrderedPointList::~OrderedPointList()
{
	point *spare = point_list_head;
	while (spare != NULL) {
		point *helper = spare->next_point;
		delete spare;
		spare = helper;
	}

	for (int32 i=0;i<pow(2,16);i++) {
		cost_limits[i] = NULL;
	}
	delete[] cost_limits;
}


void OrderedPointList::RemoveLowestCostPoint(int32 *x, int32 *y, uint16 *cost)
{
	if (point_list_head != NULL) {
		point *spare = point_list_head;
		point_list_head = spare->next_point;
		*x = spare->x;
		*y = spare->y;
		*cost = spare->cost;
		if (point_list_head != NULL) {
			point_list_head->prev_point = NULL;
			if (point_list_head->cost == *cost) {
				cost_limits[*cost] = point_list_head;
			}
			else {
				cost_limits[*cost] = NULL;
			}
		}
		else {
			highest_cost = 0;
			cost_limits[*cost] = NULL;
		}
		delete spare;
	}
	else {
		*x = -1;
		*y = -1;
		*cost = 0;
	}
}

void OrderedPointList::RemovePoint(int32 x, int32 y, uint16 cost)
{
	point *spare = cost_limits[cost];
	while ((spare != NULL) && ((spare->x != x) && (spare->y != y) && (spare->cost == cost))) {
		spare = spare->next_point;
	}
	if ((spare != NULL) && (spare->cost != cost))
		spare = NULL;

	if (spare != NULL) {
		if (spare->prev_point != NULL) {
			spare->prev_point->next_point = spare->next_point;
		}
		if (spare->next_point != NULL) {
			spare->next_point->prev_point = spare->prev_point;
		}
		if (spare == cost_limits[cost]) {
			if ((spare->next_point != NULL) && (spare->next_point->cost == cost))
				cost_limits[cost] = spare->next_point;
			else {
				cost_limits[cost] = NULL;
				if (highest_cost == cost) {
					while ((highest_cost > 0) && (cost_limits[highest_cost] == NULL))
						highest_cost--;
				}
			}
		}
		if (point_list_head == spare) {
			point_list_head = spare->next_point;
		}
		delete spare;
	}
}


void OrderedPointList::InsertPoint(int32 x, int32 y,uint16 cost)
{
	point *spare = new point;
	spare->x = x;
	spare->y = y;
	spare->cost = cost;
	spare->next_point = NULL;
	spare->prev_point = NULL;

	if (cost_limits[cost] != NULL) {
		spare->next_point = cost_limits[cost];
		spare->prev_point = cost_limits[cost]->prev_point;
		if (spare->next_point != NULL)
			spare->next_point->prev_point = spare;
		if (spare->prev_point != NULL)
			spare->prev_point->next_point = spare;
		cost_limits[cost] = spare;
	}
	else {
		if (cost < highest_cost) {
			int32 next_cost = cost+1;
			while (cost_limits[next_cost] == NULL)
				next_cost++;


			spare->next_point = cost_limits[next_cost];
			spare->prev_point = cost_limits[next_cost]->prev_point;
			if (spare->next_point != NULL)
				spare->next_point->prev_point = spare;
			if (spare->prev_point != NULL)
				spare->prev_point->next_point = spare;
			cost_limits[cost] = spare;
		}
		else {
			int32 prev_cost = cost - 1;
			while ((prev_cost >= 0) && (cost_limits[prev_cost] == NULL))
				prev_cost--;

			if (prev_cost >= 0) {
				point *helper = cost_limits[prev_cost];

				if (helper != NULL) {
					while ((helper->next_point != NULL) && (helper->cost < cost))
						helper = helper->next_point;

					spare->next_point = helper;
					spare->prev_point = helper->prev_point;
					if (spare->next_point != NULL)
						spare->next_point->prev_point = spare;
					if (spare->prev_point != NULL)
						spare->prev_point->next_point = spare;
					cost_limits[cost] = spare;
				}
				else {
					cost_limits[cost] = spare;
				}
			}
			else {
				cost_limits[cost] = spare;
			}
		}
	}

	if (spare->prev_point == NULL)
		point_list_head = spare;

	highest_cost = max_c(highest_cost,cost);
}


int32 OrderedPointList::ContainsPoint(int32 x, int32 y,uint16 min_cost)
{
	if (min_cost > highest_cost)
		return -1;
	while (cost_limits[min_cost] == NULL)
		min_cost++;

	point *spare = cost_limits[min_cost];

	while (spare != NULL) {
		if ((spare->x == x) && (spare->y == y))
			return spare->cost;

		spare = spare->next_point;
	}

	return -1;
}
