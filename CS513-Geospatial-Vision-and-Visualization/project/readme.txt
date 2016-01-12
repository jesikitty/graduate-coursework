Jesi Merrick
CS513
Final Project: Automatic Road Feature Extraction from LIDAR

/* SUMMARY */

This program was completed using the Point Cloud Library (PLC), an open source project for point cloud processing.

To begin, the program will scan through the raw LIDAR data, merge the contents of each folder, and convert the results to .pcd files. In this form, the data is easy to manipulate as well as view in cloud point viewers (for this project, I used the built-in viewer of PLC as well as CloudCompare).

The program then strips out points that are a low intensity (< 185) and removes some of the noise.

I experimented with refining the point cloud by working with the normals as well as the intensity (grouping together points according to similarites in intensity or normals).

To Do:
More clearly mark the lane markings and road edges
	I attempted this, but could not find a way to automate it (up until this point, all progress was automated - snapshots are from in-progress pcd files)
	I’m not sure if this is easily done in the point cloud format, or if I would be forced to automate a way to capture the point cloud as an image, and then detect the lines

/* TO EXECUTE */

This project was completed using Point Cloud Library (PCL) (http://pointclouds.org/). To run the executable, it is required to download the library from their site.
Additional links that may be helpful with setting up to run this program:
	http://pointclouds.org/documentation/tutorials/using_pcl_pcl_config.php#using-pcl-pcl-config
  	http://pointclouds.org/documentation/tutorials/compiling_pcl_posix.php#compiling-pcl-posix

To compile the program use the command:
	make
Afterward, to run the program:
	./road_feature_extraction
Note: The program does take a while to run! Make sure you have the time for it!

 Expected folder structure:
 project 
 	> lidar_data
 	> readme.txt
 	> build
 		> CMakeCache.txt
 		> road_feature_extraction.cpp
 		> cloudData //initially empty
 		> cloudDataResults //initially empty
May include additional folders and files created through installation and compiling.