/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef IMAGE_ADAPTER_H
#define IMAGE_ADAPTER_H

/*
	ImageAdapter can be used as a base for manipulators that want to
	deal with instances of Image-class. This class defines one function
	through which the Image can be set.
*/

class Image;

class ImageAdapter {
protected:
		Image	*image;
public:
		ImageAdapter() { image = NULL; }

void		SetImage(Image *img) { image = img; }
};

#endif
