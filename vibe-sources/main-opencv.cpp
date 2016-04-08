/**
 * @file main-opencv.cpp
 * @date July 2014 
 * @brief An exemplative main file for the use of ViBe and OpenCV
 */
#include <iostream>

#include <opencv2/opencv.hpp>
//#include <opencv/highgui.h>

#include "vibe-background-sequential.h"

using namespace cv;
using namespace std;

/** Function Headers */
void processVideo(char* videoFilename);

/**
 * Displays instructions on how to use this program.
 */

void help()
{
    cout
    << "--------------------------------------------------------------------------" << endl
    << "This program shows how to use ViBe with OpenCV                            " << endl
    << "Usage:"                                                                     << endl
    << "./main-opencv <video filename>"                                             << endl
    << "for example: ./main-opencv video.avi"                                       << endl
    << "--------------------------------------------------------------------------" << endl
    << endl;
}

/**
 * Main program. It shows how to use the grayscale version (C1R) and the RGB version (C3R). 
 */
int main(int argc, char* argv[])
{
  /* Print help information. */
  help();

  /* Check for the input parameter correctness. */
  if (argc != 2) {
    cerr <<"Incorrect input" << endl;
    cerr <<"exiting..." << endl;
    return EXIT_FAILURE;
  }

  /* Create GUI windows. */
  namedWindow("Frame");
  namedWindow("Segmentation by ViBe");

  processVideo(argv[1]);

  /* Destroy GUI windows. */
  destroyAllWindows();
  return EXIT_SUCCESS;
}

/**
 * Processes the video. The code of ViBe is included here. 
 *
 * @param videoFilename  The name of the input video file. 
 */
void processVideo(char* videoFilename)
{
  /* Create the capture object. */
  VideoCapture capture(videoFilename);

  if (!capture.isOpened()) {
    /* Error in opening the video input. */
    cerr << "Unable to open video file: " << videoFilename << endl;
    exit(EXIT_FAILURE);
  }

  /* Variables. */
  static int frameNumber = 1; /* The current frame number */
  Mat inpframe;                  /* Current frame. */
  Mat frame(480, 640, CV_8UC3);
  Mat segmentationMap;        /* Will contain the segmentation map. This is the binary output map. */
  int keyboard = 0;           /* Input from keyboard. Used to stop the program. Enter 'q' to quit. */

  /* Model for ViBe. */
  vibeModel_Sequential_t *model = NULL; /* Model used by ViBe. */

  /* Read input data. ESC or 'q' for quitting. */
  while ((char)keyboard != 'q' && (char)keyboard != 27) {
    // Read the current frame.
    if (!capture.read(inpframe)) {
      cerr << "Unable to read next frame." << endl;
      cerr << "Exiting..." << endl;
      exit(EXIT_FAILURE);
    }
    resize(inpframe, frame, Size(640, 480), 0, 0, INTER_LINEAR);

    if ((frameNumber % 100) == 0) { cout << "Frame number = " << frameNumber << endl; }

    /* Applying ViBe.
     * If you want to use the grayscale version of ViBe (which is much faster!):
     * (1) remplace C3R by C1R in this file.
     * (2) uncomment the next line (cvtColor).
     */
    cvtColor(frame, frame, CV_BGR2GRAY);

    if (frameNumber == 1) {
      segmentationMap = Mat(frame.rows, frame.cols, CV_8UC1);
      model = (vibeModel_Sequential_t*)libvibeModel_Sequential_New();
      libvibeModel_Sequential_AllocInit_8u_C1R(model, frame.data, frame.cols, frame.rows);
    }

    /* ViBe: Segmentation and updating. */
    libvibeModel_Sequential_Segmentation_8u_C1R(model, frame.data, segmentationMap.data);
    libvibeModel_Sequential_Update_8u_C1R(model, frame.data, segmentationMap.data);

    /* Post-processes the segmentation map. This step is not compulsory. 
       Note that we strongly recommend to use post-processing filters, as they 
       always smooth the segmentation map. For example, the post-processing filter 
       used for the Change Detection dataset (see http://www.changedetection.net/ ) 
       is a 5x5 median filter. */
    medianBlur(segmentationMap, segmentationMap, 3); /* 3x3 median filtering */
/*
    Mat element = getStructuringElement(MORPH_ELLIPSE, Size(5,5), Point(-1,-1));
    morphologyEx(segmentationMap, segmentationMap, MORPH_CLOSE, element);
*/

    /* Shows the current frame and the segmentation map. */
    imshow("Segmentation by ViBe", segmentationMap);

                vector<vector<Point> > contours;
                vector<Vec4i> hierarchy;

                findContours( segmentationMap, contours, hierarchy,
                        CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

                vector<vector<Point> > contours_poly( contours.size() );
                vector<Rect> boundRect( contours.size() );

                RNG rng(12345);
                for(int i=0 ; (i >= 0) && (i < contours.size()); i = hierarchy[i][0] )
                {
                        //approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
                        //boundRect[i] = boundingRect( Mat(contours_poly[i]) );
                        boundRect[i] = boundingRect( contours[i] );

                        Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
                        //drawContours( dst1, contours_poly, (int)i, color, 1, 8, vector<Vec4i>(), 0, Point() );
                        if (contourArea(contours[i]) >= 250)
                                rectangle( frame, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
                }

    imshow("Frame", frame);
    ++frameNumber;

    /* Gets the input from the keyboard. */
    keyboard = waitKey(1);
  }

  /* Delete capture object. */
  capture.release();

  /* Frees the model. */
  libvibeModel_Sequential_Free(model);
}
