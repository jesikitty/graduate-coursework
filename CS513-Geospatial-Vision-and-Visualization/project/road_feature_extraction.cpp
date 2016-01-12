//Jesi Merrick
//CS513
//Final Project
//Detect road boundaries, lane markings, and lane geometry for LIDAR point cloud data.

#define _USE_MATH_DEFINES

#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <vector>
#include <math.h>

#include <boost/algorithm/string/predicate.hpp>

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/common/common_headers.h>
#include <pcl/features/normal_3d.h>
#include <pcl/features/impl/normal_3d.hpp>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/console/parse.h>
#include <pcl/registration/icp.h>
#include <pcl/common/transforms.h>
#include <pcl/filters/statistical_outlier_removal.h>
#include <pcl/filters/impl/statistical_outlier_removal.hpp>
#include <pcl/filters/filter.h>
#include <pcl/segmentation/conditional_euclidean_clustering.h>

using namespace std;

typedef pcl::PointXYZI PointTypeIO;
typedef pcl::PointXYZINormal PointTypeFull;

//Function declarations
int createPCDFiles();
int viewPCDFiles();
int detectRoad();

bool enforceCurvatureOrIntensitySimilarity (const PointTypeFull& point_a, const PointTypeFull& point_b, float squared_distance);

int main (int argc, char** argv){
  createPCDFiles();
  //viewPCDFiles();
  detectRoad();

  printf("TERMINATING\n");
  return (0);
}

//creates PCD files from the raw data
int createPCDFiles(){
  //Main directory where input data is stored
  std::string mainDir = string("../lidar_data");
  vector<string> dataFolders = vector<string>();

  //retrieve list of data files
  DIR *dp; 
  struct dirent *dirp;
  if((dp  = opendir(mainDir.c_str())) == NULL) {
      cout << "Error(" << errno << ") opening " << mainDir << endl;
      return errno;
  }

  while ((dirp = readdir(dp)) != NULL) {
    if(boost::starts_with(string(dirp->d_name), "chunk"))
     dataFolders.push_back(string(dirp->d_name));
  }
  closedir(dp);

  //For each folder of data, scan through the files
  DIR *temp_dp;
  struct dirent *temp_dirp;
  vector<string> tempFiles = vector<string>();
  std::string temp_dir;
  for(unsigned int i = 0; i < dataFolders.size(); i++){
    temp_dir = mainDir + "/" + dataFolders[i];

    if((temp_dp  = opendir(temp_dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << temp_dir << endl;
        return errno;
    }

    while ((temp_dirp = readdir(temp_dp)) != NULL) {
      if (boost::starts_with(string(temp_dirp->d_name), "dump0"))
        tempFiles.push_back(string(temp_dirp->d_name));
    }
    closedir(temp_dp);

    //Create cloud for that folder
    pcl::PointCloud<pcl::PointXYZI> cloud;

    //scan through all files to get # of lines = cloud.width
    ifstream tempFile;
    int totalNumLines = 0;
    for(unsigned int j = 0; j < tempFiles.size(); j++){
      std::string inputFile = tempFiles[j];
      std::string fullFilePath = mainDir + "/" + dataFolders[i] + "/" + inputFile;
      std::string dataLine;

      tempFile.open(fullFilePath.c_str());
      for (int k = 0; std::getline(tempFile, dataLine); ++k){
        totalNumLines++;
      }
      tempFile.close();
      tempFile.clear();
    }

    //Cloud data
    cloud.width    = totalNumLines; 
    cloud.height   = 1; //1 signifies unorganized datasets
    cloud.is_dense = false;
    cloud.points.resize(cloud.width * cloud.height);

    //Have all the files for a particular folder
    //Scan through them, only reading from the ones that read dump%int.txt
    int k = 0;
    for (unsigned int j = 0; j < tempFiles.size(); j++) {
        std::string inputFile = tempFiles[j];
        
        //check if valid data file
        if (boost::starts_with(inputFile, "dump0")){ //FIX - if there are a larger amount of dump files...
          std::string fullFilePath = mainDir + "/" + dataFolders[i] + "/" + inputFile;
          std::string dataLine;

          tempFile.open(fullFilePath.c_str());
          if(tempFile.is_open()){
            while(getline(tempFile, dataLine)){
              //Parse line of point data
              std::vector<char> v(dataLine.begin(), dataLine.end());
              v.push_back('\0');
              char * pch;
              //Skip unnecessary data
              pch = strtok (&v[0],","); pch = strtok (NULL,",");
              
              pch = strtok (NULL,","); 
              double latitude = atof(pch);
              
              pch = strtok (NULL,",");
              double longitude = atof(pch);
              
              pch = strtok (NULL,",");
              double altitude = atof(pch);

              pch = strtok (NULL,",");
              int intensity = atoi(pch);

              //convert LLA to XYZ
              double cosLat = cos(latitude * M_PI / 180.0);
              double sinLat = sin(latitude * M_PI / 180.0);
              double cosLon = cos(longitude * M_PI / 180.0);
              double sinLon = sin(longitude * M_PI / 180.0);
              double rad = 500.0;
              cloud.points[k].x = rad * cosLat * cosLon;
              cloud.points[k].y = rad * cosLat * sinLon;
              cloud.points[k].z = rad * sinLat;
              cloud.points[k].intensity = intensity;

              k++;
            }
            tempFile.close();
            tempFile.clear();
          }
          else {
            cout << "Unable to open input file";
          }
        }
        tempFiles.clear(); //clear the temp files for this directory
    }

    std::ostringstream fileNum; fileNum << i;
    std::string pcdFileName = "cloudData/" + dataFolders[i] + ".pcd";
    pcl::io::savePCDFileASCII (pcdFileName, cloud);
    std::cerr << "Saved " << cloud.points.size () << " data points as " << pcdFileName << std::endl;
  }
  return 0;
}

int viewPCDFiles(){
  //Visualize cloud
  boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer (new pcl::visualization::PCLVisualizer ("3D Viewer"));

  //add all clouds
  DIR *dp;
  struct dirent *dirp;
  vector<string> dataFiles = vector<string>(); //all the stored 
  std::string dir = "cloudData";

  if((dp  = opendir(dir.c_str())) == NULL) {
    cout << "Error(" << errno << ") opening " << dir << endl;
    return errno;
  }

  while ((dirp = readdir(dp)) != NULL) {
    if (boost::starts_with(string(dirp->d_name), "chunk"))
      dataFiles.push_back(string(dirp->d_name));
  }
  closedir(dp);

  sort(dataFiles.begin(), dataFiles.end());

  pcl::PointCloud<pcl::PointXYZI>::Ptr ptrCloud(new pcl::PointCloud<pcl::PointXYZI>);
  //printf("Total of %zu pcd files to load\n", dataFiles.size());
  for (unsigned int i = 0; i < dataFiles.size(); i++) { 
    std::string inputFile = dataFiles[i];
    
    //check if valid data file
    std::string tempDir = dir + "/" + dataFiles[i];
    cout << "Getting file " << tempDir << endl;
    if (pcl::io::loadPCDFile<pcl::PointXYZI>(tempDir, *ptrCloud) == -1) {
      PCL_ERROR ("Couldn't read file a pcd file \n");
      return (-1);
    }

    std::ostringstream fileNum; fileNum << i;
    viewer->addPointCloud<pcl::PointXYZI> (ptrCloud, fileNum.str());
    viewer->setPointCloudRenderingProperties (pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, fileNum.str());
  }
  printf("Done loading files\n");

  viewer->addCoordinateSystem (1.0);
  viewer->initCameraParameters ();
  //viewer->setCameraPosition(321, 48, 380, 321, 48, 380, 0); 
  //viewer->setPosition(321, 48);
  while (!viewer->wasStopped ())
  {
    viewer->spinOnce (100);
    boost::this_thread::sleep (boost::posix_time::microseconds (100000));
  }
}

int detectRoad(){
  //add all clouds
  DIR *dp; struct dirent *dirp;
  vector<string> dataFiles = vector<string>(); //all the original stored files
  std::string dir = "cloudData";

  if((dp  = opendir(dir.c_str())) == NULL) {
    cout << "Error(" << errno << ") opening " << dir << endl;
    return errno;
  }

  while ((dirp = readdir(dp)) != NULL) {
    if (boost::starts_with(string(dirp->d_name), "chunk"))
      dataFiles.push_back(string(dirp->d_name));
  }
  closedir(dp);

  pcl::PointCloud<pcl::PointXYZI>::Ptr ptrCloudOriginal(new pcl::PointCloud<pcl::PointXYZI>);
  for (unsigned int i = 0; i < dataFiles.size(); i++) {
    std::string inputFile = dataFiles[i];
    
    //check if valid data file
    std::string tempDir = dir + "/" + dataFiles[i];
    //cout << "Getting file " << tempDir << endl;
    if (pcl::io::loadPCDFile<pcl::PointXYZI>(tempDir, *ptrCloudOriginal) == -1) {
      PCL_ERROR ("Couldn't read file a pcd file \n");
      return (-1);
    }
    //determine size of filter cloud
    int filteredCloudWidth = 0;
    for (size_t j = 0; j < ptrCloudOriginal->points.size(); j++){
      if(ptrCloudOriginal->points[j].intensity > 185)
        filteredCloudWidth++;
    }

    //create new cloud to write as filtered pcd object
    //pcl::PointCloud<pcl::PointXYZI> filteredCloud;
    pcl::PointCloud<pcl::PointXYZI>::Ptr ptrfilteredCloud(new pcl::PointCloud<pcl::PointXYZI>);
    ptrfilteredCloud->width    = filteredCloudWidth; 
    ptrfilteredCloud->height   = 1; //1 signifies unorganized datasets
    ptrfilteredCloud->is_dense = false;
    ptrfilteredCloud->points.resize(ptrfilteredCloud->width * ptrfilteredCloud->height);

    //filter out points that are of a high intensity
    int filteredIndex = 0;
    for (size_t j = 0; j < ptrCloudOriginal->points.size(); j++){
      if(ptrCloudOriginal->points[j].intensity > 185) {
        ptrfilteredCloud->points[filteredIndex].x = ptrCloudOriginal->points[j].x;
        ptrfilteredCloud->points[filteredIndex].y = ptrCloudOriginal->points[j].y;
        ptrfilteredCloud->points[filteredIndex].z = ptrCloudOriginal->points[j].z;
        ptrfilteredCloud->points[filteredIndex].intensity = ptrCloudOriginal->points[j].intensity;
        filteredIndex++;
      }
    }

    //Remove noise
    //pcl::PointCloud<pcl::PointXYZI>::Ptr ptrFilteredCloud(&filteredCloud);
    pcl::PointCloud<pcl::PointXYZI>::Ptr ptrfilteredCloudNoise (new pcl::PointCloud<pcl::PointXYZI>);
    pcl::StatisticalOutlierRemoval<pcl::PointXYZI> sor;
    sor.setInputCloud (ptrfilteredCloud);
    sor.setMeanK (50);
    sor.setStddevMulThresh (1.0);
    sor.filter (*ptrfilteredCloudNoise);

    pcl::PointCloud<PointTypeFull>::Ptr cloud_with_normals (new pcl::PointCloud<PointTypeFull>);
    pcl::IndicesClustersPtr clusters (new pcl::IndicesClusters), small_clusters (new pcl::IndicesClusters), large_clusters (new pcl::IndicesClusters);
    pcl::search::KdTree<PointTypeIO>::Ptr search_tree (new pcl::search::KdTree<PointTypeIO>);

    // Set up a Normal Estimation class and merge data in cloud_with_normals
     pcl::copyPointCloud (*ptrfilteredCloudNoise, *cloud_with_normals);
     pcl::NormalEstimation<PointTypeIO, PointTypeFull> ne;
     ne.setInputCloud (ptrfilteredCloudNoise);
     ne.setSearchMethod (search_tree);
     ne.setRadiusSearch (300.0);
     ne.compute (*cloud_with_normals);

     // Set up a Conditional Euclidean Clustering class
     pcl::ConditionalEuclideanClustering<PointTypeFull> cec (true);
     cec.setInputCloud (cloud_with_normals);
     cec.setConditionFunction (&enforceCurvatureOrIntensitySimilarity); //customize
     cec.setClusterTolerance (500.0);
     cec.setMinClusterSize (cloud_with_normals->points.size () / 1000); 
     cec.setMaxClusterSize (cloud_with_normals->points.size () / 5);
     cec.segment (*clusters);
     cec.getRemovedClusters (small_clusters, large_clusters);

     // Using the intensity channel for lazy visualization of the output
     for (int j = 0; j < small_clusters->size (); ++j)
       for (int k = 0; k < (*small_clusters)[j].indices.size (); ++k)
         ptrfilteredCloudNoise->points[(*small_clusters)[j].indices[k]].intensity = -2.0;
     for (int j = 0; j < large_clusters->size (); ++j)
       for (int k = 0; k < (*large_clusters)[j].indices.size (); ++k)
         ptrfilteredCloudNoise->points[(*large_clusters)[j].indices[k]].intensity = +10.0;
     for (int j = 0; j < clusters->size (); ++j)
     {
       int label = rand () % 8;
       for (int k = 0; k < (*clusters)[j].indices.size (); ++k)
         ptrfilteredCloudNoise->points[(*clusters)[j].indices[k]].intensity = label;
     }

    std::string pcdFileName = "cloudDataResults/" + dataFiles[i];
    pcl::io::savePCDFile (pcdFileName, *ptrfilteredCloudNoise);
    std::cerr << "Saved " << ptrfilteredCloud->points.size () << " data points as " << pcdFileName << std::endl;
  }
}

bool enforceCurvatureOrIntensitySimilarity (const PointTypeFull& point_a, const PointTypeFull& point_b, float squared_distance)
{
  Eigen::Map<const Eigen::Vector3f> point_a_normal = point_a.normal, point_b_normal = point_b.normal;
  if (fabs (point_a.intensity - point_b.intensity) < 1.0f) //customize -> maybe 2 or 3?
    return (true);
  if (fabs (point_a_normal.dot (point_b_normal)) < 0.05)
    return (true);
  return (false);
}