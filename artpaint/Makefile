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
APP_MIME_SIG= 

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
SRCS= Utilities/BitmapUtilities.cpp application/FilePanels.cpp application/FloaterManager.cpp \
application/HSPolygon.cpp application/IntelligentPathFinder.cpp application/MatrixView.cpp \
application/MessageFilters.cpp application/PaintApplication.cpp application/ProjectFileFunctions.cpp \
application/RandomNumberGenerator.cpp application/RefFilters.cpp application/ResourceServer.cpp \
application/Selection.cpp application/SettingsServer.cpp application/StringServer.cpp \
application/UndoAction.cpp application/UndoEvent.cpp application/UndoQueue.cpp \
application/UndoWindow.cpp application/UtilityClasses.cpp controls/CMYControl.cpp controls/ColorPalette.cpp \
controls/ColorView.cpp controls/HSPictureButton.cpp controls/HSVControl.cpp controls/NumberControl.cpp \
controls/NumberSliderControl.cpp controls/PopUpList.cpp controls/PopUpSlider.cpp controls/RGBControl.cpp \
controls/VisualColorControl.cpp controls/YIQControl.cpp controls/YUVControl.cpp layers/Layer.cpp \
layers/LayerView.cpp layers/LayerWindow.cpp paintwindow/BackgroundView.cpp paintwindow/Image.cpp \
paintwindow/ImageUpdater.cpp paintwindow/ImageView.cpp paintwindow/MagnificationView.cpp \
paintwindow/PaintWindow.cpp paintwindow/PaintWindowMenuItem.cpp paintwindow/StatusView.cpp \
tools/AirBrushTool.cpp tools/BitmapDrawer.cpp tools/BlurTool.cpp tools/Brush.cpp tools/BrushEditor.cpp \
tools/BrushTool.cpp tools/ColorSelectorTool.cpp tools/CoordinateQueue.cpp tools/CoordinateReader.cpp \
tools/DrawingTool.cpp tools/EllipseTool.cpp tools/EraserTool.cpp tools/FillTool.cpp tools/FreeLineTool.cpp \
tools/HairyBrushTool.cpp tools/RectangleTool.cpp tools/SelectorTool.cpp tools/StraightLineTool.cpp \
tools/TextTool.cpp tools/ToolButton.cpp tools/ToolEventAdapter.cpp tools/ToolManager.cpp \
tools/ToolScript.cpp tools/ToolSelectionWindow.cpp tools/ToolSetupWindow.cpp tools/TransparencyTool.cpp \
viewmanipulators/CropManipulator.cpp viewmanipulators/FlipManipulator.cpp \
viewmanipulators/FreeTransformManipulator.cpp viewmanipulators/Manipulator.cpp \
viewmanipulators/ManipulatorInformer.cpp viewmanipulators/ManipulatorServer.cpp \
viewmanipulators/Rotate90Manipulator.cpp viewmanipulators/RotationManipulator.cpp \
viewmanipulators/ScaleManipulator.cpp viewmanipulators/TextManipulator.cpp \
viewmanipulators/TranslationManipulator.cpp viewmanipulators/TransparencyManipulator.cpp \
viewmanipulators/WindowGUIManipulator.cpp windows/AboutWindow.cpp \
windows/BrushStoreWindow.cpp windows/DatatypeSetupWindow.cpp windows/GlobalSetupWindow.cpp \
windows/ManipulatorWindow.cpp windows/ViewSetupWindow.cpp

#	specify the resource definition files to use
#	full path or a relative path to the resource file can be used.
RDEFS= ArtPaint.rdef
	
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
LIBS= be tracker translation $(STDCPPLIBS)

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
SYSTEM_INCLUDE_PATHS = 

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
LOCALES=

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
