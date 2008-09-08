/*

	Filename:	RefFilters.h
	Contents:	Declarations for various ref-filters
	Author:		Heikki Suhonen

*/


#ifndef REF_FILTERS_H
#define REF_FILTERS_H

#include <FilePanel.h>

class ImageFilter : public BRefFilter {

public:
bool	Filter(const entry_ref*, BNode*, struct stat*, const char*);
};

#endif
