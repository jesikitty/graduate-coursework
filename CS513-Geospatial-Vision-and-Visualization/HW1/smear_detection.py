import os
import glob
import time

from SimpleCV import *

#To be added later in order to play effects automatically to all images

#my_images_path = "/home/jesikitty/Desktop/sample_drive/" 
#extension = "*.jpg"

#imgs = list() #load up an image list
#directory = os.path.join(path, extension)
#files = glob.glob(directory)

#for file in files:

#test image for working out strong solution
test1 = Image("/home/jesikitty/Desktop/test.jpg")

#The "smear" looks much more like a smear now, and easier to identify
test2 = test1.erode(20)
test2.save("/home/jesikitty/Desktop/step1.jpg")

#adding light -> most of the simplecv methods will apply better
#can identify the smear as smaller amount of white among color
test3 = test2*3
test3.save("/home/jesikitty/Desktop/step2.jpg")

#mask = test3.createBinaryMask(color1=(0,128,128),color2=(255,255,255))
#test4 = test3.applyBinaryMask(mask)

#having some issues with this functionality of simplecv
#should be able to capture the smear by specifying the program to select
#smaller groups of white (excludes things like sky, etc, by size)
blobs = test3.findBlobs(-1)
blobs.draw()

test3.save("/home/jesikitty/Desktop/step3.jpg")

