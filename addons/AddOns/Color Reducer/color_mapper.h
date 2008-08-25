/*
	Filename:	color_mapper.h
	Author:		Heikki Suhonen (Heikki.Suhonen@Helsinki.FI)
*/


BBitmap* nearest_color_mapper(BBitmap * inSource, const rgb_color * inPalette, int inPaletteSize);
BBitmap* floyd_steinberg_edd_color_mapper(BBitmap * inSource, const rgb_color * inPalette, int inPaletteSize);
BBitmap* preserve_solids_fs_color_mapper(BBitmap * inSource, const rgb_color * inPalette, int inPaletteSize);
BBitmap* n_candidate_color_mapper(BBitmap * inSource, const rgb_color * inPalette, int inPaletteSize, int maxCandidates = 0);




/*
	This function can be called at most 32768 times for an image.
	For this reason it should be as fast as possible. Using some other
	distance metric instead of the euclidian rgb-distance might very well
	be useful. For example the sum of r,g and b component absolute 
	differences could be used thus removing the sqrts.
*/

inline int32 find_palette_index(uint32 bgra_word,const rgb_color * inPalette, int inPaletteSize)
{
	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;
	
	bgra32.word = bgra_word;
	
	float min_distance = 1000000.0;
	uint8 red = bgra32.bytes[2];
	uint8 green = bgra32.bytes[1];
	uint8 blue = bgra32.bytes[0];
	
	int32 selected_index = -1;

	for (int i=0;i<inPaletteSize;i++) {
		float distance = sqrt(	pow(inPalette[i].red-red,2) +
								pow(inPalette[i].green-green,2) +
								pow(inPalette[i].blue-blue,2));
		if (distance < min_distance) {
			selected_index = i;
			min_distance = distance;
		}																	
	}

	return selected_index;
}



inline int32 find_candidate(uint32 bgra_word,const rgb_color * inPalette, int inPaletteSize, int candidateNumber, float * outDistance)
{
	struct Candidate {
		int32 index;
		float distance;
	};

	Candidate *candidates = new Candidate[candidateNumber];

	for (int32 i=0;i<candidateNumber;i++) {
		candidates[i].index = 0;
		candidates[i].distance = 1000000.0;
	}

	union {
		uint8 bytes[4];
		uint32 word;
	} bgra32;
	
	bgra32.word = bgra_word;
	
	float min_distance = 1000000.0;
	uint8 red = bgra32.bytes[2];
	uint8 green = bgra32.bytes[1];
	uint8 blue = bgra32.bytes[0];
	
	int32 selected_index = -1;

	if (candidateNumber > inPaletteSize) {
		*outDistance = min_distance; 
		delete[] candidates;
		return 0;
	}

	for (int i=0;i<inPaletteSize;i++) {
		float distance = sqrt(	pow(inPalette[i].red-red,2) +
								pow(inPalette[i].green-green,2) +
								pow(inPalette[i].blue-blue,2));
		int k=0;
		while ((k<candidateNumber) && (candidates[k].distance < distance)) {
			k++;
		}
		for (int j=candidateNumber-1;j>k;j--) {
			candidates[j] = candidates[j-1];
		}
		if (k<candidateNumber) {
			candidates[k].distance = distance;
			candidates[k].index = i; 
		}
	}

	selected_index = candidates[candidateNumber-1].index;
	*outDistance = candidates[candidateNumber-1].distance;

	delete[] candidates;
	return selected_index;	
}





