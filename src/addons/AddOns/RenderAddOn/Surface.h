/*

        Filename:       Surface.h
        Contents:       Surface-class declaration and definitions.
        Author:

*/

#ifndef SURFACE_H
#define SURFACE_H

#ifndef int32
typedef long            int32;
typedef unsigned char   uint8;
#endif

class Surface {
                float   **height_map;
                int32   width;
                int32   height;

                float   ambient_reflection;
                float   diffuse;
                float   specularity;
                float   specularity_brightness;

public:
                Surface(float **h_map, int32 w, int32 h,
                                float spec, float spec_brightness, float diff,
                                float ambient) {

                        height_map = h_map;

                        width = w;
                        height = h;

                        specularity = spec;
                        diffuse = diff;
                        ambient_reflection = ambient;
                        specularity_brightness = spec_brightness;
                }
                ~Surface() {};


int32   Width();
int32   Height();

float** ReturnHeightMap() { return height_map; }

float   GetAmbientReflection() { return ambient_reflection; }
float   GetDiffuse() { return diffuse; }
float   GetSpecularity() { return specularity; }
float   GetSpecularityBrightness() { return specularity_brightness; }
};
#endif
