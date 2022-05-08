#!/bin/sh -e
echo "Building ArtPaint"
make -f Makefile OBJ_DIR="objects_artpaint" $*
make -f Makefile OBJ_DIR="objects_artpaint" bindcatalogs

echo "Building ArtPaint addons"
for addonDir in $(find ./addons/AddOns/* -maxdepth 0 -type d) ; do
	pushd "$addonDir"
	make OBJ_DIR="objects_addons" $*
	make OBJ_DIR="objects_addons" bindcatalogs
	popd
done

echo "Moving final executable, addons, and Documentation into dist folder"
mkdir -p dist/add-ons
mv -f objects_artpaint/ArtPaint dist
cp -rf addons/AddOns/*/objects_addons/*.so dist/add-ons
cp -arf documentation dist/Documentation

echo "Build has finished!"
