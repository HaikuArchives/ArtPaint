/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#include <Bitmap.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <Slider.h>
#include <StatusBar.h>
#include <Window.h>

#include "AddOns.h"
#include "Blur.h"
#include "ManipulatorInformer.h"
#include "Selection.h"

#include <new>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AddOns_Blur"


#ifdef __cplusplus
extern "C" {
#endif
	int32 add_on_api_version = ADD_ON_API_VERSION;
	char name[255] = B_TRANSLATE_MARK("Blur" B_UTF8_ELLIPSIS);
	char menu_help_string[255] = B_TRANSLATE_MARK("Adds a blur.");
	add_on_types add_on_type = BLUR_FILTER_ADD_ON;
#ifdef __cplusplus
}
#endif


Manipulator*
instantiate_add_on(BBitmap* bm, ManipulatorInformer* i)
{
	delete i;
	return new BlurManipulator(bm);
}


BlurManipulator::BlurManipulator(BBitmap* bm)
	:
	WindowGUIManipulator(),
	selection(NULL)
{
	preview_bitmap = NULL;
	wide_copy_of_the_preview_bitmap = NULL;
	tall_copy_of_the_preview_bitmap = NULL;

	config_view = NULL;
	settings.blur_amount = 5;

	SetPreviewBitmap(bm);
}


BlurManipulator::~BlurManipulator()
{
	if (tall_copy_of_the_preview_bitmap != NULL) {
		delete tall_copy_of_the_preview_bitmap;
		tall_copy_of_the_preview_bitmap = NULL;
	}
	if (wide_copy_of_the_preview_bitmap != NULL) {
		delete wide_copy_of_the_preview_bitmap;
		wide_copy_of_the_preview_bitmap = NULL;
	}
	if (config_view != NULL) {
		config_view->RemoveSelf();
		delete config_view;
	}
}


BBitmap*
BlurManipulator::ManipulateBitmap(
	ManipulatorSettings* set, BBitmap* original, BStatusBar* status_bar)
{
	BlurManipulatorSettings* new_settings = dynamic_cast<BlurManipulatorSettings*>(set);

	if (new_settings == NULL)
		return NULL;

	if (original == NULL)
		return NULL;

	if (new_settings->blur_amount < 1) {
		return NULL;
	}

	BBitmap* source_bitmap;
	BBitmap* target_bitmap;
	BBitmap* new_bitmap = NULL;

	return original;
}


int32
BlurManipulator::PreviewBitmap(bool full_quality, BRegion* updated_region)
{
	updated_region->Set(preview_bitmap->Bounds());

	if ((settings == previous_settings) == FALSE) {
		// The blur will be done separably. First in the vertical direction and then in horizontal
		// direction.
		thread_count = GetSystemCpuCount();
		tall_bits = (uint32*)tall_copy_of_the_preview_bitmap->Bits();
		wide_bits = (uint32*)wide_copy_of_the_preview_bitmap->Bits();
		final_bits = (uint32*)preview_bitmap->Bits();
		tall_bpr = tall_copy_of_the_preview_bitmap->BytesPerRow() / 4;
		wide_bpr = wide_copy_of_the_preview_bitmap->BytesPerRow() / 4;
		final_bpr = preview_bitmap->BytesPerRow() / 4;
		final_width = preview_bitmap->Bounds().Width();
		final_height = preview_bitmap->Bounds().Height();
		status_bar = NULL;
		blur_amount = settings.blur_amount;
		previous_settings = settings;
		CalculateBlur();
		return 1;
	} else
		return DRAW_NOTHING;
}


void
BlurManipulator::CalculateBlur()
{
	thread_id* threads = new thread_id[thread_count];

	for (int32 i = 0; i < thread_count; i++) {
		threads[i] = spawn_thread(thread_entry, "vertical_blur_thread", B_NORMAL_PRIORITY, this);
		resume_thread(threads[i]);
		send_data(threads[i], VERTICAL_THREAD, NULL, 0);
		send_data(threads[i], i, NULL, 0);
	}
	for (int32 i = 0; i < thread_count; i++) {
		int32 return_value;
		wait_for_thread(threads[i], &return_value);
	}

	for (int32 i = 0; i < thread_count; i++) {
		threads[i] = spawn_thread(thread_entry, "horizontal_blur_thread", B_NORMAL_PRIORITY, this);
		resume_thread(threads[i]);
		send_data(threads[i], HORIZONTAL_THREAD, NULL, 0);
		send_data(threads[i], i, NULL, 0);
	}
	for (int32 i = 0; i < thread_count; i++) {
		int32 return_value;
		wait_for_thread(threads[i], &return_value);
	}

	delete[] threads;
}


int32
BlurManipulator::thread_entry(void* data)
{
	BlurManipulator* this_pointer = (BlurManipulator*)data;
	thread_id sender;
	int32 type = receive_data(&sender, NULL, 0);
	int32 thread_number = receive_data(&sender, NULL, 0);
	if (this_pointer != NULL) {
		if (type == VERTICAL_THREAD)
			this_pointer->VerticalBlur(thread_number);
		else
			this_pointer->HorizontalBlur(thread_number);

		return B_OK;
	}
	return B_ERROR;
}


int32
BlurManipulator::VerticalBlur(int32 thread_number)
{
	// vertical blur will be done columnwise so that we can use the
	// partial sums.
	int32 left = (final_width) / thread_count * thread_number;
	int32 right = min_c(left + (final_width + 1) / thread_count, final_width);
	float status_bar_update_amount = 50.0 / thread_count / (right - left) * 40;
	int32 height = final_height;
	uint32* source_bits = tall_bits;
	int32 source_bpr = tall_bpr;
	uint32* target_bits = wide_bits;
	int32 target_bpr = wide_bpr;

	source_bits += MAX_BLUR_AMOUNT * source_bpr;
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	float red;
	float green;
	float blue;
	float alpha;
	float divider = 1.0 / (2 * blur_amount + 1);
	BWindow* status_bar_window = NULL;
	if (status_bar != NULL)
		status_bar_window = status_bar->Window();

	for (int32 x = left; x <= right; x++) {
		// Calculate the first pixel for this column.
		red = green = blue = alpha = 0;
		for (int32 dy = -blur_amount; dy <= blur_amount; dy++) {
			color.word = *(source_bits + x + dy * source_bpr);
			if (color.bytes[3] != 0) {
				blue += color.bytes[0];
				green += color.bytes[1];
				red += color.bytes[2];
			}
			alpha += color.bytes[3];
		}
		color.bytes[0] = blue * divider;
		color.bytes[1] = green * divider;
		color.bytes[2] = red * divider;
		color.bytes[3] = alpha * divider;
		*(target_bits + x + MAX_BLUR_AMOUNT) = color.word;

		int32 positive_source_y_offset = (blur_amount + 1) * source_bpr;
		int32 negative_source_y_offset = (-blur_amount) * source_bpr;
		int32 target_y_offset = 1 * target_bpr;
		for (int32 y = 1; y <= height; y++) {
			color.word = *(source_bits + x + negative_source_y_offset);
			if (color.bytes[3] != 0) {
				blue -= color.bytes[0];
				green -= color.bytes[1];
				red -= color.bytes[2];
			}
			alpha -= color.bytes[3];

			color.word = *(source_bits + x + positive_source_y_offset);
			if (color.bytes[3] != 0) {
				blue += color.bytes[0];
				green += color.bytes[1];
				red += color.bytes[2];
			}
			alpha += color.bytes[3];

			color.bytes[0] = blue * divider;
			color.bytes[1] = green * divider;
			color.bytes[2] = red * divider;
			color.bytes[3] = alpha * divider;
			*(target_bits + x + MAX_BLUR_AMOUNT + target_y_offset) = color.word;
			positive_source_y_offset += source_bpr;
			negative_source_y_offset += source_bpr;
			target_y_offset += target_bpr;
		}
		if (((x % 40) == 0) && (status_bar_window != NULL)
			&& (status_bar_window->LockWithTimeout(0) == B_OK)) {
			status_bar->Update(status_bar_update_amount);
			status_bar_window->Unlock();
		}
	}
	return B_OK;
}


int32
BlurManipulator::HorizontalBlur(int32 thread_number)
{
	int32 top = (final_height) / thread_count * thread_number;
	int32 bottom = min_c(top + (final_height + 1) / thread_count, final_height);
	float status_bar_update_amount = 50.0 / thread_count / (bottom - top) * 40.0;
	int32 width = final_width;
	uint32* source_bits = wide_bits;
	int32 source_bpr = wide_bpr;
	uint32* target_bits = final_bits;
	int32 target_bpr = final_bpr;
	union {
		uint8 bytes[4];
		uint32 word;
	} color;
	float red;
	float green;
	float blue;
	float alpha;
	float divider = 1.0 / (2 * blur_amount + 1);
	// In this loop we can use the sums that we already have calculated to remove the innermost
	// loop.

	BWindow* status_bar_window = NULL;
	if (status_bar != NULL)
		status_bar_window = status_bar->Window();

	source_bits += top * source_bpr;
	target_bits += top * target_bpr;

	if ((selection == NULL) || (selection->IsEmpty())) {
		for (int32 y = top; y <= bottom; y++) {
			for (int32 dx = 0; dx < MAX_BLUR_AMOUNT; dx++) {
				*(source_bits + dx) = *(source_bits + MAX_BLUR_AMOUNT);
				*(source_bits + source_bpr - dx - 1)
					= *(source_bits + source_bpr - MAX_BLUR_AMOUNT - 1);
			}
			source_bits += MAX_BLUR_AMOUNT;
			// Calculate the first pixel for this row.
			red = green = blue = alpha = 0;
			for (int32 dx = -blur_amount; dx <= blur_amount; dx++) {
				color.word = *(source_bits + dx);
				if (color.bytes[3] != 0) {
					blue += color.bytes[0];
					green += color.bytes[1];
					red += color.bytes[2];
				}
				alpha += color.bytes[3];
			}
			color.bytes[0] = blue * divider;
			color.bytes[1] = green * divider;
			color.bytes[2] = red * divider;
			color.bytes[3] = alpha * divider;
			*target_bits++ = color.word;
			++source_bits;
			for (int32 x = 1; x <= width; x++) {
				color.word = *(source_bits - blur_amount - 1);
				if (color.bytes[3] != 0) {
					blue -= color.bytes[0];
					green -= color.bytes[1];
					red -= color.bytes[2];
				}
				alpha -= color.bytes[3];

				color.word = *(source_bits + blur_amount);
				if (color.bytes[3] != 0) {
					blue += color.bytes[0];
					green += color.bytes[1];
					red += color.bytes[2];
				}
				alpha += color.bytes[3];

				color.bytes[0] = blue * divider;
				color.bytes[1] = green * divider;
				color.bytes[2] = red * divider;
				color.bytes[3] = alpha * divider;
				*target_bits++ = color.word;
				++source_bits;
			}
			source_bits += MAX_BLUR_AMOUNT;
			if (((y % 40) == 0) && (status_bar_window != NULL)
				&& (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(status_bar_update_amount);
				status_bar_window->Unlock();
			}
		}
	} else if (selection->IsEmpty() == false) {
		for (int32 y = top; y <= bottom; y++) {
			for (int32 dx = 0; dx < MAX_BLUR_AMOUNT; dx++) {
				*(source_bits + dx) = *(source_bits + MAX_BLUR_AMOUNT);
				*(source_bits + source_bpr - dx - 1)
					= *(source_bits + source_bpr - MAX_BLUR_AMOUNT - 1);
			}
			source_bits += MAX_BLUR_AMOUNT;
			// Calculate the first pixel for this row.
			red = green = blue = alpha = 0;
			for (int32 dx = -blur_amount; dx <= blur_amount; dx++) {
				color.word = *(source_bits + dx);
				if (color.bytes[3] != 0) {
					blue += color.bytes[0];
					green += color.bytes[1];
					red += color.bytes[2];
				}
				alpha += color.bytes[3];
			}
			if (selection->ContainsPoint(0, y)) {
				color.bytes[0] = blue * divider;
				color.bytes[1] = green * divider;
				color.bytes[2] = red * divider;
				color.bytes[3] = alpha * divider;
				*target_bits++ = color.word;
				++source_bits;
			} else {
				++source_bits;
				++target_bits;
			}
			for (int32 x = 1; x <= width; x++) {
				color.word = *(source_bits - blur_amount - 1);
				if (color.bytes[3] != 0) {
					blue -= color.bytes[0];
					green -= color.bytes[1];
					red -= color.bytes[2];
				}
				alpha -= color.bytes[3];

				color.word = *(source_bits + blur_amount);
				if (color.bytes[3] != 0) {
					blue += color.bytes[0];
					green += color.bytes[1];
					red += color.bytes[2];
				}
				alpha += color.bytes[3];

				if (selection->ContainsPoint(x, y)) {
					color.bytes[0] = blue * divider;
					color.bytes[1] = green * divider;
					color.bytes[2] = red * divider;
					color.bytes[3] = alpha * divider;
					*target_bits++ = color.word;
					++source_bits;
				} else {
					++target_bits;
					++source_bits;
				}
			}
			source_bits += MAX_BLUR_AMOUNT;
			if (((y % 40) == 0) && (status_bar_window != NULL)
				&& (status_bar_window->LockWithTimeout(0) == B_OK)) {
				status_bar->Update(status_bar_update_amount);
				status_bar_window->Unlock();
			}
		}
	}
	return B_OK;
}


void
BlurManipulator::SetPreviewBitmap(BBitmap* bm)
{
	if (bm != preview_bitmap) {
		if (wide_copy_of_the_preview_bitmap != NULL) {
			delete wide_copy_of_the_preview_bitmap;
			wide_copy_of_the_preview_bitmap = NULL;
		}
		if (tall_copy_of_the_preview_bitmap != NULL) {
			delete tall_copy_of_the_preview_bitmap;
			tall_copy_of_the_preview_bitmap = NULL;
		}
		if (bm != NULL) {
			preview_bitmap = bm;
			BRect bounds = preview_bitmap->Bounds();
			bounds.InsetBy(0, -MAX_BLUR_AMOUNT);
			bounds.OffsetTo(0, 0);
			tall_copy_of_the_preview_bitmap = new BBitmap(bounds, B_RGB32);
			if (tall_copy_of_the_preview_bitmap->IsValid() == FALSE)
				throw std::bad_alloc();

			bounds.InsetBy(-MAX_BLUR_AMOUNT, MAX_BLUR_AMOUNT);
			bounds.OffsetTo(0, 0);
			wide_copy_of_the_preview_bitmap = new BBitmap(bounds, B_RGB32);
			if (wide_copy_of_the_preview_bitmap->IsValid() == FALSE) {
				delete tall_copy_of_the_preview_bitmap;
				throw std::bad_alloc();
			}

			// After allocating the bitmaps we should copy the data from preview_bitmap to
			// tall_copy_of_the_preview_bitmap.
			uint32* source_bits = (uint32*)preview_bitmap->Bits();
			uint32* target_bits = (uint32*)tall_copy_of_the_preview_bitmap->Bits();
			int32 source_bpr = preview_bitmap->BytesPerRow() / 4;
			int32 width = preview_bitmap->Bounds().Width();
			int32 height = preview_bitmap->Bounds().Height();
			// First copy the first row MAX_BLUR_AMOUNT times.
			for (int32 y = 0; y < MAX_BLUR_AMOUNT; y++) {
				for (int32 x = 0; x <= width; x++)
					*target_bits++ = *source_bits++;

				source_bits -= source_bpr;
			}
			// Then copy the actual bitmap
			for (int32 y = 0; y <= height; y++) {
				for (int32 x = 0; x <= width; x++)
					*target_bits++ = *source_bits++;
			}
			// Then copy the last row MAX_BLUR_AMOUNT times
			for (int32 y = 0; y < MAX_BLUR_AMOUNT; y++) {
				source_bits -= source_bpr;
				for (int32 x = 0; x <= width; x++)
					*target_bits++ = *source_bits++;
			}
		} else {
			preview_bitmap = NULL;
			tall_copy_of_the_preview_bitmap = NULL;
			wide_copy_of_the_preview_bitmap = NULL;
		}
	}
}


void
BlurManipulator::Reset()
{
	if (preview_bitmap != NULL) {
		uint32* source_bits = (uint32*)tall_copy_of_the_preview_bitmap->Bits();
		uint32* target_bits = (uint32*)preview_bitmap->Bits();
		int32 source_bpr = tall_copy_of_the_preview_bitmap->BytesPerRow() / 4;
		int32 target_bpr = preview_bitmap->BytesPerRow() / 4;

		for (int32 y = 0; y < preview_bitmap->Bounds().Height() + 1; y++) {
			for (int32 x = 0; x < target_bpr; x++) {
				*(target_bits + x + y * target_bpr)
					= *(source_bits + x + (y + MAX_BLUR_AMOUNT) * source_bpr);
			}
		}
	}
}


ManipulatorSettings*
BlurManipulator::ReturnSettings()
{
	return new BlurManipulatorSettings(&settings);
}


const char*
BlurManipulator::ReturnHelpString()
{
	return B_TRANSLATE("Adds a blur.");
}


const char*
BlurManipulator::ReturnName()
{
	return B_TRANSLATE("Blur");
}


void
BlurManipulator::ChangeSettings(ManipulatorSettings* set)
{
	BlurManipulatorSettings* new_settings = dynamic_cast<BlurManipulatorSettings*>(set);

	if (new_settings != NULL)
		settings = *new_settings;
}


BView*
BlurManipulator::MakeConfigurationView(const BMessenger& target)
{
	config_view = new BlurManipulatorView(BRect(0, 0, 0, 0), this, target);
	config_view->ChangeSettings(&settings);
	return config_view;
}


status_t
BlurManipulator::ReadSettings(BNode* node)
{
	if (node != NULL) {
		int32 new_amount;
		if (node->ReadAttr("blur_amount", B_INT32_TYPE, 0, &new_amount, sizeof(int32))
			== sizeof(int32)) {
			settings.blur_amount = new_amount;
			previous_settings.blur_amount = new_amount - 1;
			return B_OK;
		} else
			return B_ERROR;
	} else
		return B_ERROR;
}


status_t
BlurManipulator::WriteSettings(BNode* node)
{
	if (node != NULL) {
		if (node->WriteAttr("blur_amount", B_INT32_TYPE, 0, &settings.blur_amount, sizeof(int32))
			== sizeof(int32))
			return B_OK;
		else
			return B_ERROR;
	} else
		return B_ERROR;
}


BlurManipulatorView::BlurManipulatorView(BRect rect, BlurManipulator* manip, const BMessenger& t)
	:
	WindowGUIManipulatorView()
{
	manipulator = manip;
	target = new BMessenger(t);

	blur_amount_slider = new BSlider("blur_amount_slider", B_TRANSLATE("Blur amount:"),
		new BMessage(BLUR_AMOUNT_CHANGED), 1, MAX_BLUR_AMOUNT, B_HORIZONTAL, B_TRIANGLE_THUMB);
	blur_amount_slider->SetLimitLabels(B_TRANSLATE("Low"), B_TRANSLATE("High"));
	blur_amount_slider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	blur_amount_slider->SetHashMarkCount(11);
	blur_amount_slider->SetModificationMessage(new BMessage(BLUR_AMOUNT_CHANGE_STARTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(blur_amount_slider)
		.SetInsets(B_USE_SMALL_INSETS)
		.End();

	preview_started = FALSE;
}


BlurManipulatorView::~BlurManipulatorView()
{
	delete target;
}


void
BlurManipulatorView::AttachedToWindow()
{
	WindowGUIManipulatorView::AttachedToWindow();
	blur_amount_slider->SetTarget(BMessenger(this));
}


void
BlurManipulatorView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case BLUR_AMOUNT_CHANGE_STARTED:
		{
			if (preview_started == FALSE) {
				preview_started = TRUE;
				target->SendMessage(HS_MANIPULATOR_ADJUSTING_STARTED);
			}
			settings.blur_amount = blur_amount_slider->Value();
			manipulator->ChangeSettings(&settings);
		} break;
		case BLUR_AMOUNT_CHANGED:
		{
			settings.blur_amount = blur_amount_slider->Value();
			manipulator->ChangeSettings(&settings);
			preview_started = FALSE;
			target->SendMessage(HS_MANIPULATOR_ADJUSTING_FINISHED);
		} break;
		default:
			WindowGUIManipulatorView::MessageReceived(message);
	}
}


void
BlurManipulatorView::ChangeSettings(BlurManipulatorSettings* s)
{
	settings = *s;

	BWindow* window = Window();

	if (window != NULL)
		window->Lock();

	blur_amount_slider->SetValue(settings.blur_amount);

	if (window != NULL)
		window->Unlock();
}
