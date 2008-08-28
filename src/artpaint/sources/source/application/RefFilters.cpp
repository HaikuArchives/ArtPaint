/* 

	Filename:	RefFilters.cpp
	Contents:	Definitions for various ref-filters	
	Author:		Heikki Suhonen
	
*/


#include <Entry.h>
#include <NodeInfo.h>
#include <stdio.h>
#include <string.h>
#include <Path.h>
#include <Roster.h>

#include "RefFilters.h"
#include <sys/stat.h>

bool ImageFilter::Filter(const entry_ref *ref, BNode *node, struct stat *st, const char*)
{
	BNodeInfo *node_info = new BNodeInfo();
	node_info->SetTo(node);
	
	char *mime_type;
	mime_type = new char[B_MIME_TYPE_LENGTH];
	if (!(S_ISDIR(st->st_mode)) && (node_info->GetType(mime_type) != B_NO_ERROR)) {
//		// Setting the mime type
//		BEntry entry(ref,true);
//		BPath path;
//		entry.GetPath(&path);
//		char **argv;
//		argv = new char*[2];
//		argv[0] = new char[B_FILE_NAME_LENGTH];
//		argv[1] = new char[B_FILE_NAME_LENGTH];
//		
//		strcpy(argv[0],"mimeset");
//		strcpy(argv[1],path.Path());
//		be_roster->Launch("application/x-Be.vnd.mimeset",2,argv);
//		node_info->GetType(mime_type);		
//		printf("identified the file: %s\n",path.Path());
//		printf("MIME TYPE: %s\n",mime_type);
//		delete[] argv;
	}
	
	if ((S_ISDIR(st->st_mode)) || (strncmp("image",mime_type,5) == 0)) {
		delete[] mime_type;
		return TRUE;
	}
	else {
		delete[] mime_type;
		return FALSE;
	}
}

