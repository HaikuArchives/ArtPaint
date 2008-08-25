/* 

	Filename:	Marble.cpp
	Contents:	Definitions for a manipulator that encanhes edges in the image.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "Marble.h"
#include <new.h>
#include "PixelOperations.h"
#include "PerlinNoiseGenerator.h"

extern "C" __declspec(dllexport) Manipulator* manipulator_creator(BBitmap*);
extern "C" __declspec(dllexport) char name[255] = "Marble Texture";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Replaces the contents of the active layer with a marble texture.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = EFFECT_FILTER_ADD_ON;


Manipulator* manipulator_creator(BBitmap*)
{
	return new MarbleManipulator();	
}



MarbleManipulator::MarbleManipulator()
		: Manipulator()
{
}


MarbleManipulator::~MarbleManipulator()
{
}


BBitmap* MarbleManipulator::ManipulateBitmap(BBitmap *original,Selection *selection,BStatusBar *status_bar)
{
	BStopWatch watch("PerlinMarble");
	
	source_bitmap = original;
	target_bitmap = original;
	the_selection = selection;
	progress_bar = status_bar;
	
	system_info info;
	get_system_info(&info);
	
	thread_id *threads = new thread_id[info.cpu_count];

	for (int32 i=0;i<info.cpu_count;i++) {
		threads[i] = spawn_thread(thread_entry,"perlin_fade_thread",B_NORMAL_PRIORITY,this);	
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}
	
	for (int32 i=0;i<info.cpu_count;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;	
	
	// Return the original.
	return original;
}

char* MarbleManipulator::ReturnName()
{
	return "Marble Texture";
}



int32 MarbleManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);
	
	MarbleManipulator *this_pointer = (MarbleManipulator*)data;

	return this_pointer->thread_function(thread_number);	
}


int32 MarbleManipulator::thread_function(int32 thread_number)
{
	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();

	register uint32 *source = (uint32*)source_bitmap->Bits();
	int32 source_bpr = source_bitmap->BytesPerRow()/4;

	PerlinNoiseGenerator generator(0.85,8,2.0);
	
	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color1,color2;
		
	if (the_selection->IsEmpty()) {
		// Here handle the whole image.		
		float left = target_bitmap->Bounds().left;
		float right = target_bitmap->Bounds().right;
		float top = target_bitmap->Bounds().top;
		float bottom = target_bitmap->Bounds().bottom;

		system_info info;
		get_system_info(&info);
		
		int32 height = (bottom - top+1)/info.cpu_count;		
		top = min_c(bottom,top+thread_number*height);
		bottom = min_c(bottom,top + height-1);

		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)info.cpu_count;
		float missed_update = 0;
						
		// Loop through all pixels in original.
//		float one_per_width = 1.0/(right-left);
//		float one_per_height = 1.0/(target_bitmap->Bounds().Height());
		float one_per_width = 1.0/256;
		float one_per_height = 1.0/256;
		source += (int32)top*source_bpr;

		color1.bytes[0] = 30;
		color1.bytes[1] = 30;
		color1.bytes[2] = 255;
		color1.bytes[3] = 255;
	
		color2.bytes[0] = 220;
		color2.bytes[1] = 220;
		color2.bytes[2] = 255;
		color2.bytes[3] = 255;
	

		for (float y=top;y<=bottom;++y) {
			int32 y_times_bpr = y*source_bpr;
			for (float x=left;x<=right;++x) {
				float noise = generator.PerlinNoise2D(x*one_per_width,y*one_per_height);
//				noise = (1+noise)*.5;
				
//				*source++ = mix_2_pixels(color1.word,color2.word,noise);				
//				*source++ = marble_color(noise);
				*source++ = marble_color(sin(x*.1+noise)+sin(y*0.1+noise));
			}

			// Update the status-bar
			if ( (((int32)y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount+missed_update);				
				progress_bar_window->Unlock();
				missed_update = 0;
			}
			else if (((int32)y % update_interval) == 0) {
				missed_update += update_amount;
			}
		}

	}
	else {
		// Here handle only those pixels for which selection->ContainsPoint(x,y) is true.
		BRect rect = the_selection->GetBoundingRect();

		int32 left = rect.left;
		int32 right = rect.right;
		int32 top = rect.top;
		int32 bottom = rect.bottom;

		system_info info;
		get_system_info(&info);
		
		float height = bottom - top;		
		top += height/info.cpu_count*thread_number;
		bottom = min_c(bottom,top + (height+1)/info.cpu_count);
		
		int32 update_interval = 10;
		float update_amount = 100.0/(bottom-top)*update_interval/(float)info.cpu_count;
		
		// Loop through all pixels in original.
		for (int32 y=top;y<=bottom;++y) {
			for (int32 x=left;x<=right;++x) {
				if (the_selection->ContainsPoint(x,y)) {
				}
			}

			// Update the status-bar
			if ( ((y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
				progress_bar->Update(update_amount);				
				progress_bar_window->Unlock();
			}
		}
	}

	return B_OK;
}


uint32 MarbleManipulator::marble_color(float c)
{
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
			
	color.bytes[3] = 255;
	color.bytes[0] = 0;
	color.bytes[1] = 0;
	color.bytes[2] = 0;
	
	if (c < 0.8) {
		color.bytes[0] = (1-c*.125)*255;
		color.bytes[1] = (1-c*.125)*50;
	}
	else if (c > 0.8) {
		color.bytes[2] = c*255;
	}		
	return color.word;
}