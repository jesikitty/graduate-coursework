using System;
using System.Text;
using System.Net; 

public class AerialImageRetrieval
{
	private const double EarthRadius = 6378137;
	private const double MinLatitude = -85.05112878;
	private const double MaxLatitude = 85.05112878;
	private const double MinLongitude = -180;
	private const double MaxLongitude = 180;

   public static void Main()
   {
   	  //has a limited life span of one month
   	  //A key is required to use Bings Map services
   	  string BingMapsKey = "";

   	  double minLocalLatitude;
   	  double maxLocalLatitude;
   	  double minLocalLongitude;
   	  double maxLocalLongitude;

   	  string input;
   	  Console.WriteLine("Enter minimum latitude:");
   	  input = Console.ReadLine(); double.TryParse(input, out minLocalLatitude);
   	  Console.WriteLine("Enter minimum longitude:");
   	  input = Console.ReadLine(); double.TryParse(input, out minLocalLongitude);
   	  Console.WriteLine("Enter maximum latitude:");
   	  input = Console.ReadLine(); double.TryParse(input, out maxLocalLatitude);
   	  Console.WriteLine("Enter maximum longtitude:");
   	  input = Console.ReadLine(); double.TryParse(input, out maxLocalLongitude);


   	  int minpixelX, maxpixelX, minpixelY, maxpixelY;
   	  int minX, maxX, minY, maxY; //switched out from query at the moment -> not working correctly

   	  LatLongToPixelXY(minLocalLatitude, minLocalLongitude, 1, out minpixelX, out minpixelY);
   	  LatLongToPixelXY(maxLocalLatitude, maxLocalLongitude, 1, out maxpixelX, out maxpixelY);

   	  PixelXYToTileXY(minpixelX, minpixelY, out minX, out minY);
   	  PixelXYToTileXY(maxpixelX, maxpixelY, out maxX, out maxY);

   	  //Console.WriteLine(minX + "," + minY + "," + maxX + "," + maxY);
   	  //Console.WriteLine(minLocalLatitude + "," + minLocalLongitude + "," + maxLocalLatitude + "," + maxLocalLongitude);

      string localFilename = @"test.jpg";
      using(WebClient client = new WebClient())
      {	
          try {
          client.DownloadFile("http://dev.virtualearth.net/REST/v1/Imagery/Map/Aerial/1?mapArea=" + 
          	 minLocalLatitude + "," + minLocalLongitude + "," + maxLocalLatitude + "," + maxLocalLongitude + "&mapSize=900,834&key=" +
          	BingMapsKey, localFilename);
      	}
      	catch (Exception e)
      	{
      	    Console.WriteLine(e.Message);
      	    Console.Read();
      	}
      }
   }


// METHODS FOR CONVERTING BETWEEN TILES/PIXELS/LAT&LONG
//See: http://msdn.microsoft.com/en-us/library/bb259689.aspx

   // Clips a number to the specified minimum and maximum values.
   private static double Clip(double n, double minValue, double maxValue)
   {
       return Math.Min(Math.Max(n, minValue), maxValue);
   }
   
   // Determines the map width and height (in pixels) at a specified level of detail.
   public static uint MapSize(int levelOfDetail)
   {
       return (uint) 256 << levelOfDetail;
   }

   // Converts a point from latitude/longitude WGS-84 coordinates (in degrees) into pixel XY coordinates
   // at a specified level of detail.
   public static void LatLongToPixelXY(double latitude, double longitude, int levelOfDetail, out int pixelX, out int pixelY)
   {
       latitude = Clip(latitude, MinLatitude, MaxLatitude);
       longitude = Clip(longitude, MinLongitude, MaxLongitude);

       double x = (longitude + 180) / 360; 
       double sinLatitude = Math.Sin(latitude * Math.PI / 180);
       double y = 0.5 - Math.Log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * Math.PI);

       uint mapSize = MapSize(levelOfDetail);
       pixelX = (int) Clip(x * mapSize + 0.5, 0, mapSize - 1);
       pixelY = (int) Clip(y * mapSize + 0.5, 0, mapSize - 1);
   }

   // Converts pixel XY coordinates into tile XY coordinates of the tile containing
   // the specified pixel.
   public static void PixelXYToTileXY(int pixelX, int pixelY, out int tileX, out int tileY)
   {
       tileX = pixelX / 256;
       tileY = pixelY / 256;
   }
}