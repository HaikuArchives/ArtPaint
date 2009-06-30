/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef MANIPULATOR_SERVER_H
#define	MANIPULATOR_SERVER_H

#include "Manipulator.h"


class BNode;


class ManipulatorServer {

static	int32		number_of_addons;
static	image_id	*addon_array;
static	int32		max_number_of_addons;
static	bool		addons_available;

static	int32	add_on_reader(void*);
static	BNode*	GetAddOnNode(image_id);
public:
static	Manipulator*	ReturnManipulator(manipulator_type type,int32 add_on_id=-1);
static	void			StoreManipulatorSettings(Manipulator*);

static	void			ReadAddOns();

static	bool			AddOnsAvailable() { return addons_available; }
static	int32			AddOnCount() { return number_of_addons; }
static	image_id*		AddOnArray() { return addon_array; }
};


// ---------------------

class StringSet {
		char	**string_array;
		int32	array_length;
		int32	entry_count;

public:
		StringSet();
		~StringSet();

void	AddString(const char*);
bool	ContainsString(const char*);
};
#endif

