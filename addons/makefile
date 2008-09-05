ADDON_DIR = AddOns
ADDONS = "Adaptive Histogram Equalization" \
            AntiDitherer \
			BlurAddOn \
			Brightness \
			"Color Reducer" \
			"Color Separator" \
			"ColorBalanceAddOn" \
			Contrast \
			Contrast1AddOn \
			DispersionAddOn \
			"Edge Detector" \
			EmbossAddOn \
			"Enhance Edges" \
			"Gaussian Blur" \
			GrayscaleAddOn \
			Halftone \
			Interference \
			"Marble Texturer" \
			NegativeAddOn \
			OilAddOn \
			PolarMapper \
			Saturation \
			Sharpness \
			Threshold \
			TwirlAddOn \
			WaveAddOn \
			"Wood Relief"  



#	determine wheather running on x86 or ppc
MACHINE=$(shell uname -m)
ifeq ($(MACHINE), BePC)
	CPU = x86
else
	CPU = ppc
endif

#	set the directory where object files and binaries will be created
OBJ_DIR		:= obj.$(CPU)


all default clean:: 
	@if test ! -d ../artpaint ; then \
		echo "Seems like you need to get the artpaint sources" ; \
		exit 1 ; \
	elif test ! -x ../artpaint/ArtPaint ; then \
		echo "Seems like you need to build the ArtPaint executable" ; \
		exit 1 ; \
	else \
		if test ! -d ../artpaint/add-ons ; then \
			mkdir ../artpaint/add-ons ; \
		fi ; \
		rm -f _APP_ ; \
		ln -s ../artpaint/ArtPaint _APP_ ; \
	fi

all:: default

default::
	@for d in $(ADDONS); do \
		echo "Going to make " $$d ; \
		if test -f $(ADDON_DIR)/"$$d"/makefile ; then \
			ln -sf ../../_APP_ $(ADDON_DIR)/"$$d" ; \
			make -C $(ADDON_DIR)/"$$d" $$@ ; \
			mv $(ADDON_DIR)/"$$d"/obj.x86/* ../artpaint/add-ons ; \
			rm ../artpaint/add-ons/*.o ; \
		else \
			echo "Makefile does not exist for " $$d ; \
			exit 1 ; \
		fi ; \
	done

clean:: cleanaddons
	@for d in $(ADDONS); do \
		make -C $(ADDON_DIR)/"$$d" $@; \
	done

cleanaddons: 
	@for f in `ls ../artpaint/add-ons/`; do \
		if test -f ../artpaint/add-ons/$$f ; then \
			rm -f ../artpaint/add-ons/$$f ; \
		fi ; \
	done
