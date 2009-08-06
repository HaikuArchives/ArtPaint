/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef REF_FILTERS_H
#define REF_FILTERS_H

#include <FilePanel.h>


class ImageFilter : public BRefFilter {
public:
			bool		Filter(const entry_ref* entryRef, BNode* node,
							struct stat_beos* stat, const char* fileType);
};

#endif
