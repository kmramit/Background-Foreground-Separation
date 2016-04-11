#include <stdio.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "libCodeBook.cpp"

using namespace cv;

int temporal_bound, algo_phase, num_frames;

int main(int argc, char* argv[])
{
    /* Provide your own pathname */
    //VideoCapture cap("../../videos/input_video_sample1.mov");
    if (argc != 3) {
	 cerr <<"Incorrect input" << endl;
	 cerr <<"exiting..." << endl;
	 return EXIT_FAILURE;
    }

    //VideoCapture tempcap("../../videos/sample5_cut.avi");
    VideoCapture cap(argv[1]);

    Mat frame, frame2;
    int time = 0,i,j,m;
    bool ret;
    /*
    if(!tempcap.isOpened())
        return -1;

    printf("Going to frame\n");
    for(i=0;i<=1500;i++) {
	ret = tempcap.read(frame);
	if(!ret)
	    break;
    }
    printf("Frame reached\n");
    */

    if(!cap.isOpened())
        return -1;

    ////////////////////////////
    // TRAINING
    ////////////////////////////

    CodeBook codebooks[480][640];
    printf("Codebooks initialised\n");

    int blue, green, red;
    Vec3b intensity;

    algo_phase = 0;
    for(;;)
    {
        //ret = tempcap.read(frame);
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
        if(time == 600)
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

    temporal_bound = 3*num_frames/4;

    sum = 0;
    bool Noise[480][640];
    for(j=0;j<480;j++) {
        for(i=0;i<640;i++) {
            codebooks[j][i].TemporalFit(0);
            sum += codebooks[j][i].Length(0);
	    //Noise[j][i] = true;
        }
    }
    printf("Average codewords per codebook (after temporal fitting) = %f\n",(sum*1.0)/(480*640));
   
    /////////////////////
    // TESTING
    /////////////////////

    Mat frame3(480, 640, CV_8UC1);
    //namedWindow("original",1);
    //namedWindow( "Components", 1 );
    FILE *fp;
    fp = fopen(argv[2],"w");

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
		    /*
		    if((Noise[j][i] == true) || (time <= 400)) {
			if(codebooks[j][i].DetectForeground(1, red, green, blue, time) == BACKGROUND)
			    frame3.at<uchar>(j, i) = 120;
			else
			    frame3.at<uchar>(j, i) = 255;
		    }
		    else
		    */
		    if(codebooks[j][i].DetectForeground(1, red, green, blue, time) == BACKGROUND)
			frame3.at<uchar>(j, i) = 120;
		    else
			frame3.at<uchar>(j, i) = 255;
		}
                else
                    frame3.at<uchar>(j, i) = 0;
		//Noise[j][i] = true;
            }
        }
        //imshow("classified",frame3);

	Mat dst = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat dst1 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat dst2 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat dst3 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat t1 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat omega = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat result = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat t2 = Mat::zeros(frame3.rows, frame3.cols, CV_8UC1);
	Mat element = getStructuringElement( MORPH_RECT, Size( 5,5 ), Point( -1,-1 ) );
	Rect bound;
	//medianBlur(frame3, frame3, 3); 
	GaussianBlur(frame3, frame3, Size(21,21), 0, 0);
	threshold(frame3, frame3, 120, 255, THRESH_BINARY);
	morphologyEx( frame3, frame3, MORPH_CLOSE, element );
	//erode( dst2, dst3, element );
	dilate(frame3, frame3, element);
	//imshow( "Components", dst3);
	//memset(Noise,true,sizeof(Noise));
	
	int p;
	///////////////////////////////////////////////////////////////////////
	// Construction of t1
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(frame3, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	vector<vector<Point> >hull( contours.size() );
	for(i = 0; i < contours.size(); i++ ) {  
	    //convexHull( Mat(contours[i]), hull[i], false ); 
	    bound = boundingRect(contours[i]);
	    if(bound.y >= 320) {
		if(contourArea(contours[i]) >= 1600)
		    drawContours( t1, contours, i, Scalar(255), CV_FILLED, 8 ,vector<Vec4i>(), 0, Point());
	    }
	    else {
		if(contourArea(contours[i]) >= 200)
		    drawContours( t1, contours, i, Scalar(255), CV_FILLED, 8 ,vector<Vec4i>(), 0, Point());
	    }
	}

	// Apply Sobel edge detector
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
    	Mat src_gray;
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	GaussianBlur( frame2, frame2, Size(3,3), 0, 0, BORDER_DEFAULT );
	cvtColor(frame2, src_gray, CV_BGR2GRAY);

	Sobel( src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
	Sobel( src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );	

	convertScaleAbs( grad_x, abs_grad_x );
	convertScaleAbs( grad_y, abs_grad_y );
	addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, omega );

	// Construction of t2
	findContours(omega, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	//vector<vector<Point> >hull( contours.size() );
	for(i = 0; i < contours.size(); i++ ) {  
	    //convexHull( Mat(contours[i]), hull[i], false ); 
	    bound = boundingRect(contours[i]);
	    if(bound.y >= 320) {
		if(contourArea(contours[i]) >= 1600)
		    drawContours( t2, contours, i, Scalar(255), CV_FILLED, 8 ,vector<Vec4i>(), 0, Point());
	    }
	    else {
		if(contourArea(contours[i]) >= 200)
		    drawContours( t2, contours, i, Scalar(255), CV_FILLED, 8 ,vector<Vec4i>(), 0, Point());
	    }
	}

	// Final intersection
	for(j=0;j<480;j++) {
	    for(i=0;i<640;i++) {
		result.at<uchar>(j, i) = 0;
		if((t1.at<uchar>(j,i) == 255) && (t2.at<uchar>(j,i) == 255))
		    result.at<uchar>(j, i) = 255;
	    }
	}
	GaussianBlur(frame3, dst, Size(21,21), 0, 0);
	morphologyEx( result, result, MORPH_CLOSE, element );
	dilate(result, result, element);
	//imshow( "Components", result);
	/////////////////////////////////////////////////////////////////////////


		findContours( result, contours, hierarchy,
			CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

		vector<vector<Point> > contours_poly( contours.size() );
		vector<Rect> boundRect( contours.size() );

		double area;
		vector<Rect> boundRectFiltered;
		for(i=0; (i>=0) && (i<contours.size());i=hierarchy[i][0]) {
		    bound = boundingRect(contours[i]);
		    area = contourArea(contours[i]);
		    if((bound.y >= 240)) {
			if(area >= 1600)
			    boundRectFiltered.push_back(bound);
		    }
		    else if(area >= 150) {
			boundRectFiltered.push_back(bound);
		    }
		}

		Scalar color = Scalar( 255, 255, 255 );
		for(i=0;i<boundRectFiltered.size(); i++) {
		    if(time >= 600)
			fprintf(fp,"%d,%d,%d,%d,%d\n",time,boundRectFiltered[i].x,boundRectFiltered[i].y,boundRectFiltered[i].width,boundRectFiltered[i].height);
		    rectangle(frame2, boundRectFiltered[i].tl(), boundRectFiltered[i].br(), color, 2, 8 ,0);
		}

        //imshow("original",frame2);
        //if(waitKey(30) >= 0)
        //    break;

	if(time % 100 == 1)
		printf("Frame %d\n", time);
        time++;
    }
    cap.release();

    return 0; 
}
