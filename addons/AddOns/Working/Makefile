SUBDIRS := AdaptiveHistogramEqualization AntiDitherer BlurAddOn Brightness \
ColorBalanceAddOn ColorReducer ColorSeparator Contrast ContrastManipulatorAddOn \
DispersionAddOn EdgeDetector EmbossAddOn EnhanceEdges GaussianBlur \
GrayscaleAddOn Halftone Interference MarbleTexturer NegativeAddOn \
OilAddOn PolarMapper Saturation Sharpness Threshold TwirlAddOn \
WaveAddOn WoodRelief

all clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C "$$d" $(MAKECMDGOALS) || exit $?; \
	done
