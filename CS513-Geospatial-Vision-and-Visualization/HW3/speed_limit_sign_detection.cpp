// Jesi Merrick
// CS513 - Spring 2015
// Homework 3

//NOTE: Change according to your setup. Had to use full path due to setup issues with my machine.
#include </home/jesikitty/ImageMagick-6.9.1-1/Magick++/lib/Magick++.h> 
#include <iostream> 

using namespace std; 
using namespace Magick; 

int main(int argc, char **argv) 
{ 
  InitializeMagick(*argv);

  Image image;
  try { 
    // Read a file into image object
    // NOTE: change to the file you wish to run the program on 
    image.read("input.jpg");

    image.type( GrayscaleType );
    //image.write("test_output_gray.jpg");

    image.blur(); image.blur();
    //image.write("test_output_blur.jpg");

    //Locate edges
    image.edge(3);
    //image.write("test_output_edge.jpg");

    //Image size
    int w = image.columns();
    int h = image.rows();

    image.type(TrueColorType);
    image.modifyImage();

    Color green("green"); 
    Pixels view(image);
    PixelPacket *pixels = view.get(0,0,w,h);
    Magick::Color colors [9], color; 

    //Fairly ugly piece of code
    //Was going for brute force, and then refining it
    for (int row = 0; row < h; ++row) {
      for (int column = 0; column < w; ++column) {
        color = *pixels;
        colors[0] = pixels[w * (row-1) + (column-1)];
        colors[1] = pixels[w * (row-1) + column];
        colors[2] = pixels[w * (row-1) + (column+1)];
        colors[3] = pixels[w * row + (column-1)];
        colors[4] = pixels[w * row + column];
        colors[5] = pixels[w * row + (column+1)];
        colors[6] = pixels[w * (row+1) + (column-1)];
        colors[7] = pixels[w * (row+1) + column];
        colors[8] = pixels[w * (row+1) + (column+1)];
        if (color.intensity() < 32768 && colors[0].intensity() > 32768 && colors[1].intensity() > 32768 && colors[2].intensity() > 32768){
          if (colors[6].intensity() > 32768 && colors[7].intensity() > 32768 && colors[8].intensity() > 32768){
            *pixels = green;
          }
        }
        *pixels++; 
      }
    }

   // Save changes to image.
    view.sync();

   // Write the image to a file 
   image.write( "output.jpg" ); 

   cout << "Program complete\n"; 
    } 
    catch( Exception &error_ ) 
    { 
      cout << "Caught exception: " << error_.what() << endl; 
      return 1; 
    } 

  return 0; 
}