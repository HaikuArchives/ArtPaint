/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef COLOR_BALANCE_H
#define COLOR_BALANCE_H

#include "ManipulatorSettings.h"
#include "WindowGUIManipulator.h"


#define	BLUR_AMOUNT_CHANGED			'Bamc'
#define	BLUR_AMOUNT_CHANGE_STARTED	'Bacs'
#define	BLUR_TRANSPARENCY_CHANGED	'Btpc'
#define	MAX_BLUR_AMOUNT		30

enum {
	VERTICAL_THREAD,
	HORIZONTAL_THREAD
};

class BlurManipulatorSettings : public ManipulatorSettings {
public:
		BlurManipulatorSettings()
			: ManipulatorSettings() {
			blur_amount = 1;
			blur_alpha = FALSE;
		}

		BlurManipulatorSettings(BlurManipulatorSettings *s)
			: ManipulatorSettings() {
			blur_amount = s->blur_amount;
			blur_alpha = s->blur_alpha;
		}

		BlurManipulatorSettings& operator=(const BlurManipulatorSettings& s) {
			blur_amount = s.blur_amount;
			blur_alpha = s.blur_alpha;
			return *this;
		}


		bool operator==(BlurManipulatorSettings s) {
			return ((blur_amount == s.blur_amount) &&
					(blur_alpha == s.blur_alpha));
		}

int32	blur_amount;
bool	blur_alpha;
};

class BlurManipulatorView;


class BlurManipulator : public WindowGUIManipulator {
BBitmap				*preview_bitmap;
BBitmap				*wide_copy_of_the_preview_bitmap;
BBitmap				*tall_copy_of_the_preview_bitmap;

//BBitmap				*half_preview_bitmap;
//BBitmap				*quarter_preview_bitmap;
//BBitmap				*eight_preview_bitmap;
//


// These variables will be used by the threads that calculate the Blur
// The threads take a local copy of these when they start.
int32		thread_count;
uint32		*tall_bits;
uint32		*wide_bits;
uint32		*final_bits;
int32		tall_bpr;
int32		wide_bpr;
int32		final_bpr;
int32		final_width;
int32		final_height;
int32 		blur_amount;

// The selection will also be used by the threads. It will not however be copied.
Selection	*selection;
BStatusBar	*status_bar;


// These functions calculate the blur. The function CalculateBlur is
// used to start a few threads for the calclation. It will return after the
// threads have finished their jobs. Before calling it the above variables
// should have been initialized.
			void	CalculateBlur();
static		int32	thread_entry(void*);
			int32	VerticalBlur(int32 thread_number);
			int32	HorizontalBlur(int32 thread_number);



BlurManipulatorSettings	settings;
BlurManipulatorSettings	previous_settings;

BlurManipulatorView		*config_view;
//void		CalculateBlur(BBitmap*,BBitmap*,int32,BlurManipulatorSettings*,Selection*,BStatusBar*);

public:
			BlurManipulator(BBitmap*);
			~BlurManipulator();

BBitmap*	ManipulateBitmap(ManipulatorSettings*,BBitmap*,Selection*,BStatusBar*);
BView*		MakeConfigurationView(const BMessenger& target);
int32		PreviewBitmap(Selection*,bool full_quality=FALSE,BRegion* =NULL);
void		Reset(Selection*);
void		SetPreviewBitmap(BBitmap*);


const char*	ReturnHelpString();
const char*	ReturnName();

ManipulatorSettings*	ReturnSettings();

//void		FinishManipulation(bool);
//void		ChangeValue(int32);
//void		ChangeTransparency(int32);
void		ChangeSettings(ManipulatorSettings*);


status_t	ReadSettings(BNode*);
status_t	WriteSettings(BNode*);
};


class BlurManipulatorView : public WindowGUIManipulatorView {
		BlurManipulator	*manipulator;
		BMessenger		*target;
		BSlider			*blur_amount_slider;
//		BCheckBox		*transparency_checkbox;

		BlurManipulatorSettings	settings;

		bool			preview_started;

public:
		BlurManipulatorView(BRect rect, BlurManipulator *manip,const BMessenger& target);
		~BlurManipulatorView();

void	AttachedToWindow();
void	MessageReceived(BMessage*);

void	ChangeSettings(BlurManipulatorSettings*);
};
#endif



