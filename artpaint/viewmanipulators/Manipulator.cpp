/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */

#include "Manipulator.h"


#include <Bitmap.h>


#include <new>
#include <stdlib.h>


Manipulator::Manipulator()
{
	add_on_id = -1;
	fSystemClockSpeed = 0;

	system_info info;
	get_system_info(&info);
	fCpuCount = info.cpu_count;
	fSystemClockSpeed = 0;
	uint32 topoCount = 0;
	get_cpu_topology_info(NULL, &topoCount);
	if (topoCount > 0) {
		cpu_topology_node_info topology[topoCount];
		get_cpu_topology_info(topology, &topoCount);
		for (int i = 0; i < topoCount; ++i) {
			if (topology[i].type == B_TOPOLOGY_CORE)
				fSystemClockSpeed += topology[i].data.core.default_frequency;
		}
	} else {
		// fall back to cpu info; note that current_frequency
		// is dynamic and could be less than the cpu speed
		cpu_info cpuInfos[fCpuCount];
		get_cpu_info(0, fCpuCount, cpuInfos);
		for (int i = 0; i < fCpuCount; ++i)
			fSystemClockSpeed += cpuInfos[i].current_frequency;
	}
}

BBitmap*
Manipulator::DuplicateBitmap(BBitmap* source, int32 inset, bool acceptViews)
{
	if (inset == 0)
		return (new (std::nothrow) BBitmap(source, acceptViews));

	BRect sourceBounds = source->Bounds();
	BRect targetBounds = sourceBounds;
	targetBounds.InsetBy(inset, inset);

	BBitmap* target = new (std::nothrow) BBitmap(targetBounds, B_RGB32,
		acceptViews);
	if (!target || !target->IsValid()) {
		delete target;
		return NULL;
	}

	uint32* targetBits = (uint32*)target->Bits();
	uint32* sourceBits = (uint32*)source->Bits();
	uint32 sourceBpr = source->BytesPerRow() / 4;

	if (inset > 0) {
		// Just leave some of the edge away from the target
		int32 width = targetBounds.IntegerWidth();
		int32 height = targetBounds.IntegerHeight();
		for (int32 y = 0; y <= height; ++y) {
			for (int32 x = 0; x <= width; ++x) {
				*targetBits++ =
					*(sourceBits + inset + x + (inset + y) * sourceBpr);
			}
		}
	} else if (inset < 0) {
		// This is the hardest case where we have to duplicate the edges.
		// First duplicate the top row
		int32 width = sourceBounds.IntegerWidth();
		int32 height = sourceBounds.IntegerHeight();
		for (int32 y = 0; y < abs(inset); ++y) {
			// Duplicate the start of the row
			for (int32 addx = 0; addx < abs(inset); ++addx)
				*targetBits++ = *sourceBits;

			// Duplicate the actual row
			for (int32 x = 0; x <= width; ++x)
				*targetBits++ = *sourceBits++;

			// Duplicate the end of the row
			sourceBits--;
			for (int32 addx = 0; addx < abs(inset); ++addx)
				*targetBits++ = *sourceBits;

			sourceBits = sourceBits + 1 - sourceBpr;
		}

		// Then duplicate the actual bitmap
		for (int32 y = 0; y <= height; ++y) {
			// Duplicate the start of the row
			for (int32 addx = 0; addx < abs(inset); ++addx)
				*targetBits++ = *sourceBits;

			// Duplicate the actual row
			for (int32 x = 0; x <= width; ++x)
				*targetBits++ = *sourceBits++;

			// Duplicate the end of the row
			sourceBits--;
			for (int32 addx = 0; addx < abs(inset); ++addx)
				*targetBits++ = *sourceBits;

			sourceBits++;
		}

		// Then duplicate the last row
		sourceBits = sourceBits - sourceBpr;
		for (int32 y = 0; y < abs(inset); ++y) {
			// Duplicate the start of the row
			for (int32 addx = 0; addx < abs(inset); ++addx)
				*targetBits++ = *sourceBits;

			// Duplicate the actual row
			for (int32 x = 0; x <= width; ++x)
				*targetBits++ = *sourceBits++;

			// Duplicate the end of the row
			sourceBits--;
			for (int32 addx = 0; addx < abs(inset); ++addx)
				*targetBits++ = *sourceBits;

			sourceBits = sourceBits + 1 - sourceBpr;
		}
	}

	return target;
}


