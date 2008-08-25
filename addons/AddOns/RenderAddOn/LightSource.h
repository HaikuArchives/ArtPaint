/* 

	Filename:	LightSource.h
	Contents:	LightSource-class declaration and definitions	
	Author:		
    Notes:      Definition for single light, assumes full fpu implementation.
                Coordinates left handed.
*/

#ifndef	LIGHT_SOURCE_H
#define	LIGHT_SOURCE_H


class LightSource {
public:
		LightSource(float X,float Y,float Z) {
			x = X;
			y = Y;
			z = Z;
            red = 255.0;
            green = 255.0;
            blue = 255.0;
            intensity = 1.0;
		}

		LightSource() {
            x = 0.0;
            y = 0.0;
			z = -1;
			red = 255;
			green = 255;
			blue = 255;
            intensity = 1.0;
		}

		
		LightSource(const LightSource& s) {
			x = s.x;
			y = s.y;
			z = s.z;
			red = s.red;
			green = s.green;
			blue = s.blue;
			intensity = s.intensity;
		}
		
		LightSource& operator=(const LightSource& s) {
			x = s.x;
			y = s.y;
			z = s.z;
			red = s.red;
			green = s.green;
			blue = s.blue;
			intensity = s.intensity;
						
			return *this;
		}

		bool operator==(const LightSource& s) {
			return ((x == s.x) && (y == s.y) && (z == s.z) &&
					(red == s.red) && (green == s.green) && (blue == s.blue) && (intensity == s.intensity));
        }

		bool operator!=(const LightSource& s) {
			return !(*this == s);
		}

        void SetParameters( float X,float Y,float Z,
                            float Red,float Green,float Blue,
                            float FocusX,float FocusY,float FocusZ,
                            float Intensity,float Concentration ) {

        x = X,y = Y,z = Z;
        red = Red,green = Green,blue = Blue;
        intensity     = Intensity;
        focusX        = FocusX;
        focusY        = FocusY;
        focusZ        = FocusZ;
        concentration = Concentration;

        }

float	x,y,z;
float	red,green,blue;
float   intensity;
float   focusX;
float   focusY;
float   focusZ;
float   concentration;

};
#endif
