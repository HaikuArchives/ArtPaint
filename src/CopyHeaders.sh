#!/bin/sh

SRC_DIR=./sources
ADDON_DIR=../addons
HEADER_DIR=$ADDON_DIR/headers


if [ ! -d $ADDON_DIR ]
then
	mkdir -p $ADDON_DIR
fi

if [ ! -d $HEADER_DIR ]
then
	mkdir -p $HEADER_DIR
fi

for file in application/AddOns.h \
            application/PixelOperations.h \
            application/Selection.h \
            controls/Controls.h \
            tools/BitmapDrawer.h \
            viewmanipulators/GUIManipulator.h \
            viewmanipulators/Manipulator.h \
            viewmanipulators/ManipulatorInformer.h \
            viewmanipulators/ManipulatorSettings.h \
            viewmanipulators/StatusBarGUIManipulator.h \
            viewmanipulators/WindowGUIManipulator.h \
            application/RandomNumberGenerator.h
do
	if [ ! -f $SRC_DIR/$file ]
	then
		echo $file
		echo Does not exist in
		echo $SRC_DIR
		echo Stopping!
		exit 1
	else
#		echo Copying $file to $HEADER_DIR \.\.\.
		cp $SRC_DIR/$file $HEADER_DIR
	fi
done

# Original code below

#cd /boot/home/projects/ArtPaint/sources/
#cp application/AddOns.h /boot/home/ArtPaintAddOns/headers/
#cp application/PixelOperations.h /boot/home/ArtPaintAddOns/headers/
#cp application/Selection.h /boot/home/ArtPaintAddOns/headers/
#cp controls/Controls.h /boot/home/ArtPaintAddOns/headers/
#cp tools/BitmapDrawer.h /boot/home/ArtPaintAddOns/headers/
#cp viewmanipulators/GUIManipulator.h /boot/home/ArtPaintAddOns/headers/
#cp viewmanipulators/Manipulator.h /boot/home/ArtPaintAddOns/headers/
#cp viewmanipulators/ManipulatorInformer.h /boot/home/ArtPaintAddOns/headers/
#cp viewmanipulators/ManipulatorSettings.h /boot/home/ArtPaintAddOns/headers/
#cp viewmanipulators/StatusBarGUIManipulator.h /boot/home/ArtPaintAddOns/headers/
#cp viewmanipulators/WindowGUIManipulator.h /boot/home/ArtPaintAddOns/headers/
#cp application/RandomNumberGenerator.h /boot/home/ArtPaintAddOns/headers/
