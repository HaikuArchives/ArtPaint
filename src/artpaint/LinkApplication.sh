#!/bin/sh

CURRENT_DIR=$PWD
ADDON_DIR=../addons

if [ ! -d $ADDON_DIR ]
then
	mkdir -p $ADDON_DIR
fi

if [ -L $ADDON_DIR/_APP_ ]
then
#	echo "The link already exists!"
	exit 0
fi

if [ ! -f "./ArtPaint" ]
then
	echo "The ArtPaint executable does not seem to exist!"
	exit 1
fi

cd $ADDON_DIR
ln -s $CURRENT_DIR/ArtPaint _APP_
