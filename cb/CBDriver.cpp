#include <stdio.h>
#include <opencv2/opencv.hpp>
#include "libCodeBook.cpp"

using namespace cv;

int temporal_bound, algo_phase, num_frames;

int main()
{
    /* Provide your own pathname */
    VideoCapture cap("/home/rajan/Courses/CS771/videos/input_video_sample3.mov");

    if(!cap.isOpened())
        return -1;

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
                codebooks[j][i].FindMatch(red, green, blue, time);
                /* codebooks[j][i].FindMatch(intensity.val[2], intensity.val[1], intensity.val[0], time); */

            }
        }

        time++;
    }
    cap.release();

    int sum=0;

    num_frames = time;

    for(j=0;j<480;j++) {
        for(i=0;i<640;i++) {
            codebooks[j][i].WrapAround();
            sum += codebooks[j][i].Length();
        }
    }

    printf("Average codewords per codebook = %f\n",(sum*1.0)/(480*640));

    temporal_bound = num_frames/2;

    sum = 0;
    for(j=0;j<480;j++) {
        for(i=0;i<640;i++) {
            codebooks[j][i].TemporalFit();
            sum += codebooks[j][i].Length();
        }
    }
    printf("Average codewords per codebook (after temporal fitting) = %f\n",(sum*1.0)/(480*640));
   
    return 0; 
}
