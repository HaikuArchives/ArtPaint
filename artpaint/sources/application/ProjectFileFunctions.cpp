/*

	Filename:	ProjectFileFunctions.cpp
	Contents:	Functions for handling the project-files
	Author:		Heikki Suhonen

*/

#include <ByteOrder.h>
#include <File.h>
#include <unistd.h>


#include "ProjectFileFunctions.h"



// This function moves the file to the start of asked section in the
// file. IT returns the length of that section. If the section is not
// found it returns just 0. The file is left at the start of the
// actual section data.
int64 FindProjectFileSection(BFile &file,int32 section_id)
{
	file.Seek(0,SEEK_SET);

	// Check the endianness
	int32 lendian;
	if (file.Read(&lendian,sizeof(int32)) != sizeof(int32))
		return 0;

	// Skip the file type (presume it is correct)
	file.Seek(sizeof(int32),SEEK_CUR);


	// Find the number of sections.
	int32 number_of_sections;
	if (file.Read(&number_of_sections,sizeof(int32)) != sizeof(int32))
		return 0;

	if (lendian == 0x00000000)
		number_of_sections = B_BENDIAN_TO_HOST_INT32(number_of_sections);
	else if (lendian == 0xFFFFFFFF)
		number_of_sections = B_LENDIAN_TO_HOST_INT32(number_of_sections);
	else
		return 0;

	// The loop through sections until they are finished or the correct section
	// is found.
	int32 marker;
	int32 type;
	int64 length = 0;

	while (number_of_sections > 0) {
		// Read the section beginning mark
		if (file.Read(&marker,sizeof(int32)) != sizeof(int32))
			return 0;

		if (lendian == 0x00000000)
			marker = B_BENDIAN_TO_HOST_INT32(marker);
		else
			marker = B_LENDIAN_TO_HOST_INT32(marker);

		if (marker != PROJECT_FILE_SECTION_START)
			return 0;

		// Read the section type
		if (file.Read(&type,sizeof(int32)) != sizeof(int32))
			return 0;

		if (lendian == 0x00000000)
			type = B_BENDIAN_TO_HOST_INT32(type);
		else
			type = B_LENDIAN_TO_HOST_INT32(type);

		// Read the section length
		if (file.Read(&length,sizeof(int64)) != sizeof(int64))
			return 0;

		if (lendian == 0x00000000)
			length = B_BENDIAN_TO_HOST_INT64(length);
		else
			length = B_LENDIAN_TO_HOST_INT64(length);


		// If the type is not right skip this section.
		if (type != section_id) {
			// Skip the section data
			file.Seek(length,SEEK_CUR);

			// Then get the end marker.
			if (file.Read(&marker,sizeof(int32)) != sizeof(int32))
				return 0;

			if (lendian == 0x00000000)
				marker = B_BENDIAN_TO_HOST_INT32(marker);
			else
				marker = B_LENDIAN_TO_HOST_INT32(marker);
			if (marker != PROJECT_FILE_SECTION_END)
				return 0;
		}
		else {
			return length;
		}
	}


	// If we get here we did not find the section.
	return 0;
}
