################################################
# ArtPaint build script
################################################


################################################
ADDONS_DIR := ../addons
CVS_ROOT_ANON=":pserver:anonymous@code.beunited.org:/cvs/artpaint"
CVS_ROOT := \
	$(shell \
		if test "e$$CVSROOT" == "e" ; then \
			echo $(CVS_ROOT_ANON) ; \
		else \
			echo $$CVSROOT ; \
		fi \
	)
CVS_OPTS=-z3 -q -d $(CVS_ROOT)
CVS_CO_LOG := "cvsco.log"
CVS_CO := cvs $(CVS_OPTS) co

CWD := $(shell pwd)

ifeq "$(CWD)" "/"
CWD   := /.
endif

ifneq (, $(wildcard artpaint.make))
# Ran from mozilla directory
ROOTDIR   := $(shell dirname $(CWD))
TOPSRCDIR := $(CWD)
else
# Ran from artpaint/.. directory (?)
ROOTDIR   := $(CWD)
TOPSRCDIR := $(CWD)/artpaint
endif
################################################


# Main Targets
all:: checkout artpaint addons
build:: artpaint addons
checkout:: clean_co_log artpaint_checkout addons_checkout
clean:: artpaint_clean addons_clean


###############################
# artpaint
artpaint:: 
	@make -C $(TOPSRCDIR) default

artpaint_clean::
	@make -C $(TOPSRCDIR) clean

artpaint_checkout:: 
	@echo "Getting latest source for ArtPaint ..." | tee -a $(CVS_CO_LOG)
	@cd $(ROOTDIR) && $(CVS_CO) artpaint | tee -a $(CVS_CO_LOG)


###############################	
# addons
addons:: artpaint
	@make -C $(TOPSRCDIR)/$(ADDONS_DIR) all

addons_clean::
	@make -C $(TOPSRCDIR)/$(ADDONS_DIR) clean

addons_checkout:: 
	@echo "Getting latest source for ArtPaint AddOns ..." | tee -a $(CVS_CO_LOG)
	@cd $(ROOTDIR) && $(CVS_CO) addons | tee -a $(CVS_CO_LOG)


###############################	
clean_co_log::
	@echo "checkout start: "`date` | tee $(CVS_CO_LOG)
