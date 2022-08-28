#!/bin/sh -e

# Build script for ArtPaint
#
# Targets (default is 'all'):
#	main	:	Builds only the main app and moves it in the "dist" folder
#	addons	:	Builds only all addons and copies them in the "dist" folder
#	all		:	Builds main app and addons and puts them and the docs in the "dist" folder
#
# Optional actions:
#	clean	:	Removes all objects (main app and addons)
#	catkeys	:	Generates en.catkeys (main app and addons)
#	debug	:	Builds in debug mode

builder() {
	case "$target" in
		all)
			if [ "$action" != "catkeys" ] ; then
				echo "Moving Documentation into dist folder"
				mkdir -p dist/add-ons
				cp -arf documentation dist/Documentation
			fi
			;;&

		addons | all)
			echo "Building addons"
			for addonDir in $(find ./addons/AddOns/* -maxdepth 0 -type d) ; do
				pushd "$addonDir" > /dev/null
				make OBJ_DIR="objects_addons$debug_suffix" $debug -j
				if [ "$action" = "catkeys" ] ; then
					make -f Makefile OBJ_DIR="objects_addons$debug_suffix" catkeys
				fi
				make OBJ_DIR="objects_addons$debug_suffix" bindcatalogs
				popd > /dev/null
			done

			echo "Moving addons into dist folder"
			mkdir -p dist/add-ons
			cp -rf addons/AddOns/*/objects_addons$debug_suffix/*.so dist/add-ons
			;;&

		main | all)
			echo "Building ArtPaint"
			make -f Makefile OBJ_DIR="objects_artpaint$debug_suffix" $debug -j
			if [ "$action" = "catkeys" ] ; then
				make -f Makefile OBJ_DIR="objects_artpaint$debug_suffix" catkeys
			fi
			make -f Makefile OBJ_DIR="objects_artpaint$debug_suffix" bindcatalogs

			echo "Moving final executable into dist folder"
			mkdir -p dist
			mv -f objects_artpaint$debug_suffix/ArtPaint dist/ArtPaint$debug_suffix
			;;
	esac
}

cleaner() {
	case "$target" in
		addons | all)
			echo "Cleaning addon objects"
			# In case someone reverted back to when there was an addons/Working folder
			if [ -d ./addons/AddOns/Working ] ; then
				rm -rf ./addons/AddOns/Working
			fi

			for addonDir in $(find ./addons/AddOns/* -maxdepth 0 -type d) ; do
				pushd "$addonDir" > /dev/null
				rm -rf objects_addons*
				popd > /dev/null
			done
			;;&

		main | all)
			echo "Cleaning ArtPaint objects"
			rm -rf objects_artpaint*
			;;
	esac
}

target="all"
action=
debug=
debug_suffix=

# Get optional 'action'
options="clean debug catkeys help h -h --h --help"
for item in $options ; do
	for opt in "$@" ; do
		if [ "$opt" = "$item" ] ; then
			action="$opt"
			break
		fi
	done
done

# Get optional 'target'
profiles="main addons all"
for item in $profiles ; do
	for para in "$@" ; do
		if [ "$para" = "$item" ] ; then
			target="$para"
			break
		fi
	done
done

case "$action" in
	help | h | -h | --h | --help)
		echo -e "A script to build ArtPaint"
		echo -e "Usage:\tbuild.sh (action) [clean|debug|catkeys] target [all|main|addons]"
		echo
		echo -e "Targets (default is 'all'):"
		echo -e "\tmain\tBuilds only the main app and moves it in the "dist" folder"
		echo -e "\taddons\tBuilds only all addons and copies them in the "dist" folder"
		echo -e "\tall\tBuilds main app and addons and puts them and the docs in the "dist" folder"
		echo
		echo -e "Optional actions:"
		echo -e "\tclean\tRemoves all objects (main app and addons)"
		echo -e "\tcatkeys\tGenerates en.catkeys (main app and addons)"
		echo -e "\tdebug\tBuilds in debug mode"
		exit 1
		;;

	clean)
		cleaner
		exit 0
		;;

	debug)
		debug="-e DEBUGGER=TRUE"
		debug_suffix="_debug"
		echo "DEBUG Build!"
		;;&

	*)
		builder
		echo "Build has finished: dist/ArtPaint$debug_suffix"
		;;
esac
