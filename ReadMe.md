ArtPaint
======================
ArtPaint is an award winning painting and image-processing program originally for the BeOS.      
It now runs on Haiku (available through HaikuDepot):

![ArtPaintScreenShot](/artwork/ArtPaintScreenShot.png)

See the [documentation](http://htmlpreview.github.io/?https://github.com/HaikuArchives/ArtPaint/blob/master/documentation/index.html) for more information.

### How to build

Easy. Simply execute the script `build.sh` and you'll find ArtPaint, its addons and documentation in the folder `dist`.

`build.sh` also takes optional parameters:

```
$> build.sh help
A script to build ArtPaint
Usage:  build.sh (action) [clean|debug|catkeys] target [all|main|addons]

Targets (default is 'all'):
        main    Builds only the main app and moves it in the dist folder
        addons  Builds only all addons and copies them in the dist folder
        all     Builds main app and addons and puts them and the docs in the dist folder

Optional actions:
        clean   Removes all objects (main app and addons)
        catkeys Generates en.catkeys (main app and addons)
        debug   Builds in debug mode
```
