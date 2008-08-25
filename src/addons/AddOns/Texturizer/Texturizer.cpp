/* 

	Filename:	Texturizer.cpp
	Contents:	Definitions for txturizer add-on.	
	Author:		Heikki Suhonen
	
*/

#include "AddOnTypes.h"
#include "Texturizer.h"

extern "C" __declspec(dllexport) Manipulator* manipulator_creator(BBitmap*);
extern "C" __declspec(dllexport) char name[255] = "Texturizer…";
extern "C" __declspec(dllexport) char menu_help_string[255] = "Starts testing Perlin noise functions.";
extern "C" __declspec(dllexport) int32 add_on_api_version = ADD_ON_API_VERSION;
extern "C" __declspec(dllexport) add_on_types add_on_type = COLOR_ADD_ON;


Manipulator* manipulator_creator(BBitmap *bm)
{
	return new TexturizerManipulator(bm);	
}



TexturizerManipulator::TexturizerManipulator(BBitmap *bm)
		: WindowGUIManipulator()
{
	preview_bitmap = NULL;
	config_view = NULL;
	copy_of_the_preview_bitmap = NULL;
	
	
	SetPreviewBitmap(bm);
}


TexturizerManipulator::~TexturizerManipulator()
{
	delete copy_of_the_preview_bitmap;
	delete config_view;
}


BBitmap* TexturizerManipulator::ManipulateBitmap(ManipulatorSettings *set,BBitmap *original,Selection *selection,BStatusBar *status_bar)	
{
//	TexturizerManipulatorSettings *new_settings = cast_as(set,TexturizerManipulatorSettings);
//	
//	if (new_settings == NULL)
//		return NULL;
//		
//	if (original == NULL)
//		return NULL;
//		
//	if (original == preview_bitmap) { 
//		if ((*new_settings == previous_settings) && (last_calculated_resolution <= 1))
//			return original;
//			
//		source_bitmap = copy_of_the_preview_bitmap; 
//		target_bitmap = original; 
//	} 
//	else { 
//		source_bitmap = original; 
//		target_bitmap = new BBitmap(original->Bounds(),B_RGB32,FALSE); 
//	} 	
//
//
//	current_resolution = 1;
//	current_selection = selection;
//	current_settings = *new_settings;
//	progress_bar = status_bar;
//	
//	start_threads();
//	
//	return target_bitmap;
	return NULL;
}

int32 TexturizerManipulator::PreviewBitmap(Selection *selection,bool full_quality,BRegion *updated_region)
{
	progress_bar = NULL;
	current_selection = selection;
//	if (settings == previous_settings ) {
//		if ((last_calculated_resolution != highest_available_quality) && (last_calculated_resolution > 0))
//			last_calculated_resolution = max_c(highest_available_quality,floor(last_calculated_resolution/2.0)); 
//		else
//			last_calculated_resolution = 0;
//	}
//	else
//		last_calculated_resolution = lowest_available_quality;
//
//	if (full_quality) {
//		last_calculated_resolution = min_c(1,last_calculated_resolution);
//	}
//	previous_settings = settings;
//	
//	if (last_calculated_resolution > 0) {		
//		current_resolution = last_calculated_resolution;	
		updated_region->Set(preview_bitmap->Bounds());
	
		target_bitmap = preview_bitmap;
		source_bitmap = copy_of_the_preview_bitmap;
		current_settings = settings;
		
		start_threads();
//	}
		
	return 1;//last_calculated_resolution;
}


void TexturizerManipulator::start_threads()
{
	system_info info;
	get_system_info(&info);
	number_of_threads = info.cpu_count;	
	
	thread_id *threads = new thread_id[number_of_threads];

	for (int32 i=0;i<number_of_threads;i++) {
		threads[i] = spawn_thread(thread_entry,"brightness_thread",B_NORMAL_PRIORITY,this);
		resume_thread(threads[i]);
		send_data(threads[i],i,NULL,0);
	}

	for (int32 i=0;i<number_of_threads;i++) {
		int32 return_value;
		wait_for_thread(threads[i],&return_value);
	}

	delete[] threads;
}

int32 TexturizerManipulator::thread_entry(void *data)
{
	int32 thread_number;
	thread_number = receive_data(NULL,NULL,0);
	
	TexturizerManipulator *this_pointer = (TexturizerManipulator*)data;
	
	return this_pointer->thread_function(thread_number);
}


int32 TexturizerManipulator::thread_function(int32 thread_number)
{	
	int32 step = 1;//current_resolution;
//	uint32 brightness = settings.brightness;
		
	BWindow *progress_bar_window = NULL;
	if (progress_bar != NULL)
		progress_bar_window = progress_bar->Window();


	uint8 *noise_bits = (uint8*)noise_map->Bits();
	uint32 *target_bits = (uint32*)target_bitmap->Bits();
	int32 noise_bpr = noise_map->BytesPerRow();
	int32 target_bpr = target_bitmap->BytesPerRow()/4;
	
	// This union must be used to guarantee endianness compatibility.
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	
	
	// Here handle the whole image.		
	int32 left = target_bitmap->Bounds().left;
	int32 right = target_bitmap->Bounds().right;
	int32 top = target_bitmap->Bounds().top;
	int32 bottom = target_bitmap->Bounds().bottom;

	float height = bottom - top;		
	top = height/number_of_threads*thread_number;
	top = ceil(top/(float)step);
	top *= step;
	bottom = min_c(bottom,top + (height+1)/number_of_threads);
	int32 update_interval = 10;
	float update_amount = 100.0/(bottom-top)*update_interval/(float)number_of_threads;
	float missed_update = 0;
				
	// Loop through all pixels in original.
	uint32 sum;
//	brightness *= 3;		
	for (int32 y=top;y<=bottom;y+=step) {
		int32 y_times_noise_bpr = y*noise_bpr;
		int32 y_times_target_bpr = y*target_bpr;
		for (int32 x=left;x<=right;x+=step) {
			color.word = *(target_bits + x + y_times_target_bpr);
//			color.bytes[0] = *(noise_bits + x + y_times_noise_bpr);
//			color.bytes[1] = *(noise_bits + x + y_times_noise_bpr);
//			color.bytes[2] = *(noise_bits + x + y_times_noise_bpr);
//			color.bytes[3] = 255;
			color.bytes[3] = *(noise_bits + x + y_times_noise_bpr);
			*(target_bits + x + y_times_target_bpr) = color.word;
		}

		// Update the status-bar
		if ( ((y % update_interval) == 0) && (progress_bar_window != NULL) && (progress_bar_window->LockWithTimeout(0) == B_OK) ) {
			progress_bar->Update(update_amount+missed_update);				
			progress_bar_window->Unlock();
			missed_update = 0;
		}
		else if ((y % update_interval) == 0) {
			missed_update += update_amount;
		}
	}	

	return B_OK;
}


void TexturizerManipulator::MouseDown(BPoint point,uint32,BView*,bool first_click)
{
	// This function does nothing in TexturizerManipulator.
}


void TexturizerManipulator::SetPreviewBitmap(BBitmap *bm)
{
	if (preview_bitmap != bm) {
		delete copy_of_the_preview_bitmap;
		if (bm != NULL) {
			preview_bitmap = bm;
			copy_of_the_preview_bitmap = DuplicateBitmap(bm,0);
		}
		else {
			preview_bitmap = NULL;
			copy_of_the_preview_bitmap = NULL;
		}
	}

	if (preview_bitmap != NULL) {
		system_info info;
		get_system_info(&info);
		double speed = info.cpu_count * info.cpu_clock_speed;

		// Let's select a resolution that can handle all the pixels at least
		// 10 times in a second while assuming that one pixel calculation takes
		// about 50 CPU cycles.
		speed = speed / (10*50);	
		BRect bounds = preview_bitmap->Bounds();
		float num_pixels = (bounds.Width()+1) * (bounds.Height() + 1);
//		lowest_available_quality = 1;
//		while ((num_pixels/lowest_available_quality/lowest_available_quality) > speed)
//			lowest_available_quality *= 2;			
//
//		lowest_available_quality = min_c(lowest_available_quality,16);
//		highest_available_quality = max_c(lowest_available_quality/2,1);

		noise_map = MakeNoiseMap(preview_bitmap->Bounds());
	}
//	else {
//		lowest_available_quality = 1;
//		highest_available_quality = 1;
//	}
//	last_calculated_resolution = lowest_available_quality;
}


void TexturizerManipulator::Reset(Selection*)
{
	if (copy_of_the_preview_bitmap != NULL) {
		// memcpy seems to be about 10-15% faster that copying with a loop.
		uint32 *source = (uint32*)copy_of_the_preview_bitmap->Bits();
		uint32 *target = (uint32*)preview_bitmap->Bits();
		uint32 bits_length = preview_bitmap->BitsLength();
		
		memcpy(target,source,bits_length);		
	}
}

BView* TexturizerManipulator::MakeConfigurationView(BMessenger *target)
{
	if (config_view == NULL) {
		config_view = new TexturizerManipulatorView(this,target);
		config_view->ChangeSettings(&settings);
	}
	
	return config_view;
}


ManipulatorSettings* TexturizerManipulator::ReturnSettings()
{
	return new TexturizerManipulatorSettings(settings);
}

void TexturizerManipulator::ChangeSettings(ManipulatorSettings *s)
{
	TexturizerManipulatorSettings *new_settings;
	new_settings = cast_as(s,TexturizerManipulatorSettings);
	if (new_settings != NULL) {
		settings = *new_settings;
	}
}

char* TexturizerManipulator::ReturnName()
{
	return "Texturizer";
}

char* TexturizerManipulator::ReturnHelpString()
{
	return "Testing the perlin noise function.";
}



BBitmap* TexturizerManipulator::MakeNoiseMap(BRect rect)
{
	BStopWatch noise_map("MakeNoiseMap");
	BBitmap *map = new BBitmap(rect,B_CMAP8);
	
	float width = rect.Width();
	float height = rect.Height();
	
	uint8 *bits = (uint8*)map->Bits();
	uint32 bpr = map->BytesPerRow();
	
	for (float y=0;y<=height;y++) {
		int32 y_times_bpr = y*bpr;
		for (float x=0;x<=width;x++) {
			*(bits + (int32)x + y_times_bpr) = 255 - (1.0 - PerlinNoise_2D(x/width,y/height))*127.5;					
		}
	}

	return map;
}


float TexturizerManipulator::CosineInterpolation(float a, float b, float x)
{
	float ft = x * PI;
	float f = (1 - cos(ft)) * 0.5;
	
	return a*(1-f) + b*f;
}


float TexturizerManipulator::Noise(int32 x, int32 y)
{
	register int32 n = x + y*57;
	n = n<<13 ^ n;
	return(1.0-((n*(n*n*15731+789221)+1376312589)&0x7fffffff)/1073741824.0);
}



float TexturizerManipulator::SmoothNoise(float x, float y)
{
	float corners = (Noise(x-1,y-1),Noise(x+1,y-1),Noise(x-1,y+1),Noise(x+1,y+1))/16.0;
	float sides = (Noise(x,y-1),Noise(x-1,y),Noise(x+1,y),Noise(x,y+1))/8.0;
	float center = Noise(x,y)/4;

	return corners + sides + center;	
}



float TexturizerManipulator::InterpolatedNoise(float x, float y)
{
	int32 integer_x = (int32)x;
	int32 integer_y = (int32)y;
	
	float fractional_x = x - integer_x;
	float fractional_y = y - integer_y;
	
	float v1 = Noise(integer_x,integer_y);
	float v2 = Noise(integer_x+1,integer_y);
	float v3 = Noise(integer_x,integer_y+1);
	float v4 = Noise(integer_x+1,integer_y+1);
	
	float i1 = CosineInterpolation(v1,v2,fractional_x);
	float i2 = CosineInterpolation(v3,v4,fractional_x);
	
	return CosineInterpolation(i1,i2,fractional_y);
}


float TexturizerManipulator::PerlinNoise_2D(float x, float y)
{
	float total = 0;
	float p = 1/2.0;
	float n = 7;
	
	float frequency = 1.0;
	float amplitude = 1.0 / p;
	for (int32 i=0;i<=n;i++) {
		frequency *= 2.0;
		amplitude *= p;
		
		total += InterpolatedNoise(x*frequency,y*frequency)*amplitude;
	}

	total = min_c(1.0,max_c(-1.0,total));

	return total;
}

// -------------------------------------
TexturizerManipulatorView::TexturizerManipulatorView(TexturizerManipulator *manip,BMessenger *t)
	: WindowGUIManipulatorView(BRect(0,0,0,0))
{
	target = BMessenger(*t);
	manipulator = manip;
	started_adjusting = FALSE;

//	brightness_slider = new ControlSlider(BRect(0,0,200,0),"brightness_slider","Brightness",new BMessage(BRIGHTNESS_ADJUSTING_FINISHED),0,255,B_TRIANGLE_THUMB);
//	brightness_slider->SetModificationMessage(new BMessage(BRIGHTNESS_ADJUSTED));
//	brightness_slider->SetLimitLabels("Dim","Bright");
//	brightness_slider->ResizeToPreferred();
//	brightness_slider->MoveTo(4,4);
//	AddChild(brightness_slider);
//	
//	ResizeTo(brightness_slider->Bounds().Width()+8,brightness_slider->Bounds().Height()+8);
}


void TexturizerManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
//	brightness_slider->SetTarget(BMessenger(this));
}

void TexturizerManipulatorView::AllAttached()
{
//	brightness_slider->SetValue(settings.brightness);
}


void TexturizerManipulatorView::MessageReceived(BMessage *message)
{
	switch (message->what) {
//		case BRIGHTNESS_ADJUSTED:
//			settings.brightness = brightness_slider->Value();
//			manipulator->ChangeSettings(&settings);
//			if (!started_adjusting) {
//				target.SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
//				started_adjusting = TRUE;
//			}
//			break;			
//
//		case BRIGHTNESS_ADJUSTING_FINISHED:
//			started_adjusting = FALSE;
//			settings.brightness = brightness_slider->Value();
//			manipulator->ChangeSettings(&settings);
//			target.SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
//			break;			
//						
		default:
			WindowGUIManipulatorView::MessageReceived(message);
			break;
	}
}


void TexturizerManipulatorView::ChangeSettings(ManipulatorSettings *set)
{
	TexturizerManipulatorSettings *new_settings = cast_as(set,TexturizerManipulatorSettings);

	if (set != NULL) {
		settings = *new_settings;
				
		BWindow *window = Window();
		if (window != NULL) {
			window->Lock();
//			brightness_slider->SetValue(settings.brightness);
			window->Unlock();
		}
	} 
}