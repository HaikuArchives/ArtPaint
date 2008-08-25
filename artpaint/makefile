## BeOS Generic Makefile v2.2 ##

# Set this files mime-type to "text/x-makefile", and open it with Pe :-)

## Fill in this file to specify the project being created, and the referenced
## makefile-engine will do all of the hard work for you.  This handles both
## Intel and PowerPC builds of the BeOS.

## Application Specific Settings ---------------------------------------------

# specify the name of the binary
NAME=ArtPaint

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE=APP

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
SRCS= sources/application/ClipBoardManager.cpp \
sources/application/ClipBoardView.cpp \
sources/application/FilePanels.cpp \
sources/application/FloaterManager.cpp \
sources/application/HSPolygon.cpp \
sources/application/IntelligentPathFinder.cpp \
sources/application/MatrixView.cpp \
sources/application/MessageFilters.cpp \
sources/application/PaintApplication.cpp \
sources/application/ProjectFileFunctions.cpp \
sources/application/RandomNumberGenerator.cpp \
sources/application/RefFilters.cpp \
sources/application/Selection.cpp \
sources/application/Settings.cpp \
sources/application/StringServer.cpp \
sources/application/SymbolImageServer.cpp \
sources/application/UndoAction.cpp \
sources/application/UndoEvent.cpp \
sources/application/UndoQueue.cpp \
sources/application/UndoWindow.cpp \
sources/application/UtilityClasses.cpp \
sources/controls/CMYControl.cpp \
sources/controls/ColorPalette.cpp \
sources/controls/ColorView.cpp \
sources/controls/Controls.cpp \
sources/controls/DirectionControl.cpp \
sources/controls/HSPictureButton.cpp \
sources/controls/HSVControl.cpp \
sources/controls/PopUpList.cpp \
sources/controls/PopUpSlider.cpp \
sources/controls/RGBControl.cpp \
sources/controls/VisualColorControl.cpp \
sources/controls/YIQControl.cpp \
sources/controls/YUVControl.cpp \
sources/layers/Layer.cpp \
sources/layers/LayerView.cpp \
sources/layers/LayerWindow.cpp \
sources/paintwindow/BackgroundView.cpp \
sources/paintwindow/Image.cpp \
sources/paintwindow/ImageUpdater.cpp \
sources/paintwindow/ImageView.cpp \
sources/paintwindow/MagnificationView.cpp \
sources/paintwindow/PaintWindow.cpp \
sources/paintwindow/PaintWindowMenuItem.cpp \
sources/paintwindow/StatusView.cpp \
sources/tools/AirBrushTool.cpp \
sources/tools/BitmapDrawer.cpp \
sources/tools/BlurTool.cpp \
sources/tools/Brush.cpp \
sources/tools/BrushEditor.cpp \
sources/tools/BrushTool.cpp \
sources/tools/ColorSelectorTool.cpp \
sources/tools/CoordinateQueue.cpp \
sources/tools/CoordinateReader.cpp \
sources/tools/DrawingTool.cpp \
sources/tools/EllipseTool.cpp \
sources/tools/EraserTool.cpp \
sources/tools/FillTool.cpp \
sources/tools/HairyBrushTool.cpp \
sources/tools/LineTool.cpp \
sources/tools/RectangleTool.cpp \
sources/tools/SelectorTool.cpp \
sources/tools/SimpleTool.cpp \
sources/tools/StraightLineTool.cpp \
sources/tools/TextTool.cpp \
sources/tools/ToolButton.cpp \
sources/tools/ToolEventAdapter.cpp \
sources/tools/ToolImages.cpp \
sources/tools/ToolManager.cpp \
sources/tools/ToolScript.cpp \
sources/tools/ToolSelectionWindow.cpp \
sources/tools/ToolSetupWindow.cpp \
sources/tools/TransparencyTool.cpp \
sources/viewmanipulators/CropManipulator.cpp \
sources/viewmanipulators/FlipManipulator.cpp \
sources/viewmanipulators/FreeTransformManipulator.cpp \
sources/viewmanipulators/Manipulator.cpp \
sources/viewmanipulators/ManipulatorInformer.cpp \
sources/viewmanipulators/ManipulatorServer.cpp \
sources/viewmanipulators/Rotate90Manipulator.cpp \
sources/viewmanipulators/RotationManipulator.cpp \
sources/viewmanipulators/ScaleManipulator.cpp \
sources/viewmanipulators/TextManipulator.cpp \
sources/viewmanipulators/TranslationManipulator.cpp \
sources/viewmanipulators/TransparencyManipulator.cpp \
sources/viewmanipulators/WindowGUIManipulator.cpp \
sources/windows/AboutWindow.cpp \
sources/windows/BrushStoreWindow.cpp \
sources/windows/DatatypeSetupWindow.cpp \
sources/windows/GlobalSetupWindow.cpp \
sources/windows/ManipulatorWindow.cpp \
sources/windows/PopUpColorWindow.cpp \
sources/windows/ViewSetupWindow.cpp \
Utilities/BitmapUtilities.cpp


#	specify the resource files to use
#	full path or a relative path to the resource file can be used.
RSRCS=ArtPaint.rsrc

# @<-src@ 
#%}

#	end support for Pe and Eddie

#	specify additional libraries to link against
#	there are two acceptable forms of library specifications
#	-	if your library follows the naming pattern of:
#		libXXX.so or libXXX.a you can simply specify XXX
#		library: libbe.so entry: be
#		
#	- 	if your library does not follow the standard library
#		naming scheme you need to specify the path to the library
#		and it's name
#		library: my_lib.a entry: my_lib.a or path/my_lib.a
LIBS= translation tracker be root stdc++.r4

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
LOCAL_INCLUDE_PATHS = sources/application \
sources/controls \
sources/layers \
sources/paintwindow \
sources/tools \
sources/viewmanipulators \
sources/windows \
Utilities

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE=FULL

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
include $(BUILDHOME)/etc/makefile-engine
