Jesi Merrick
CS513
Homework 3: Speed Limit Sign Detection (C++)

For this program, ImageMagick is needed (specifically, the Magick++ API was used).
See the following link:
http://www.imagemagick.org/script/magick++.php

To run the speed limit sign detection program, first compile the program by using the following command:
	c++ -O2 -o speed_limit_sign_detection speed_limit_sign_detection.cpp `Magick++-config --cppflags --cxxflags --ldflags --libs`

Then run the program using:
	./speed_limit_sign_detection

The program will look for an image called "input.jpg" within the same folder, and create an image called "output.jpg" before exiting. The output image will clearly show the bounding box of the speed limit sign.
