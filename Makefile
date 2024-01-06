## BeOS Generic Makefile v2.5 ##

## Fill in this file to specify the project being created, and the referenced
## makefile-engine will do all of the hard work for you.  This handles both
## Intel and PowerPC builds of the BeOS and Haiku.

## Application Specific Settings ---------------------------------------------

# specify the name of the binary
NAME= ArtPaint

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE= APP

# 	if you plan to use localization features
# 	specify the application MIME siganture
APP_MIME_SIG= 'application/x-vnd.artpaint'

#	add support for new Pe and Eddie features
#	to fill in generic makefile

#%{
# @src->@

#	specify the source files to use
#	full paths or paths relative to the makefile can be included
# 	all files, regardless of directory, will have their object
#	files created in the common object directory.
#	Note that this means this makefile will not work correctly
#	if two source files with the same name (source.c or source.cpp)
#	are included from different directories.  Also note that spaces
#	in folder names do not work well with this makefile.
SRCS= artpaint/Utilities/BitmapUtilities.cpp artpaint/Utilities/ScaleUtilities.cpp \
artpaint/application/FilePanels.cpp artpaint/application/FloaterManager.cpp \
artpaint/application/HSPolygon.cpp artpaint/application/IntelligentPathFinder.cpp artpaint/application/MatrixView.cpp \
artpaint/application/MessageFilters.cpp artpaint/application/PaintApplication.cpp artpaint/application/ProjectFileFunctions.cpp \
artpaint/application/RandomNumberGenerator.cpp artpaint/application/RefFilters.cpp artpaint/application/ResourceServer.cpp \
artpaint/application/Selection.cpp artpaint/application/SettingsServer.cpp \
artpaint/application/UndoAction.cpp artpaint/application/UndoEvent.cpp artpaint/application/UndoQueue.cpp \
artpaint/application/UtilityClasses.cpp artpaint/controls/ColorPalette.cpp \
artpaint/application/CustomGridLayout.cpp \
artpaint/controls/ColorView.cpp artpaint/controls/HSPictureButton.cpp artpaint/controls/NumberControl.cpp \
artpaint/controls/NumberSliderControl.cpp artpaint/controls/PopUpList.cpp artpaint/controls/PopUpSlider.cpp artpaint/controls/RGBControl.cpp \
artpaint/controls/FloatControl.cpp artpaint/controls/FloatSliderControl.cpp \
artpaint/controls/VisualColorControl.cpp artpaint/layers/Layer.cpp \
artpaint/controls/MultichannelColorControl.cpp artpaint/controls/YUVColorControl.cpp \
artpaint/controls/HSVColorControl.cpp artpaint/controls/CMYColorControl.cpp artpaint/controls/RGBColorControl.cpp \
artpaint/controls/LABColorControl.cpp artpaint/controls/ColorSlider.cpp artpaint/controls/ColorFloatSlider.cpp \
artpaint/layers/LayerView.cpp artpaint/layers/LayerWindow.cpp artpaint/paintwindow/BackgroundView.cpp artpaint/paintwindow/Image.cpp \
artpaint/paintwindow/ImageUpdater.cpp artpaint/paintwindow/ImageView.cpp artpaint/paintwindow/MagnificationView.cpp \
artpaint/paintwindow/PaintWindow.cpp artpaint/paintwindow/PaintWindowMenuItem.cpp artpaint/paintwindow/StatusView.cpp \
artpaint/tools/AirBrushTool.cpp artpaint/tools/BitmapDrawer.cpp artpaint/tools/BlurTool.cpp artpaint/tools/Brush.cpp artpaint/tools/BrushEditor.cpp \
artpaint/tools/BrushTool.cpp artpaint/tools/ColorSelectorTool.cpp artpaint/tools/CoordinateQueue.cpp artpaint/tools/CoordinateReader.cpp \
artpaint/tools/DrawingTool.cpp artpaint/tools/EllipseTool.cpp artpaint/tools/EraserTool.cpp artpaint/tools/FillTool.cpp artpaint/tools/FreeLineTool.cpp \
artpaint/tools/HairyBrushTool.cpp artpaint/tools/RectangleTool.cpp artpaint/tools/SelectorTool.cpp artpaint/tools/StraightLineTool.cpp \
artpaint/tools/TextTool.cpp artpaint/tools/ToolButton.cpp artpaint/tools/ToolEventAdapter.cpp artpaint/tools/ToolManager.cpp \
artpaint/tools/ToolScript.cpp artpaint/tools/ToolSelectionWindow.cpp artpaint/tools/ToolSetupWindow.cpp artpaint/tools/TransparencyTool.cpp \
artpaint/viewmanipulators/CropManipulator.cpp artpaint/viewmanipulators/FlipManipulator.cpp \
artpaint/viewmanipulators/FreeTransformManipulator.cpp artpaint/viewmanipulators/Manipulator.cpp \
artpaint/viewmanipulators/ManipulatorInformer.cpp artpaint/viewmanipulators/ManipulatorServer.cpp \
artpaint/viewmanipulators/RotationManipulator.cpp artpaint/viewmanipulators/Rotate90Manipulator.cpp \
artpaint/viewmanipulators/ScaleManipulator.cpp artpaint/viewmanipulators/ScaleCanvasManipulator.cpp \
artpaint/viewmanipulators/TextManipulator.cpp \
artpaint/viewmanipulators/TranslationManipulator.cpp artpaint/viewmanipulators/TransparencyManipulator.cpp \
artpaint/viewmanipulators/WindowGUIManipulator.cpp \
artpaint/windows/BrushStoreWindow.cpp artpaint/windows/DatatypeSetupWindow.cpp artpaint/windows/GlobalSetupWindow.cpp \
artpaint/windows/ManipulatorWindow.cpp artpaint/windows/ViewSetupWindow.cpp

#	specify the resource definition files to use
#	full path or a relative path to the resource file can be used.
RDEFS= artpaint/ArtPaint.rdef

#	specify the resource files to use.
#	full path or a relative path to the resource file can be used.
#	both RDEFS and RSRCS can be defined in the same makefile.
RSRCS=

# @<-src@
#%}

#	end support for Pe and Eddie

#	specify additional libraries to link against
#	there are two acceptable forms of library specifications
#	-	if your library follows the naming pattern of:
#		libXXX.so or libXXX.a you can simply specify XXX
#		library: libbe.so entry: be
#
#	-	for version-independent linking of standard C++ libraries please add
#		$(STDCPPLIBS) instead of raw "stdc++[.r4] [supc++]" library names
#
#	-	for localization support add following libs:
#		locale localestub
#
#	- 	if your library does not follow the standard library
#		naming scheme you need to specify the path to the library
#		and it's name
#		library: my_lib.a entry: my_lib.a or path/my_lib.a
LIBS= be tracker translation localestub z $(STDCPPLIBS)

#	specify additional paths to directories following the standard
#	libXXX.so or libXXX.a naming scheme.  You can specify full paths
#	or paths relative to the makefile.  The paths included may not
#	be recursive, so include all of the paths where libraries can
#	be found.  Directories where source files are found are
#	automatically included.
LIBPATHS=

#	additional paths to look for system headers
#	thes use the form: #include <header>
#	source file directories are NOT auto-included here
SYSTEM_INCLUDE_PATHS = $(shell findpaths -e B_FIND_PATH_HEADERS_DIRECTORY private/interface)

#	additional paths to look for local headers
#	thes use the form: #include "header"
#	source file directories are automatically included
LOCAL_INCLUDE_PATHS =

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE=

# 	specify here the codes for languages you are going
# 	to support in this application. The default "en"
# 	one must be provided too. "make catkeys" will recreate only
# 	locales/en.catkeys file. Use it as template for creating other
# 	languages catkeys. All localization files must be placed
# 	in "locales" sub-directory.
LOCALES= ca de en_AU en_GB en es_419 es fr fur it ja nl sv tr

#	specify any preprocessor symbols to be defined.  The symbols will not
#	have their values set automatically; you must supply the value (if any)
#	to use.  For example, setting DEFINES to "DEBUG=1" will cause the
#	compiler option "-DDEBUG=1" to be used.  Setting DEFINES to "DEBUG"
#	would pass "-DDEBUG" on the compiler's command line.
DEFINES=

#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS =

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS =

#	specify debug settings
#	if TRUE will allow application to be run from a source-level
#	debugger.  Note that this will disable all optimzation.
DEBUGGER =

#	specify additional compiler flags for all files
COMPILER_FLAGS =

#	specify additional linker flags
LINKER_FLAGS =

#	specify the version of this particular item
#	(for example, -app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL")
#	This may also be specified in a resource.
APP_VERSION =

#	(for TYPE == DRIVER only) Specify desired location of driver in the /dev
#	hierarchy. Used by the driverinstall rule. E.g., DRIVER_PATH = video/usb will
#	instruct the driverinstall rule to place a symlink to your driver's binary in
#	~/add-ons/kernel/drivers/dev/video/usb, so that your driver will appear at
#	/dev/video/usb when loaded. Default is "misc".
DRIVER_PATH =

## include the makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
