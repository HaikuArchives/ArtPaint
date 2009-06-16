/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <julun.host
 *
 */

#include "RefFilters.h"

#include <Entry.h>
#include <NodeInfo.h>

#include <string.h>
#include <compat/sys/stat.h>


bool
ImageFilter::Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
	const char* fileType)
{
	if (S_ISDIR(stat->st_mode))
		return true;

	if (S_ISLNK(stat->st_mode)) {
		BEntry entry(ref, true);
		return entry.IsDirectory();
	}

	char mimeType[B_MIME_TYPE_LENGTH];
	BNodeInfo(node).GetType(mimeType);
	if (strncmp("image/", mimeType, 6) == 0)
		return true;

	return false;
}

