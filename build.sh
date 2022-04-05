#!/bin/sh -e
echo "Building ArtPaint"
make -f Makefile OBJ_DIR="objects_artpaint" $*
make -f Makefile OBJ_DIR="objects_artpaint" bindcatalogs
echo "Building ArtPaint addons"
make -f Makefile_addons OBJ_DIR="objects_addons" $*
echo "Moving final executable, addons, and Documentation into dist folder"
mkdir -p dist/add-ons
cp -af objects_artpaint/ArtPaint dist
cp -rf addons/AddOns/Working/*/objects_addons/*.so dist/add-ons
cp -arf Documentation dist/Documentation
echo "Build has finished!"
