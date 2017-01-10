SUBDIRS := AdaptiveHistogramEqualization AntiDitherer BlurAddOn ColorBalanceAddOn ColorSeparator DispersionAddOn EdgeDetector EmbossAddOn EnhanceEdges GrayscaleAddOn Halftone MarbleTexturer NegativeAddOn OilAddOn PolarMapper TwirlAddOn WaveAddOn WoodRelief

all clean:
	@for d in $(SUBDIRS); do \
		$(MAKE) -C "$$d" $(MAKECMDGOALS) || exit $?; \
	done
