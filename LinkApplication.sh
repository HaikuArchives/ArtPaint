#!/bin/sh

ARTPAINT_DIR=artpaint
ADDON_DIR=./addons

if [ ! -d $ADDON_DIR ]
then
	mkdir -p $ADDON_DIR
fi

if [ -L $ADDON_DIR/_APP_ ]
then
	echo "The link already exists!"
	exit 0
fi

if [ ! -f $ARTPAINT_DIR"/obj.X86/ArtPaint" ]
then
	echo "The ArtPaint executable does not seem to exist!"
	echo $ARTPAINT_DIR"/obj.X86/ArtPaint"
	exit 1
fi

cd $ADDON_DIR
ln -s ../$ARTPAINT_DIR/obj.X86/ArtPaint _APP_

