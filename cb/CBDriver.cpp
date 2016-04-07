#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "libCodeBook.cpp"

using namespace cv;

int temporal_bound, algo_phase, num_frames;

int main(int argc, char* argv[])
{
    /* Provide your own pathname */
    //VideoCapture cap("../../videos/input_video_sample1.mov");
    if (argc != 2) {
	 cerr <<"Incorrect input" << endl;
	 cerr <<"exiting..." << endl;
	 return EXIT_FAILURE;
    }

    VideoCapture cap(argv[1]);

    if(!cap.isOpened())
        return -1;

    ////////////////////////////
    // TRAINING
    ////////////////////////////

    CodeBook codebooks[480][640];
    printf("Codebooks initialised\n");

    int time = 0,i,j,m;
    Mat frame, frame2;
    int blue, green, red;
    Vec3b intensity;
    bool ret;

    algo_phase = 0;
    for(;;)
    {
        ret = cap.read(frame);
        if(!ret)
            break;
        
        // Resizing the image to save on memory. Will rectify this later.
        resize(frame, frame2, Size(640, 480), 0, 0, INTER_LINEAR);

        for(j=0;j<480;j++) {
            for(i=0;i<640;i++) {

                if(time == 0)
                    codebooks[j][i].Init();
                intensity = frame2.at<Vec3b>(j, i);	// BGR Tuple
                blue = (int) intensity.val[0];
                green = (int) intensity.val[1];
                red = (int) intensity.val[2];
                codebooks[j][i].FindMatch(0, red, green, blue, time);
                /* codebooks[j][i].FindMatch(intensity.val[2], intensity.val[1], intensity.val[0], time); */

            }
        }

        time++;
        if(time == 500)
            break;
    }
    //cap.release();

    ///////////////////////
    // TEMPORAL FITTING
    ///////////////////////

    int sum=0;

    num_frames = time;

    for(j=0;j<480;j++) {
        for(i=0;i<640;i++) {
            codebooks[j][i].WrapAround(0);
            sum += codebooks[j][i].Length(0);
        }
    }

    printf("Average codewords per codebook = %f\n",(sum*1.0)/(480*640));

    temporal_bound = 9*num_frames/10;

    sum = 0;
    for(j=0;j<480;j++) {
        for(i=0;i<640;i++) {
            codebooks[j][i].TemporalFit(0);
            sum += codebooks[j][i].Length(0);
        }
    }
    printf("Average codewords per codebook (after temporal fitting) = %f\n",(sum*1.0)/(480*640));
   
    /////////////////////
    // TESTING
    /////////////////////

    Mat frame3(480, 640, CV_8UC1);
    namedWindow("original",1);
    namedWindow( "Components", 1 );
    //namedWindow("classified",1);
    for(;;)
    {
        ret = cap.read(frame);
        if(!ret)
            break;

        resize(frame, frame2, Size(640, 480), 0, 0, INTER_LINEAR);

        for(j=0;j<480;j++) {
            for(i=0;i<640;i++) {

                intensity = frame2.at<Vec3b>(j, i);	// BGR Tuple
                blue = (int) intensity.val[0];
                green = (int) intensity.val[1];
                red = (int) intensity.val[2];
                //if((codebooks[j][i].DetectForeground(0, red, green, blue, time) == BACKGROUND) || 
		//	(codebooks[j][i].DetectForeground(1, red, green, blue, time) == BACKGROUND))
                if(codebooks[j][i].DetectForeground(0, red, green, blue, time) == FOREGROUND) {
		    if(codebooks[j][i].DetectForeground(1, red, green, blue, time) == BACKGROUND)
			frame3.at<uchar>(j, i) = 120;
		    else
                    	frame3.at<uchar>(j, i) = 255;
		}
                else
                    frame3.at<uchar>(j, i) = 0;
            }
        }
        //imshow("classified",frame3);

	Mat dst = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat dst1 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat dst2 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat dst3 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat element = getStructuringElement( MORPH_RECT, Size( 5,5 ), Point( -1,-1 ) );
	//medianBlur(frame3, frame3, 3); 
	GaussianBlur(frame3, dst, Size(21,21), 0, 0);
	morphologyEx( dst, dst1, MORPH_CLOSE, element );
	threshold(dst1, dst2, 70, 255, THRESH_BINARY);
	erode( dst2, dst3, element );
	imshow( "Components", dst3);
	//dilate(dst1, dst3, element);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		findContours( dst3, contours, hierarchy,
			CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

		vector<vector<Point> > contours_poly( contours.size() );
		vector<Rect> boundRect( contours.size() );

		RNG rng(12345);
		int i = 0;
		for( ; (i >= 0) && (i < contours.size()); i = hierarchy[i][0] )
		{ 
			//approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
			//boundRect[i] = boundingRect( Mat(contours_poly[i]) );
			boundRect[i] = boundingRect( contours[i] );

			Scalar color = Scalar( rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255) );
			//drawContours( dst1, contours_poly, (int)i, color, 1, 8, vector<Vec4i>(), 0, Point() );
			if (contourArea(contours[i]) >= 200)
				rectangle( frame2, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0 );
		}
		//namedWindow( "Contours", WINDOW_AUTOSIZE );
		//imshow( "Contours", drawing );

		/*
		// iterate through all the top-level contours,
		// draw each connected component with its own random color
		int idx = 0;
		for( ; (idx >= 0) && (contours.size()>0); idx = hierarchy[idx][0] )
		{
			Scalar color( rand()&255, rand()&255, rand()&255 );
			drawContours( dst, contours, idx, color, CV_FILLED, 8, hierarchy );
		}
		*/


        imshow("original",frame2);
        if(waitKey(30) >= 0)
            break;

	if(time % 100 == 0)
		printf("Frame %d\n", time);
        time++;
    }
    cap.release();

    return 0; 
}
