/* 

	Filename:	Render.cpp
	Contents:	Definitions for RenderManipulator-class.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "Render.h"
#include "LightSource.h"
#include "Surface.h"
#include "PerlinNoiseGenerator.h"

extern "C" __declspec(dllexport) Manipulator* manipulator_creator(BBitmap*);
extern "C" __declspec(dllexport) char name[255] = "RenderTest…";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Tests the renderer thing";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = GENERIC_ADD_ON;

extern uint8 *render(float **,uint8 *, Surface *,LightSource *,int32,int32);


Manipulator* manipulator_creator(BBitmap *bm)
{
	return new RenderManipulator(bm);	
}



RenderManipulator::RenderManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	SetPreviewBitmap(bm);
}


RenderManipulator::~RenderManipulator()
{
}


BBitmap* RenderManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)	
{
	int32 width = original->Bounds().Width();
	int32 height = original->Bounds().Height();
	uint32 *bits = (uint32*)original->Bits();
	
	float **hm = new float*[height+2];
	for (int32 i=0;i<height+2;i++)
		hm[i] = new float[width+2];
		
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	
	PerlinNoiseGenerator generator(.7,7);
	float one_per_256 = 1/256.0;

// Pyramids and Landscape
	for (int32 y=0;y<height+2;y++) {
		for (int32 x=0;x<width+2;x++) {
//			hm[y][x] = min_c(min_c(y%100,99-y%100),min_c(x%100,99-x%100));				
//			if (((x%100 < 50) && (y % 100 < 50)) || ((x%100>50)&&(y%100>50))) {
//				hm[y][x] = 180;
//			}
//			else
//				hm[y][x] = 0;
//			hm[y][x] = 128 + sin(x*.05)*127;
			float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256);
			if (noise > -0.5)
				hm[y][x] = 128 + noise*40;
			else
				hm[y][x] = 128 - 0.5*40;
		}
	}	
	color.bytes[0] = 0;
	color.bytes[1] = 64;
	color.bytes[2] = 125;
	color.bytes[3] = 255;
	for (int32 y=0;y<height+1;y++) {
		for (int32 x=0;x<width+1;x++) {
			float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256);
			if (noise > 0.6)
				*bits++ = 0xFFFFFFFF;
			else if (noise > -0.5) {
				color.bytes[0] = 0;
				color.bytes[1] = (0.4-noise)*120 + 64; 
				color.bytes[2] = 125;
				color.bytes[3] = 255;
				*bits++ = color.word;				
			}
			else {
				color.bytes[0] = 255;
				color.bytes[1] = 0;
				color.bytes[2] = 0;
				color.bytes[3] = 255;
				*bits++ = color.word;			
			}
		}
	}

// Somehow this generates some blocks.
//	generator = PerlinNoiseGenerator(0.5,5);
//	for (int32 y=0;y<height+2;y++) {
//		for (int32 x=0;x<width+2;x++) {
//			float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256)*30;
//			hm[y][x] = noise;
////			hm[y][x] = -1;
//		}
//	}
//	
//	
//	for (int32 y=0;y<height+1;y++) {
//		for (int32 x=0;x<width+1;x++) {
//			float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256)*50;
//			color.bytes[0] = 100 + noise;
//			color.bytes[1] = 100 + noise;
//			color.bytes[2] = 100 + noise;
//			color.bytes[3] = 255;
//			
//			*bits++ = color.word;
//		}
//	}

// Tiles
//	generator = PerlinNoiseGenerator(.7,8);
//	for (int32 y=0;y<height+2;y++) {
//		int32 y_mod_90 = y%90;
//		for (int32 x=0;x<width+2;x++) {
//			if (y_mod_90<45) {
//				// Odd rows
//				float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256);
//				if ((y_mod_90 < 40) && (x%105 < 100)) {
//					// Red and higher
//					hm[y][x] = 30+noise;
//				}
//				else {
//					hm[y][x] = 10+noise;
//				}
//			}
//			else {
//				// even rows
//	
//				float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256);
//				if ((y_mod_90 < 85) && ((x+50)%105 < 100)) {
//					// Red and higher
//					hm[y][x] = 30+noise;
//				}
//				else {
//					hm[y][x] = 10+noise;
//				}
//			}
//		}
//	}
//
//	color.bytes[0] = 0;
//	color.bytes[1] = 0;
//	color.bytes[2] = 255;
//	color.bytes[3] = 255;
//	
//	for (int32 y=0;y<height+1;y++) {
//		int32 y_mod_90 = y%90;
//		for (int32 x=0;x<width+1;x++) {
//			if (y_mod_90<45) {
//				// Odd rows
//				if ((y_mod_90 < 40) && (x%105 < 100)) {
//					// Red and higher
//					*bits++ = color.word;
//				}
//				else {
//					*bits++ = 0xFFFFFFFF;
//				}
//			}
//			else {
//				// even rows
//	
//				float noise = generator.PerlinNoise2D(x*one_per_256,y*one_per_256);
//				if ((y_mod_90 < 85) && ((x+50)%105 < 100)) {
//					// Red and higher
//					*bits++ = color.word;
//				}
//				else {
//					*bits++ = 0xFFFFFFFF;
//				}
//			}
//		}
//	}

//	for (int32 y=0;y<height+1;y++) {
//		for (int32 x=0;x<width+1;x++) {
//			color.word = *bits++;
//			hm[y][x] = (color.bytes[0]*.144 + color.bytes[1]*.587 + color.bytes[2]*.299)*0.01;
//		}
//		hm[y][width+1] = (color.bytes[0]*.144 + color.bytes[1]*.587 + color.bytes[2]*.299)*0.01; 
//	}
//	for (int32 x=0;x<width+2;x++)
//		hm[height+1][x] = hm[height][x];
		

//	for (int32 y=0;y<height+2;y++) {
//		for (int32 x=0;x<width+2;x++) {
//			float dist = sqrt(pow(x-width/2,2)+pow(y-height/2,2));
//			hm[y][x] = sin(dist*0.1)*3;
//		}
//	}

//	bits = (uint32*)original->Bits();
//	float one_per_8 = 1/8.0;
//	for (int32 y=0;y<height+1;y++) {
//		for (int32 x=0;x<width+1;x++) {
//			float noise = generator.PerlinNoise2D(x*one_per_8,y*one_per_256);
//				
//			float coeff = 0.5+(1+noise)*.25;
//			color.bytes[0] = min_c(255,max_c(0,30*coeff));
//			color.bytes[1] = min_c(255,max_c(0,140*coeff));
//			color.bytes[2] = min_c(255,max_c(0,200*coeff));
//			color.bytes[3] = 255;
//			*bits++ = color.word;
//		}
//	}

//	generator = PerlinNoiseGenerator(0.5,8);
//	for (int32 y=0;y<height+2;y++) {
//		for (int32 x=0;x<width+2;x++) {
//			hm[y][x] = (1.0-generator.PerlinNoise2D(x*one_per_256,y*one_per_256))*10.0;
//		}
//	}

	LightSource *L = new LightSource;
 
  					//	x 		y		z	r		g		b 
	L->SetParameters( -300, 1000.0, width/2, 255.0, 255.0, 255.0,width/2, 0.0, height/2,2.0, 1.0);

	Surface *surface = new Surface(hm,width+1,height+1,0.5,32,0.4,0.2);
	render(hm,(uint8*)original->Bits(),surface,L,original->Bounds().Width()+2,original->Bounds().Height()+2);
	
	delete surface;
	for (int32 i=0;i<height+2;i++) {
		delete[] hm[i];
		hm[i] = NULL;
	}		
	delete[] hm;
	delete L;

	return original;	
}

int32 RenderManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	return -1;
}


void RenderManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
}


void RenderManipulator::SetPreviewBitmap(BBitmap *bm)
{
}


void RenderManipulator::Reset(Selection*)
{
}

BView* RenderManipulator::MakeConfigurationView(BMessenger *target)
{
	return new BBox(BRect(0,0,0,0));
}


ManipulatorSettings* RenderManipulator::ReturnSettings()
{
	return NULL;
}

void RenderManipulator::ChangeSettings(ManipulatorSettings *s)
{
}

char* RenderManipulator::ReturnName()
{
	return "RenderTest";
}

char* RenderManipulator::ReturnHelpString()
{
	return "Test the rendering thing.";
}